/*
 *  Copyright (C) 2024 Team Kodi
 *  Copyright (C) 2010-2021 Hendrik Leppkes
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 *
 *  The TrueHD seamless-branch handling (output-timing discontinuity detection
 *  and padding carry-forward) is derived from the TrueHD MAT packer in LAV
 *  Filters by Hendrik Leppkes (Nevcairiel), decoder/LAVAudio/BitstreamMAT.cpp.
 */

#include "PackerMAT.h"

#include "utils/BitstreamReader.h"
#include "utils/log.h"

#include <array>
#include <assert.h>
#include <utility>

extern "C"
{
#include <libavutil/common.h>
#include <libavutil/intreadwrite.h>
}

namespace
{
constexpr uint32_t FORMAT_MAJOR_SYNC = 0xf8726fba;

constexpr auto BURST_HEADER_SIZE = 8;
constexpr auto MAT_BUFFER_SIZE = 61440;
constexpr auto MAT_BUFFER_LIMIT = MAT_BUFFER_SIZE - 24; // MAT end code size
constexpr auto MAT_POS_MIDDLE = 30708 + BURST_HEADER_SIZE; // middle point + IEC header in front

// magic MAT format values, meaning is unknown at this point
constexpr std::array<uint8_t, 20> mat_start_code = {0x07, 0x9E, 0x00, 0x03, 0x84, 0x01, 0x01,
                                                    0x01, 0x80, 0x00, 0x56, 0xA5, 0x3B, 0xF4,
                                                    0x81, 0x83, 0x49, 0x80, 0x77, 0xE0};

constexpr std::array<uint8_t, 12> mat_middle_code = {0xC3, 0xC1, 0x42, 0x49, 0x3B, 0xFA,
                                                     0x82, 0x83, 0x49, 0x80, 0x77, 0xE0};

constexpr std::array<uint8_t, 24> mat_end_code = {0xC3, 0xC2, 0xC0, 0xC4, 0x00, 0x00, 0x00, 0x00,
                                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x97, 0x11,
                                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Parsed fields from a TrueHD major sync frame. outputTiming lets us detect a
// stream branch point that the plain frame-time counter cannot. Internal to
// this translation unit.
struct TrueHDMajorSyncInfo
{
  int ratebits{0};
  uint16_t outputTiming{0};
  bool outputTimingPresent{false};
  bool valid{false};
};

// Parse a TrueHD major sync frame for the sample rate bits and, when a restart
// header is present, the output timing counter used for seamless-branch
// detection. Derived from LAV Filters (Nevcairiel).
TrueHDMajorSyncInfo ParseTrueHDMajorSyncHeaders(const uint8_t* p, int buffsize)
{
  TrueHDMajorSyncInfo info;

  if (buffsize < 32)
    return {};

  // parse major sync and look for a restart header
  int majorSyncSize = 28;
  if (p[29] & 1) // restart header exists
  {
    int extensionSize = p[30] >> 4; // calculate headers size
    majorSyncSize += 2 + extensionSize * 2;
  }

  // The computed header size must fit within the provided buffer; otherwise the
  // bitstream reads below could run past the end on malformed/corrupt input.
  if (majorSyncSize > buffsize)
    return {};

  CBitstreamReader bs(p + 4, buffsize - 4);

  bs.SkipBits(32); // format_sync

  info.ratebits = bs.ReadBits(4); // ratebits
  info.valid = true;

  //  (1) 6ch_multichannel_type
  //  (1) 8ch_multichannel_type
  //  (2) reserved
  //  (2) 2ch_presentation_channel_modifier
  //  (2) 6ch_presentation_channel_modifier
  //  (5) 6ch_presentation_channel_assignment
  //  (2) 8ch_presentation_channel_modifier
  // (13) 8ch_presentation_channel_assignment
  // (16) signature
  // (16) flags
  // (16) reserved
  //  (1) variable_rate
  // (15) peak_data_rate
  bs.SkipBits(1 + 1 + 2 + 2 + 2 + 5 + 2 + 13 + 16 + 16 + 16 + 1 + 15);

  const int numSubstreams = bs.ReadBits(4);

  bs.SkipBits(4 + (majorSyncSize - 17) * 8);

  // substream directory
  for (int i = 0; i < numSubstreams; i++)
  {
    int extraSubstreamWord = bs.ReadBits(1);
    //  (1) restart_nonexistent
    //  (1) crc_present
    //  (1) reserved
    // (12) substream_end_ptr
    bs.SkipBits(15);
    if (extraSubstreamWord)
      bs.SkipBits(16); // drc_gain_update, drc_time_update, reserved
  }

  // substream segments
  for (int i = 0; i < numSubstreams; i++)
  {
    if (bs.ReadBits(1))
    { // block_header_exists
      if (bs.ReadBits(1))
      { // restart_header_exists
        bs.SkipBits(14); // restart_sync_word
        info.outputTiming = bs.ReadBits(16);
        info.outputTimingPresent = true;
        // XXX: restart header
      }
      // XXX: Block header
    }
    // XXX: All blocks, all substreams?
    break;
  }

  return info;
}
} // namespace

CPackerMAT::CPackerMAT()
{
  m_buffer.reserve(MAT_BUFFER_SIZE);
}

// On a high level, a MAT frame consists of a sequence of padded TrueHD frames
// The size of the padded frame can be determined from the frame time/sequence code in the frame header,
// since it varies to accommodate spikes in bitrate.
// In average all frames are always padded to 2560 bytes, so that 24 frames fit in one MAT frame, however
// due to bitrate spikes single sync frames have been observed to use up to twice that size, in which
// case they'll be preceded by smaller frames to keep the average bitrate constant.
// A constant padding to 2560 bytes can work (this is how the ffmpeg spdifenc module works), however
// high-bitrate streams can overshoot this size and therefor require proper handling of dynamic padding.
bool CPackerMAT::PackTrueHD(const uint8_t* data, int size)
{
  // discard too small packets (cannot be valid)
  if (size < 10)
    return false;

  const bool isMajorSync = (AV_RB32(data + 4) == FORMAT_MAJOR_SYNC);

  // Parse the major sync headers to obtain the sample rate bits and the output
  // timing counter, which is needed to detect seamless branch points.
  TrueHDMajorSyncInfo info;

  if (isMajorSync)
  {
    info = ParseTrueHDMajorSyncHeaders(data, size);

    // Fall back to master's ratebits read when the full header parse fails on a
    // short/unusual major-sync frame, so the frame is still packed (without
    // seamless-branch detection) instead of being dropped.
    m_state.ratebits = info.valid ? info.ratebits : (data[8] >> 4);
  }
  else if (!m_state.prevFrametimeValid)
  {
    // only start streaming on a major sync frame
    return false;
  }

  const uint16_t frameTime = AV_RB16(data + 2);
  uint32_t spaceSize = 0;
  const uint16_t frameSamples = 40 << (m_state.ratebits & 7);

  // Advance the output timing counter and, if the parsed output timing does not
  // match, treat it as a seamless branch point: carry the padding forward
  // instead of dropping frames.
  m_state.outputTiming += frameSamples;

  if (info.outputTimingPresent)
  {
    if (m_state.outputTimingValid && (info.outputTiming != m_state.outputTiming))
    {
      CLog::Log(LOGDEBUG,
                "CPackerMAT::PackTrueHD: detected stream discontinuity "
                "(seamless branch), expected outputTiming={}, actual={}",
                m_state.outputTiming, info.outputTiming);

      // Reset frame timing state and use default padding.
      // NOTE: do NOT reset prevMatFramesize - it is kept for proper padding calculation.
      m_state.prevFrametimeValid = false;
      // Standard padding for one MAT slot: frameSamples * (64 >> ratebits) = 2560
      // bytes at every rate (frameSamples is 40/80/160 for 48/96/192 kHz; a bare
      // 40 would only yield 2560 at 48 kHz and under-pad higher rates).
      spaceSize = frameSamples * (64 >> (m_state.ratebits & 7));

      // Calculate and carry forward padding based on the output time offset. The
      // output timing is always one frame ahead for buffering reasons, so deduct
      // one frame worth.
      uint32_t prevOutput = static_cast<uint16_t>(info.outputTiming - frameSamples);
      if (prevOutput < frameTime) // wrap around, output is always in front of frame time
        prevOutput += 0x10000u; // 2^16, not UINT16_MAX (65535) - 16-bit counter wraps at 65536

      // Get the offset of this frame, so we can compare to the previous frame,
      // and determine the amount of padding that needs to be inserted.
      int32_t currentFrameOutputOffset = static_cast<int32_t>(prevOutput - frameTime);

      // The previous offset should never be smaller than the incoming offset,
      // or we will lack the reserved space.
      if (m_state.nOutputTimeOffset >= currentFrameOutputOffset)
        m_state.padding +=
            (m_state.nOutputTimeOffset - currentFrameOutputOffset) * (64 >> (m_state.ratebits & 7));

      CLog::Log(LOGDEBUG, "CPackerMAT::PackTrueHD: carrying forward {} padding (offset {} - {})",
                m_state.padding, m_state.nOutputTimeOffset, currentFrameOutputOffset);
    }
    m_state.outputTiming = info.outputTiming;
    m_state.outputTimingValid = true;
  }

  // compute final padded size for the previous frame, if any
  if (m_state.prevFrametimeValid)
    spaceSize = uint16_t(frameTime - m_state.prevFrametime) * (64 >> (m_state.ratebits & 7));

  // compute padding (ie. difference to the size of the previous frame)
  assert(!m_state.prevFrametimeValid || spaceSize >= m_state.prevMatFramesize);

  // if for some reason the spaceSize fails, align the actual frame size
  if (spaceSize < m_state.prevMatFramesize)
    spaceSize = FFALIGN(m_state.prevMatFramesize, (64 >> (m_state.ratebits & 7)));

  m_state.padding += (spaceSize - m_state.prevMatFramesize);

  // Seek/corruption safety net: negative padding is expected (branch-point
  // carry-forward), but an unbounded large POSITIVE padding - e.g. a wild
  // frameTime jump on a hard seek or corrupt stream - would spin the
  // while(padding > 0) loop below emitting a huge number of MAT frames.
  if (m_state.padding > MAT_BUFFER_SIZE * 5)
  {
    CLog::Log(LOGINFO, "CPackerMAT::PackTrueHD: seek detected, re-initializing MAT packer state");
    m_state = {};
    m_state.init = true;
    m_buffer.clear();
    m_bufferCount = 0;
    return false;
  }

  // Record the offset of frame time to output time, used to size the padding
  // carry-forward on the next discontinuity.
  if (m_state.outputTimingValid)
  {
    uint32_t prevOutput = static_cast<uint16_t>(m_state.outputTiming - frameSamples);
    if (prevOutput < frameTime) // wrap around, output is always in front of frame time
      prevOutput += 0x10000u; // 2^16, not UINT16_MAX (65535) - 16-bit counter wraps at 65536

    m_state.nOutputTimeOffset = static_cast<int32_t>(prevOutput - frameTime);
  }

  // store frame time of the previous frame
  m_state.prevFrametime = frameTime;
  m_state.prevFrametimeValid = true;

  // Write the MAT header into the fresh buffer
  if (GetCount() == 0)
  {
    WriteHeader();

    // initial header, don't count it for the frame size
    if (m_state.init == false)
    {
      m_state.init = true;
      m_state.matFramesize = 0;
    }
  }

  // write padding of the previous frame (if any)
  while (m_state.padding > 0)
  {
    WritePadding();

    assert(m_state.padding == 0 || GetCount() == MAT_BUFFER_SIZE);

    // Buffer is full, submit it
    if (GetCount() == MAT_BUFFER_SIZE)
    {
      FlushPacket();

      // and setup a new buffer
      WriteHeader();
    }
  }

  // write actual audio data to the buffer
  int remaining = FillDataBuffer(data, size, Type::DATA);

  // not all data could be written, or the buffer is full
  if (remaining || GetCount() == MAT_BUFFER_SIZE)
  {
    // flush out old data
    FlushPacket();

    if (remaining)
    {
      // setup a new buffer
      WriteHeader();

      // and write the remaining data
      remaining = FillDataBuffer(data + (size - remaining), remaining, Type::DATA);

      assert(remaining == 0);
    }
  }

  // store the size of the current MAT frame, so we can add padding later
  m_state.prevMatFramesize = m_state.matFramesize;
  m_state.matFramesize = 0;

  // return true if have MAT packet
  return !m_outputQueue.empty();
}

std::vector<uint8_t> CPackerMAT::GetOutputFrame()
{
  if (m_outputQueue.empty())
    return {};

  std::vector<uint8_t> buffer = std::move(m_outputQueue.front());
  m_outputQueue.pop_front();

  return buffer;
}

void CPackerMAT::WriteHeader()
{
  m_buffer.resize(MAT_BUFFER_SIZE);

  // reserve size for the IEC header and the MAT start code
  const size_t size = BURST_HEADER_SIZE + mat_start_code.size();

  // write MAT start code. IEC header written later, skip space only
  memcpy(m_buffer.data() + BURST_HEADER_SIZE, mat_start_code.data(), mat_start_code.size());
  m_bufferCount = size;

  // unless the start code falls into the padding, it's considered part of the current MAT frame
  // Note that audio frames are not always aligned with MAT frames, so we might already have a partial
  // frame at this point
  m_state.matFramesize += size;

  // The MAT metadata counts as padding, if we're scheduled to write any, which mean the start bytes
  // should reduce any further padding.
  if (m_state.padding > 0)
  {
    // if the header fits into the padding of the last frame, just reduce the amount of needed padding
    if (m_state.padding > size)
    {
      m_state.padding -= size;
      m_state.matFramesize = 0;
    }
    else
    {
      // otherwise, consume all padding and set the size of the next MAT frame to the remaining data
      m_state.matFramesize = (size - m_state.padding);
      m_state.padding = 0;
    }
  }
}

void CPackerMAT::WritePadding()
{
  if (m_state.padding == 0)
    return;

  // for padding not writes any data (nullptr) as buffer is already zeroed
  // only counts/skip bytes
  const int remaining = FillDataBuffer(nullptr, m_state.padding, Type::PADDING);

  // not all padding could be written to the buffer, write it later
  if (remaining >= 0)
  {
    m_state.padding = remaining;
    m_state.matFramesize = 0;
  }
  else
  {
    // more padding then requested was written, eg. there was a MAT middle/end marker
    // that needed to be written
    m_state.padding = 0;
    m_state.matFramesize = -remaining;
  }
}

void CPackerMAT::AppendData(const uint8_t* data, int size, Type type)
{
  // for padding not write anything, only skip bytes
  if (type == Type::DATA)
    memcpy(m_buffer.data() + m_bufferCount, data, size);

  m_state.matFramesize += size;
  m_bufferCount += size;
}

int CPackerMAT::FillDataBuffer(const uint8_t* data, int size, Type type)
{
  if (GetCount() >= MAT_BUFFER_LIMIT)
    return size;

  int remaining = size;

  // Write MAT middle marker, if needed
  // The MAT middle marker always needs to be in the exact same spot, any audio data will be split.
  // If we're currently writing padding, then the marker will be considered as padding data and
  // reduce the amount of padding still required.
  if (GetCount() <= MAT_POS_MIDDLE && GetCount() + size > MAT_POS_MIDDLE)
  {
    // write as much data before the middle code as we can
    int nBytesBefore = MAT_POS_MIDDLE - GetCount();
    AppendData(data, nBytesBefore, type);
    remaining -= nBytesBefore;

    // write the MAT middle code
    AppendData(mat_middle_code.data(), mat_middle_code.size(), Type::DATA);

    // if we're writing padding, deduct the size of the code from it
    if (type == Type::PADDING)
      remaining -= mat_middle_code.size();

    // write remaining data after the MAT marker
    // Guard nullptr: for padding, data is nullptr - pointer arithmetic on nullptr
    // is undefined behavior, so pass it through unchanged.
    if (remaining > 0)
      remaining = FillDataBuffer(data ? data + nBytesBefore : nullptr, remaining, type);

    return remaining;
  }

  // not enough room in the buffer to write all the data,
  // write as much as we can and add the MAT footer
  if (GetCount() + size >= MAT_BUFFER_LIMIT)
  {
    // write as much data before the middle code as we can
    int nBytesBefore = MAT_BUFFER_LIMIT - GetCount();
    AppendData(data, nBytesBefore, type);
    remaining -= nBytesBefore;

    // write the MAT end code
    AppendData(mat_end_code.data(), mat_end_code.size(), Type::DATA);

    assert(GetCount() == MAT_BUFFER_SIZE);

    // MAT markers don't displace padding, so reduce the amount of padding
    if (type == Type::PADDING)
      remaining -= mat_end_code.size();

    // any remaining data will be written in future calls
    return remaining;
  }

  AppendData(data, size, type);

  return 0;
}

void CPackerMAT::FlushPacket()
{
  if (GetCount() == 0)
    return;

  assert(GetCount() == MAT_BUFFER_SIZE);

  // push MAT packet to output queue
  m_outputQueue.emplace_back(std::move(m_buffer));

  m_buffer.clear();
  m_bufferCount = 0;
}

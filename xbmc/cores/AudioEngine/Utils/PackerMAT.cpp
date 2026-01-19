/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PackerMAT.h"

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
} // namespace

CPackerMAT::CPackerMAT()
{
  m_buffer.reserve(MAT_BUFFER_SIZE);
}

void CPackerMAT::Reset()
{
  m_state = {};
  m_buffer.clear();
  m_bufferCount = 0;
  m_outputQueue.clear();
  m_offsetQueue.clear();
  m_discontinuityQueue.clear();
  m_lastOutputSamplesOffset = 0;
  m_lastOutputHadDiscontinuity = false;
  m_pendingDiscontinuity = false;
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
  TrueHDMajorSyncInfo info;
  bool isMajorSync = (AV_RB32(data + 4) == FORMAT_MAJOR_SYNC);

  // get the ratebits and output timing from the sync frame
  if (isMajorSync)
  {
    info = ParseTrueHDMajorSyncHeaders(data, size);

    if (!info.valid)
      return false;

    m_state.ratebits = info.ratebits;
  }
  else if (m_state.prevFrametimeValid == false)
  {
    // only start streaming on a major sync frame
    m_state.numberOfSamplesOffset = 0;
    return false;
  }

  const uint16_t frameTime = AV_RB16(data + 2);
  uint32_t spaceSize = 0;
  const uint16_t frameSamples = 40 << (m_state.ratebits & 7);
  m_state.outputTiming += frameSamples;

  // Detect stream discontinuity via outputTiming mismatch (seamless branch points).
  // LAV Filters approach: detect discontinuity BEFORE padding calculation and
  // calculate proper padding carry-forward to prevent dropped frames.
  // Only active when m_lavStyleEnabled = true.
  if (info.outputTimingPresent)
  {
    if (m_lavStyleEnabled && m_state.outputTimingValid && (info.outputTiming != m_state.outputTiming))
    {
      CLog::Log(LOGDEBUG, "CPackerMAT::PackTrueHD: detected stream discontinuity "
                "(seamless branch), expected outputTiming={}, actual={}",
                m_state.outputTiming, info.outputTiming);

      // Reset frame timing state and use default padding (like LAV Filters)
      // NOTE: Do NOT reset prevMatFramesize - LAV keeps it for proper padding calculation
      m_state.prevFrametimeValid = false;
      // Standard padding: 40 samples * (64 >> ratebits) bytes = 2560 bytes for 48kHz
      spaceSize = 40 * (64 >> (m_state.ratebits & 7));

      // LAV fix: Calculate and carry forward padding based on output time offset
      // The output timing is always one frame ahead for buffering reasons, so deduct one frame worth
      uint32_t prevOutput = static_cast<uint16_t>(info.outputTiming - frameSamples);
      if (prevOutput < frameTime) // wrap around, output is always in front of frame time
        prevOutput += UINT16_MAX;

      // Get the offset of this frame, so we can compare to the previous frame,
      // and determine the amount of padding that needs to be inserted
      int32_t currentFrameOutputOffset = static_cast<int32_t>(prevOutput - frameTime);

      // The previous offset should never be smaller than the incoming offset,
      // or we will lack the reserved space
      if (m_state.nOutputTimeOffset >= currentFrameOutputOffset)
        m_state.padding += (m_state.nOutputTimeOffset - currentFrameOutputOffset) * static_cast<int32_t>(64 >> (m_state.ratebits & 7));

      CLog::Log(LOGDEBUG, "CPackerMAT::PackTrueHD: carrying forward {} padding (offset {} - {})",
                m_state.padding, m_state.nOutputTimeOffset, currentFrameOutputOffset);

      // Mark discontinuity to propagate to output (LAV discontinuity flag)
      m_pendingDiscontinuity = true;
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

  m_state.padding += static_cast<int32_t>(spaceSize - m_state.prevMatFramesize);

  if (m_lavStyleEnabled)
  {
    // LAV Filters has no overflow safety net - it trusts the early discontinuity detection.
    // If padding goes negative, WritePadding() will simply skip (padding <= 0 check).
    // If padding is excessively large, it will be consumed over multiple MAT frames.
    // We only clamp negative padding to 0 to prevent issues in WritePadding loop.
    if (m_state.padding < 0)
      m_state.padding = 0;
  }
  else
  {
    // Baseline (non-LAV): detect seeks and re-initialize internal state
    // i.e. skip stream until the next major sync frame
    if (m_state.padding > MAT_BUFFER_SIZE * 5)
    {
      CLog::Log(LOGDEBUG, "CPackerMAT::PackTrueHD: seek detected, re-initializing MAT packer state");
      m_state = {};
      m_state.init = true;
      m_buffer.clear();
      m_bufferCount = 0;
      return false;
    }
  }

  // LAV: Record the offset of frame time to output time, which is used to verify
  // the size of the padding on discontinuities
  if (m_lavStyleEnabled && m_state.outputTimingValid)
  {
    uint32_t prevOutput = static_cast<uint16_t>(m_state.outputTiming - frameSamples);
    if (prevOutput < frameTime) // wrap around, output is always in front of frame time
      prevOutput += UINT16_MAX;

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

  // count the number of samples in this frame
  m_state.samples += frameSamples;

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

  // LAV: Store the samples offset and discontinuity flag for this frame
  // (caller can retrieve via GetSamplesOffset() and HadDiscontinuity())
  // Only when m_lavStyleEnabled = true
  if (m_lavStyleEnabled)
  {
    if (!m_offsetQueue.empty())
    {
      m_lastOutputSamplesOffset = m_offsetQueue.front();
      m_offsetQueue.pop_front();
    }
    else
    {
      m_lastOutputSamplesOffset = 0;
    }

    if (!m_discontinuityQueue.empty())
    {
      m_lastOutputHadDiscontinuity = m_discontinuityQueue.front();
      m_discontinuityQueue.pop_front();
    }
    else
    {
      m_lastOutputHadDiscontinuity = false;
    }
  }
  else
  {
    // Standard mode: Clear any queued tracking data and return defaults
    m_lastOutputSamplesOffset = 0;
    m_lastOutputHadDiscontinuity = false;
    m_offsetQueue.clear();
    m_discontinuityQueue.clear();
  }

  return buffer;
}

void CPackerMAT::WriteHeader()
{
  m_buffer.resize(MAT_BUFFER_SIZE);

  // reserve size for the IEC header and the MAT start code
  const size_t size = BURST_HEADER_SIZE + mat_start_code.size();

  // write MAT start code. IEC header written later, skip space only
  memcpy(m_buffer.data() + BURST_HEADER_SIZE, mat_start_code.data(), mat_start_code.size());
  m_bufferCount = static_cast<uint32_t>(size);

  // unless the start code falls into the padding, it's considered part of the current MAT frame
  // Note that audio frames are not always aligned with MAT frames, so we might already have a partial
  // frame at this point
  m_state.matFramesize += static_cast<uint32_t>(size);

  // The MAT metadata counts as padding, if we're scheduled to write any, which mean the start bytes
  // should reduce any further padding.
  if (m_state.padding > 0)
  {
    // if the header fits into the padding of the last frame, just reduce the amount of needed padding
    if (m_state.padding > static_cast<int32_t>(size))
    {
      m_state.padding -= static_cast<int32_t>(size);
      m_state.matFramesize = 0;
    }
    else
    {
      // otherwise, consume all padding and set the size of the next MAT frame to the remaining data
      m_state.matFramesize = static_cast<uint32_t>(static_cast<int32_t>(size) - m_state.padding);
      m_state.padding = 0;
    }
  }
}

void CPackerMAT::WritePadding()
{
  if (m_state.padding <= 0)
    return;

  // for padding not writes any data (nullptr) as buffer is already zeroed
  // only counts/skip bytes
  const int remaining = FillDataBuffer(nullptr, static_cast<int>(m_state.padding), Type::PADDING);

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
    m_state.matFramesize = static_cast<uint32_t>(-remaining);
  }
}

void CPackerMAT::AppendData(const uint8_t* data, int size, Type type)
{
  // for padding not write anything, only skip bytes
  if (type == Type::DATA)
    memcpy(m_buffer.data() + m_bufferCount, data, size);

  m_state.matFramesize += static_cast<uint32_t>(size);
  m_bufferCount += static_cast<uint32_t>(size);
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
  if (GetCount() <= MAT_POS_MIDDLE && GetCount() + static_cast<uint32_t>(size) > MAT_POS_MIDDLE)
  {
    // write as much data before the middle code as we can
    int nBytesBefore = static_cast<int>(MAT_POS_MIDDLE - GetCount());
    AppendData(data, nBytesBefore, type);
    remaining -= nBytesBefore;

    // write the MAT middle code
    AppendData(mat_middle_code.data(), static_cast<int>(mat_middle_code.size()), Type::DATA);

    // if we're writing padding, deduct the size of the code from it
    if (type == Type::PADDING)
      remaining -= static_cast<int>(mat_middle_code.size());

    // write remaining data after the MAT marker
    if (remaining > 0)
      remaining = FillDataBuffer(data + nBytesBefore, remaining, type);

    return remaining;
  }

  // not enough room in the buffer to write all the data,
  // write as much as we can and add the MAT footer
  if (GetCount() + static_cast<uint32_t>(size) >= MAT_BUFFER_LIMIT)
  {
    // write as much data before the middle code as we can
    int nBytesBefore = static_cast<int>(MAT_BUFFER_LIMIT - GetCount());
    AppendData(data, nBytesBefore, type);
    remaining -= nBytesBefore;

    // write the MAT end code
    AppendData(mat_end_code.data(), static_cast<int>(mat_end_code.size()), Type::DATA);

    assert(GetCount() == MAT_BUFFER_SIZE);

    // MAT markers don't displace padding, so reduce the amount of padding
    if (type == Type::PADDING)
      remaining -= static_cast<int>(mat_end_code.size());

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

  // normal number of samples per frame
  const uint16_t frameSamples = 40 << (m_state.ratebits & 7);
  const uint32_t MATSamples = (frameSamples * 24);

  // push MAT packet to output queue
  m_outputQueue.emplace_back(std::move(m_buffer));
  
  // LAV: also queue samples offset and discontinuity flag
  // (like LAV Filters, the offset is captured BEFORE updating, so it applies to this frame)
  if (m_lavStyleEnabled)
  {
    m_offsetQueue.push_back(m_state.numberOfSamplesOffset);
    m_discontinuityQueue.push_back(m_pendingDiscontinuity);
    m_pendingDiscontinuity = false; // Clear after queuing

    // we expect 24 frames per MAT frame, so calculate an offset from that
    // this is done after delivery, because it modifies the duration of the frame,
    //  eg. the start of the next frame
    if (MATSamples != m_state.samples)
      m_state.numberOfSamplesOffset += static_cast<int32_t>(m_state.samples) - static_cast<int32_t>(MATSamples);
  }

  m_state.samples = 0;

  m_buffer.clear();
  m_bufferCount = 0;
}

TrueHDMajorSyncInfo CPackerMAT::ParseTrueHDMajorSyncHeaders(const uint8_t* p, int buffsize) const
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

  CBitStream bs(p + 4, buffsize - 4);

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

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

  // get the ratebits from the major sync frame
  if (AV_RB32(data + 4) == FORMAT_MAJOR_SYNC)
  {
    // read audio_sampling_frequency (high nibble after format major sync)
    m_state.ratebits = data[8] >> 4;
  }
  else if (!m_state.prevFrametimeValid)
  {
    // only start streaming on a major sync frame
    return false;
  }

  const uint16_t frameTime = AV_RB16(data + 2);
  uint32_t spaceSize = 0;

  // compute final padded size for the previous frame, if any
  if (m_state.prevFrametimeValid)
    spaceSize = uint16_t(frameTime - m_state.prevFrametime) * (64 >> (m_state.ratebits & 7));

  // compute padding (ie. difference to the size of the previous frame)
  assert(!m_state.prevFrametimeValid || spaceSize >= m_state.prevMatFramesize);

  // if for some reason the spaceSize fails, align the actual frame size
  if (spaceSize < m_state.prevMatFramesize)
    spaceSize = FFALIGN(m_state.prevMatFramesize, (64 >> (m_state.ratebits & 7)));

  m_state.padding += (spaceSize - m_state.prevMatFramesize);

  // detect seeks and re-initialize internal state i.e. skip stream
  // until the next major sync frame
  if (m_state.padding > MAT_BUFFER_SIZE * 5)
  {
    CLog::Log(LOGINFO, "CPackerMAT::PackTrueHD: seek detected, re-initializing MAT packer state");
    m_state = {};
    m_state.init = true;
    m_buffer.clear();
    m_bufferCount = 0;
    return false;
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
  std::vector<uint8_t> buffer;

  if (m_outputQueue.empty())
    return {};

  buffer = std::move(m_outputQueue.front());

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
    if (remaining > 0)
      remaining = FillDataBuffer(data + nBytesBefore, remaining, type);

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

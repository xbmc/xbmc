/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/streams/memory/DeltaPairMemoryStream.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::RETRO;

namespace
{
class TestMemoryStream : public CDeltaPairMemoryStream
{
public:
  size_t WordCount() const { return m_paddedFrameSize; }
  size_t LastDeltaCount() const { return m_rewindBuffer.back().buffer.size(); }
};
} // namespace

class TestDeltaPairMemoryStream : public testing::Test
{
protected:
  void ExpectZeroPadding(const uint8_t* frame) const
  {
    for (size_t i = m_stream.FrameSize(); i < m_stream.WordCount() * sizeof(uint32_t); ++i)
      EXPECT_EQ(0, frame[i]) << "Byte " << i;
  }

  TestMemoryStream m_stream;
};

TEST_F(TestDeltaPairMemoryStream, SizesStorageInWords)
{
  for (const size_t stateSize : {4099, 4096})
  {
    SCOPED_TRACE(stateSize);
    m_stream.Init(stateSize, 4);

    EXPECT_EQ(stateSize, m_stream.FrameSize());
    ASSERT_EQ(stateSize == 4099 ? 1025 : 1024, m_stream.WordCount());

    uint8_t* frame = m_stream.BeginFrame();
    ASSERT_NE(nullptr, frame);
    std::fill_n(frame, stateSize, 0x5a);
    ExpectZeroPadding(frame);
    m_stream.SubmitFrame();

    frame = m_stream.BeginFrame();
    ASSERT_NE(nullptr, frame);
    std::fill_n(frame, stateSize, 0xa5);
    ExpectZeroPadding(frame);
    m_stream.SubmitFrame();
    ExpectZeroPadding(m_stream.CurrentFrame());
  }
}

TEST_F(TestDeltaPairMemoryStream, RewindsAndPreservesPaddingThroughBufferReuse)
{
  for (const size_t stateSize : {4099, 4096})
  {
    SCOPED_TRACE(stateSize);
    m_stream.Init(stateSize, 4);

    std::vector<uint8_t> first(stateSize);
    for (size_t i = 0; i < stateSize; ++i)
      first[i] = static_cast<uint8_t>(i);
    std::vector<uint8_t> second = first;
    second.back() ^= 0xff;

    for (const auto* state : {&first, &second, &second, &first})
    {
      uint8_t* frame = m_stream.BeginFrame();
      ASSERT_NE(nullptr, frame);
      std::copy(state->begin(), state->end(), frame);
      ExpectZeroPadding(frame);
      m_stream.SubmitFrame();
      EXPECT_EQ(0, std::memcmp(state->data(), m_stream.CurrentFrame(), stateSize));
      ExpectZeroPadding(m_stream.CurrentFrame());
      if (m_stream.PastFramesAvailable() > 0)
      {
        EXPECT_EQ(m_stream.PastFramesAvailable() == 2 ? 0 : 1, m_stream.LastDeltaCount());
      }
    }

    for (const auto* state : {&second, &second, &first})
    {
      ASSERT_EQ(1, m_stream.RewindFrames(1));
      EXPECT_EQ(0, std::memcmp(state->data(), m_stream.CurrentFrame(), stateSize));
      ExpectZeroPadding(m_stream.CurrentFrame());
    }

    uint8_t* frame = m_stream.BeginFrame();
    ASSERT_NE(nullptr, frame);
    std::copy(first.begin(), first.end(), frame);
    ExpectZeroPadding(frame);
    m_stream.SubmitFrame();
    EXPECT_EQ(0, m_stream.LastDeltaCount());
    ASSERT_EQ(1, m_stream.RewindFrames(1));
    EXPECT_EQ(0, std::memcmp(first.data(), m_stream.CurrentFrame(), stateSize));
    ExpectZeroPadding(m_stream.CurrentFrame());
  }
}

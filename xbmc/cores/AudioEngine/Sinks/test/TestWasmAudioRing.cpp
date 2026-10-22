/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/wasm/WasmAudioRing.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::PLATFORM::WASM;

namespace
{
constexpr unsigned int CAPACITY = 8;

// Reads the way the processor in audio_worklet_processor.js does: it trusts the
// two frame counters and keeps a ring position of its own.
class Reader
{
public:
  explicit Reader(CWasmAudioRing& ring, unsigned int channel = 0)
    : m_ring(ring),
      m_plane(ring.GetData() + channel * ring.GetCapacityFrames())
  {
  }

  std::vector<float> Read(unsigned int frames)
  {
    const uint32_t read = m_ring.GetReadFrame().load();
    const uint32_t available = m_ring.GetWriteFrame().load() - read;
    const unsigned int toRead = std::min<uint32_t>(available, frames);
    std::vector<float> out;
    for (unsigned int i = 0; i < toRead; ++i)
      out.push_back(m_plane[(m_index + i) % m_ring.GetCapacityFrames()]);
    m_index = (m_index + toRead) % m_ring.GetCapacityFrames();
    m_ring.GetReadFrame().store(read + toRead);
    return out;
  }

private:
  CWasmAudioRing& m_ring;
  const float* m_plane;
  unsigned int m_index{0};
};

// Writes frames numbered from first, with the second plane negated.
unsigned int Write(CWasmAudioRing& ring, unsigned int frames, float first)
{
  std::vector<float> left;
  std::vector<float> right;
  for (unsigned int i = 0; i < frames; ++i)
  {
    left.push_back(first + static_cast<float>(i));
    right.push_back(-left.back());
  }
  const std::array<const float*, 2> planes{left.data(), right.data()};
  return ring.Write(planes.data(), 2, frames, 0);
}

using Frames = std::vector<float>;
} // namespace

TEST(TestWasmAudioRing, WritesAcrossTheRingEnd)
{
  CWasmAudioRing ring;
  ring.Allocate(CAPACITY, 2);
  Reader left(ring);

  EXPECT_EQ(Write(ring, 6, 1.0f), 6U);
  EXPECT_EQ(left.Read(4), (Frames{1, 2, 3, 4}));
  EXPECT_EQ(Write(ring, 6, 7.0f), 6U);
  EXPECT_EQ(ring.GetBufferedFrames(), 8U);
  EXPECT_EQ(left.Read(8), (Frames{5, 6, 7, 8, 9, 10, 11, 12}));
  EXPECT_EQ(ring.GetBufferedFrames(), 0U);
  EXPECT_EQ(Write(ring, 6, 13.0f), 6U);
  EXPECT_EQ(left.Read(6), (Frames{13, 14, 15, 16, 17, 18}));
}

TEST(TestWasmAudioRing, WritesEveryPlane)
{
  CWasmAudioRing ring;
  ring.Allocate(CAPACITY, 2);
  Reader right(ring, 1);

  Write(ring, 6, 1.0f);
  right.Read(6);
  Write(ring, 4, 7.0f);
  EXPECT_EQ(right.Read(4), (Frames{-7, -8, -9, -10}));
}

TEST(TestWasmAudioRing, TakesOnlyWhatFits)
{
  CWasmAudioRing ring;
  ring.Allocate(CAPACITY, 2);
  Reader left(ring);

  EXPECT_EQ(Write(ring, 10, 1.0f), 8U);
  EXPECT_EQ(Write(ring, 1, 9.0f), 0U);
  EXPECT_EQ(left.Read(3), (Frames{1, 2, 3}));
  EXPECT_EQ(Write(ring, 5, 9.0f), 3U);
  EXPECT_EQ(left.Read(8), (Frames{4, 5, 6, 7, 8, 9, 10, 11}));
}

TEST(TestWasmAudioRing, CountersWrap)
{
  CWasmAudioRing ring;
  ring.Allocate(CAPACITY, 2);
  ring.GetReadFrame().store(0xfffffffc);
  ring.GetWriteFrame().store(0xfffffffc);
  Reader left(ring);

  EXPECT_EQ(Write(ring, 6, 1.0f), 6U);
  EXPECT_EQ(ring.GetWriteFrame().load(), 2U);
  EXPECT_EQ(ring.GetBufferedFrames(), 6U);
  EXPECT_EQ(left.Read(2), (Frames{1, 2}));
  EXPECT_EQ(ring.GetReadFrame().load(), 0xfffffffeU);
  EXPECT_EQ(Write(ring, 6, 7.0f), 4U);
  EXPECT_EQ(Write(ring, 1, 11.0f), 0U);
  EXPECT_EQ(ring.GetBufferedFrames(), 8U);
  EXPECT_EQ(left.Read(8), (Frames{3, 4, 5, 6, 7, 8, 9, 10}));
  EXPECT_EQ(ring.GetReadFrame().load(), 6U);
}

TEST(TestWasmAudioRing, ResetStartsAtTheRingStart)
{
  CWasmAudioRing ring;
  ring.Allocate(CAPACITY, 2);
  Write(ring, 5, 1.0f);
  Reader(ring).Read(3);

  ring.Reset();
  EXPECT_EQ(ring.GetBufferedFrames(), 0U);
  EXPECT_EQ(ring.GetReadFrame().load(), 0U);
  EXPECT_EQ(ring.GetWriteFrame().load(), 0U);
  EXPECT_EQ(Write(ring, 3, 20.0f), 3U);
  EXPECT_EQ(Reader(ring).Read(3), (Frames{20, 21, 22}));
}

TEST(TestWasmAudioRing, HonoursTheSourceOffset)
{
  CWasmAudioRing ring;
  ring.Allocate(CAPACITY, 1);
  const Frames source{1, 2, 3, 4, 5};
  const float* const plane = source.data();

  EXPECT_EQ(ring.Write(&plane, 1, 3, 2), 3U);
  EXPECT_EQ(Reader(ring).Read(3), (Frames{3, 4, 5}));
}

TEST(TestWasmAudioRing, ZeroesPlanesWithoutSource)
{
  CWasmAudioRing ring;
  ring.Allocate(CAPACITY, 2);
  Write(ring, 8, 1.0f);
  Reader(ring).Read(8);

  const Frames source{1, 2, 3, 4};
  const float* const plane = source.data();
  EXPECT_EQ(ring.Write(&plane, 1, 4, 0), 4U);
  EXPECT_EQ(Reader(ring, 1).Read(4), (Frames{0, 0, 0, 0}));
}

TEST(TestWasmAudioRing, HoldsNothingOnceFreed)
{
  CWasmAudioRing ring;
  ring.Allocate(CAPACITY, 2);
  Write(ring, 4, 1.0f);

  ring.Free();
  EXPECT_EQ(ring.GetCapacityFrames(), 0U);
  EXPECT_EQ(ring.GetBufferedFrames(), 0U);
  EXPECT_EQ(Write(ring, 4, 1.0f), 0U);
}

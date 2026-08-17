/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/AudioEngine/Utils/AEBitstreamPacker.h"
#include "cores/AudioEngine/Utils/AEStreamInfo.h"

#include <vector>

#include <gtest/gtest.h>

namespace
{
// An AC3 frame is wrapped rather than parsed, save for the bitstream mode in byte 5, so a frame
// of arbitrary bytes packs exactly as a real one does.
std::vector<uint8_t> MakeAc3Frame(uint8_t seed)
{
  std::vector<uint8_t> frame(64);
  for (size_t i = 0; i < frame.size(); ++i)
    frame[i] = static_cast<uint8_t>(seed + i);
  return frame;
}

CAEStreamInfo MakeAc3Info()
{
  CAEStreamInfo info;
  info.m_type = CAEStreamInfo::STREAM_TYPE_AC3;
  info.m_sampleRate = 48000;
  info.m_channels = 2;
  return info;
}

std::vector<uint8_t> Snapshot(CAEBitstreamPacker& packer)
{
  return std::vector<uint8_t>(packer.GetBuffer(), packer.GetBuffer() + packer.GetSize());
}
} // namespace

TEST(TestAEBitstreamPacker, LastBurstIsUnavailableBeforeOneIsPacked)
{
  CAEBitstreamPacker packer;

  EXPECT_FALSE(packer.PackLastBurst());
}

TEST(TestAEBitstreamPacker, LastBurstReturnsTheStreamsOwnBurstAfterAPause)
{
  CAEBitstreamPacker packer;
  CAEStreamInfo info{MakeAc3Info()};
  const std::vector<uint8_t> frame{MakeAc3Frame(0x10)};

  packer.Pack(info, const_cast<uint8_t*>(frame.data()), static_cast<int>(frame.size()));
  const std::vector<uint8_t> burst{Snapshot(packer)};
  ASSERT_FALSE(burst.empty());

  // A pause burst is what the sink would otherwise put on the wire, and reads as non-audio
  packer.PackPause(info, 100, true);
  EXPECT_NE(burst, Snapshot(packer));

  ASSERT_TRUE(packer.PackLastBurst());
  EXPECT_EQ(burst, Snapshot(packer));
}

TEST(TestAEBitstreamPacker, LastBurstIsTheMostRecentOne)
{
  CAEBitstreamPacker packer;
  CAEStreamInfo info{MakeAc3Info()};
  const std::vector<uint8_t> first{MakeAc3Frame(0x10)};
  const std::vector<uint8_t> second{MakeAc3Frame(0x40)};

  packer.Pack(info, const_cast<uint8_t*>(first.data()), static_cast<int>(first.size()));
  const std::vector<uint8_t> firstBurst{Snapshot(packer)};

  packer.Pack(info, const_cast<uint8_t*>(second.data()), static_cast<int>(second.size()));
  const std::vector<uint8_t> secondBurst{Snapshot(packer)};
  ASSERT_NE(firstBurst, secondBurst);

  packer.PackPause(info, 100, true);
  ASSERT_TRUE(packer.PackLastBurst());
  EXPECT_EQ(secondBurst, Snapshot(packer));
}

TEST(TestAEBitstreamPacker, RepeatingABurstDoesNotStrandTheNextPauseOfTheSameLength)
{
  CAEBitstreamPacker packer;
  CAEStreamInfo info{MakeAc3Info()};
  const std::vector<uint8_t> frame{MakeAc3Frame(0x10)};

  packer.Pack(info, const_cast<uint8_t*>(frame.data()), static_cast<int>(frame.size()));
  ASSERT_TRUE(packer.PackPause(info, 100, true));
  const std::vector<uint8_t> pause{Snapshot(packer)};

  ASSERT_TRUE(packer.PackLastBurst());

  // PackPause reuses the buffer when asked for the duration it last packed, so repeating a burst
  // over the top of it has to drop that record or the pause never reaches the wire again.
  ASSERT_TRUE(packer.PackPause(info, 100, true));
  EXPECT_EQ(pause, Snapshot(packer));
}

TEST(TestAEBitstreamPacker, ResetForgetsTheRetainedBurst)
{
  CAEBitstreamPacker packer;
  CAEStreamInfo info{MakeAc3Info()};
  const std::vector<uint8_t> frame{MakeAc3Frame(0x10)};

  packer.Pack(info, const_cast<uint8_t*>(frame.data()), static_cast<int>(frame.size()));
  ASSERT_TRUE(packer.PackLastBurst());

  // The burst belongs to the stream that is ending, so it must not follow the next one
  packer.Reset();
  EXPECT_FALSE(packer.PackLastBurst());
}

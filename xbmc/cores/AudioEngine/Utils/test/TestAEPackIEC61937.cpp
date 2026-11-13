/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/AudioEngine/Utils/AEPackIEC61937.h"

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

namespace
{
// The packer pads to the burst size, so give it room past MAX_IEC61937_PACKET to see an overrun.
constexpr unsigned int GUARD = 8192;
constexpr uint8_t CANARY = 0xA5;
} // namespace

TEST(TestAEPackIEC61937, ADtsHdFrameThatFitsItsBurstIsPacked)
{
  std::vector<uint8_t> frame(8000, 0x11);
  std::vector<uint8_t> dest(MAX_IEC61937_PACKET + GUARD, CANARY);

  EXPECT_EQ(8192, CAEPackIEC61937::PackDTSHD(frame.data(), frame.size(), dest.data(), 2048));
}

//! A DTS-HD MA frame at peak bit rate taken for HRA: 8320 bytes into an 8192-byte burst. The
//! padding length went negative and memset ran for about 4 GB.
TEST(TestAEPackIEC61937, ADtsHdFrameLargerThanItsBurstIsDropped)
{
  std::vector<uint8_t> frame(8320, 0x11);
  std::vector<uint8_t> dest(MAX_IEC61937_PACKET + GUARD, CANARY);

  EXPECT_EQ(0, CAEPackIEC61937::PackDTSHD(frame.data(), frame.size(), dest.data(), 2048));
  for (unsigned int i = 0; i < dest.size(); ++i)
    ASSERT_EQ(CANARY, dest[i]) << "byte " << i << " written";
}

//! A period of 16384 is a 65536-byte burst, larger than the MAX_IEC61937_PACKET destination.
TEST(TestAEPackIEC61937, ABurstLargerThanTheDestinationBufferIsDropped)
{
  std::vector<uint8_t> frame(1000, 0x11);
  std::vector<uint8_t> dest(MAX_IEC61937_PACKET + GUARD, CANARY);

  EXPECT_EQ(0, CAEPackIEC61937::PackDTSHD(frame.data(), frame.size(), dest.data(), 16384));
  for (unsigned int i = MAX_IEC61937_PACKET; i < dest.size(); ++i)
    ASSERT_EQ(CANARY, dest[i]) << "byte " << i << " written past the destination buffer";
}

/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/AudioEngine/Utils/PackerMAT.h"
#include "utils/Crc32.h"

#include <array>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

namespace
{
constexpr int FRAME_SIZE = 64;
constexpr uint16_t FRAME_SAMPLES = 40; // 48 kHz

// A TrueHD access unit. A major sync carries one substream with a restart header, whose output
// timing the packer compares with its own count to detect a seamless branch.
std::array<uint8_t, FRAME_SIZE> MakeFrame(uint16_t frameTime, bool majorSync, uint16_t outputTiming)
{
  std::array<uint8_t, FRAME_SIZE> frame{};
  frame[0] = 0xF0;
  frame[1] = FRAME_SIZE / 2;
  frame[2] = static_cast<uint8_t>(frameTime >> 8);
  frame[3] = static_cast<uint8_t>(frameTime & 0xFF);
  if (majorSync)
  {
    frame[4] = 0xF8; // format_sync
    frame[5] = 0x72;
    frame[6] = 0x6F;
    frame[7] = 0xBA;
    frame[8] = 0x00; // ratebits 0: 48 kHz
    frame[20] = 0x10; // one substream
    frame[34] = 0xC0; // block header and restart header present
    frame[36] = static_cast<uint8_t>(outputTiming >> 8);
    frame[37] = static_cast<uint8_t>(outputTiming & 0xFF);
  }
  return frame;
}
} // namespace

TEST(TestPackerMAT, PackTrueHD)
{
  constexpr uint16_t FIRST_OUTPUT_TIMING = 0x0100;
  // Three frames ahead of the count the packer keeps, so it is treated as a branch point
  constexpr uint16_t BRANCH_OUTPUT_TIMING =
      FIRST_OUTPUT_TIMING + 16 * FRAME_SAMPLES + 3 * FRAME_SAMPLES;

  CPackerMAT packer;
  Crc32 crc;
  unsigned int frames{0};
  size_t bytes{0};
  std::vector<double> outputPts;
  std::vector<double> outputOriginalPts;
  constexpr double FIRST_PTS = 100000.0;
  constexpr double FIRST_ORIGINAL_PTS = 500000.0;
  constexpr double MAT_DURATION = 20000.0;

  for (uint16_t i = 0; i < 60; ++i)
  {
    const bool majorSync{i == 0 || i == 16};
    const auto frame{MakeFrame(i * FRAME_SAMPLES, majorSync,
                               i == 0 ? FIRST_OUTPUT_TIMING : BRANCH_OUTPUT_TIMING)};
    const double pts = FIRST_PTS + i * (static_cast<double>(FRAME_SAMPLES) / 48000.0) * 1000000.0;
    const double originalPts = FIRST_ORIGINAL_PTS + i * 40.0 / 48000.0 * 1000000.0;
    packer.PackTrueHD(frame.data(), FRAME_SIZE, pts, originalPts, MAT_DURATION, true, true);

    double outputPtsValue{0.0};
    double outputOriginalPtsValue{0.0};
    bool hasPts{false};
    bool hasOriginalPts{false};
    for (auto output{
             packer.GetOutputFrame(outputPtsValue, hasPts, outputOriginalPtsValue, hasOriginalPts)};
         !output.empty(); output = packer.GetOutputFrame(outputPtsValue, hasPts,
                                                         outputOriginalPtsValue, hasOriginalPts))
    {
      ++frames;
      bytes += output.size();
      ASSERT_TRUE(hasPts);
      ASSERT_TRUE(hasOriginalPts);
      outputPts.push_back(outputPtsValue);
      outputOriginalPts.push_back(outputOriginalPtsValue);
      crc.Compute(reinterpret_cast<const char*>(output.data()), output.size());
    }
  }

  // A change means the parsing of the major sync changed
  EXPECT_EQ(frames, 2u);
  EXPECT_EQ(bytes, 122880u);
  ASSERT_EQ(2u, outputPts.size());
  EXPECT_DOUBLE_EQ(FIRST_PTS, outputPts[0]);
  EXPECT_DOUBLE_EQ(FIRST_PTS + MAT_DURATION, outputPts[1]);
  ASSERT_EQ(2u, outputOriginalPts.size());
  EXPECT_DOUBLE_EQ(FIRST_ORIGINAL_PTS, outputOriginalPts[0]);
  EXPECT_DOUBLE_EQ(FIRST_ORIGINAL_PTS + MAT_DURATION, outputOriginalPts[1]);
  EXPECT_EQ(static_cast<uint32_t>(crc), 3000242471u);
}

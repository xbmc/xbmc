/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/rendering/HdrOutput.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"

#include <gtest/gtest.h>

extern "C"
{
#include <libavutil/rational.h>
}

using namespace KODI::RETRO;

TEST(TestHdrOutput, IsHdr10InBt2020)
{
  VideoPicture picture;
  GetHdrOutputPicture(picture, 200, 1000);

  EXPECT_EQ(picture.color_transfer, AVCOL_TRC_SMPTE2084);
  EXPECT_EQ(picture.color_primaries, AVCOL_PRI_BT2020);
  EXPECT_EQ(picture.color_space, AVCOL_SPC_BT2020_NCL);
  EXPECT_EQ(picture.colorBits, 10u);
  EXPECT_DOUBLE_EQ(av_q2d(picture.displayMetadata.display_primaries[1][1]), 0.797);
  EXPECT_DOUBLE_EQ(av_q2d(picture.displayMetadata.white_point[0]), 0.3127);
}

TEST(TestHdrOutput, CarriesTheBrightestLight)
{
  VideoPicture picture;
  GetHdrOutputPicture(picture, 200, 1000);

  ASSERT_TRUE(picture.hasDisplayMetadata);
  ASSERT_TRUE(picture.hasLightMetadata);
  EXPECT_DOUBLE_EQ(av_q2d(picture.displayMetadata.max_luminance), 1000.0);
  EXPECT_EQ(picture.lightMetadata.MaxCLL, 1000u);
  EXPECT_EQ(picture.lightMetadata.MaxFALL, 200u);
}

TEST(TestHdrOutput, TakesThePeakFromTheDisplay)
{
  EXPECT_EQ(GetHdrPeak(600.4f), 600u);
  EXPECT_EQ(GetHdrPeak(1499.6f), 1500u);
}

TEST(TestHdrOutput, AssumesAPeakTheDisplayDoesNotGive)
{
  EXPECT_EQ(GetHdrPeak(0.0f), HDR_DEFAULT_PEAK);
}

TEST(TestHdrOutput, KeepsThePeakAboveWhite)
{
  EXPECT_EQ(GetHdrPeak(100.0f), HDR_REFERENCE_WHITE);
}

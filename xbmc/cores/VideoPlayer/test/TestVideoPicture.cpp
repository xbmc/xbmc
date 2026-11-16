/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"

#include <gtest/gtest.h>

namespace
{

void Describe(VideoPicture& picture, unsigned int width, unsigned int height)
{
  picture.Reset();
  picture.iWidth = width;
  picture.iHeight = height;
  picture.iDisplayWidth = width;
  picture.iDisplayHeight = height;
  picture.color_primaries = AVCOL_PRI_BT709;
  picture.color_transfer = AVCOL_TRC_BT709;
}

class TestVideoPicture : public testing::Test
{
protected:
  void SetUp() override
  {
    Describe(m_full, 3840, 2160);
    Describe(m_half, 1920, 1080);
  }

  VideoPicture m_full;
  VideoPicture m_half;
};

} // namespace

TEST_F(TestVideoPicture, AnotherSizeIsTheOnlyDifference)
{
  EXPECT_FALSE(m_full.IsSameParams(m_half));
  EXPECT_TRUE(m_full.IsSameParamsExceptSize(m_half));
}

TEST_F(TestVideoPicture, AnotherDisplaySizeIsTheOnlyDifference)
{
  Describe(m_half, 3840, 2160);
  m_half.iDisplayWidth = 5120;

  EXPECT_FALSE(m_full.IsSameParams(m_half));
  EXPECT_TRUE(m_full.IsSameParamsExceptSize(m_half));
}

TEST_F(TestVideoPicture, TheSamePictureDiffersInNothing)
{
  Describe(m_half, 3840, 2160);

  EXPECT_TRUE(m_full.IsSameParams(m_half));
  EXPECT_TRUE(m_full.IsSameParamsExceptSize(m_half));
}

TEST_F(TestVideoPicture, AnotherColourDescriptionIsMoreThanAnotherSize)
{
  m_half.color_primaries = AVCOL_PRI_BT2020;
  EXPECT_FALSE(m_full.IsSameParamsExceptSize(m_half));

  Describe(m_half, 1920, 1080);
  m_half.color_transfer = AVCOL_TRC_SMPTE2084;
  EXPECT_FALSE(m_full.IsSameParamsExceptSize(m_half));

  Describe(m_half, 1920, 1080);
  m_half.hdrType = StreamHdrType::HDR_TYPE_HDR10;
  EXPECT_FALSE(m_full.IsSameParamsExceptSize(m_half));
}

TEST_F(TestVideoPicture, AnotherMasteringDisplayIsMoreThanAnotherSize)
{
  m_half.hasDisplayMetadata = true;
  EXPECT_FALSE(m_full.IsSameParamsExceptSize(m_half));

  m_full.hasDisplayMetadata = true;
  m_full.displayMetadata.max_luminance = {1000, 1};
  m_full.displayMetadata.min_luminance = {1, 10000};
  m_half.displayMetadata.max_luminance = {4000, 1};
  m_half.displayMetadata.min_luminance = {1, 10000};
  EXPECT_FALSE(m_full.IsSameParamsExceptSize(m_half));
}

TEST_F(TestVideoPicture, AnotherStereoModeIsMoreThanAnotherSize)
{
  m_half.stereoMode = "left_right";
  EXPECT_FALSE(m_full.IsSameParamsExceptSize(m_half));
}

/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "input/touch/VideoGesture.h"

#include <limits>

#include <gtest/gtest.h>

using namespace KODI::TOUCH;

TEST(TestVideoGesture, DoubleTapSeeksTenSecondsInTheTappedHalf)
{
  EXPECT_EQ(GetVideoDoubleTapSeekSeconds(0, 1024), -10);
  EXPECT_EQ(GetVideoDoubleTapSeekSeconds(511, 1024), -10);
  EXPECT_EQ(GetVideoDoubleTapSeekSeconds(512, 1024), 10);
  EXPECT_EQ(GetVideoDoubleTapSeekSeconds(1024, 1024), 10);
}

TEST(TestVideoGesture, DoubleTapIgnoresInvalidPositions)
{
  EXPECT_EQ(GetVideoDoubleTapSeekSeconds(-1, 1024), 0);
  EXPECT_EQ(GetVideoDoubleTapSeekSeconds(1025, 1024), 0);
  EXPECT_EQ(GetVideoDoubleTapSeekSeconds(0, 0), 0);
  EXPECT_EQ(GetVideoDoubleTapSeekSeconds(0, -1), 0);
  EXPECT_EQ(GetVideoDoubleTapSeekSeconds(std::numeric_limits<double>::quiet_NaN(), 1024), 0);
  EXPECT_EQ(GetVideoDoubleTapSeekSeconds(0, std::numeric_limits<double>::infinity()), 0);
}

TEST(TestVideoGesture, PinchChangesModeAfterTenPercentMovement)
{
  EXPECT_EQ(GetVideoPinchMode(1), VideoPinchMode::NONE);
  EXPECT_EQ(GetVideoPinchMode(1.09), VideoPinchMode::NONE);
  EXPECT_EQ(GetVideoPinchMode(1.10), VideoPinchMode::ZOOM);
  EXPECT_EQ(GetVideoPinchMode(0.91), VideoPinchMode::NONE);
  EXPECT_EQ(GetVideoPinchMode(0.90), VideoPinchMode::NORMAL);
}

TEST(TestVideoGesture, PinchIgnoresInvalidScales)
{
  EXPECT_EQ(GetVideoPinchMode(0), VideoPinchMode::NONE);
  EXPECT_EQ(GetVideoPinchMode(-1), VideoPinchMode::NONE);
  EXPECT_EQ(GetVideoPinchMode(std::numeric_limits<double>::quiet_NaN()), VideoPinchMode::NONE);
  EXPECT_EQ(GetVideoPinchMode(std::numeric_limits<double>::infinity()), VideoPinchMode::NONE);
  EXPECT_EQ(GetVideoPinchMode(-std::numeric_limits<double>::infinity()), VideoPinchMode::NONE);
}

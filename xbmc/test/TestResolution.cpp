/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "windowing/Resolution.h"

#include <gtest/gtest.h>

TEST(TestResolution, CompatibleRefreshRateMultiples)
{
  constexpr float FPS_23_976 = 24000.0f / 1001.0f;
  constexpr float FPS_29_97 = 30000.0f / 1001.0f;
  constexpr float FPS_59_94 = 60000.0f / 1001.0f;
  constexpr float REFRESH_119_88 = 120000.0f / 1001.0f;

  EXPECT_TRUE(CResolutionUtils::IsRefreshRateMultiple(REFRESH_119_88, FPS_23_976));
  EXPECT_TRUE(CResolutionUtils::IsRefreshRateMultiple(REFRESH_119_88, FPS_29_97));
  EXPECT_TRUE(CResolutionUtils::IsRefreshRateMultiple(REFRESH_119_88, FPS_59_94));
  EXPECT_TRUE(CResolutionUtils::IsRefreshRateMultiple(120.0f, 24.0f));
  EXPECT_TRUE(CResolutionUtils::IsRefreshRateMultiple(100.0f, 25.0f));
  EXPECT_TRUE(CResolutionUtils::IsRefreshRateMultiple(FPS_23_976, FPS_23_976));

  EXPECT_FALSE(CResolutionUtils::IsRefreshRateMultiple(120.0f, FPS_23_976));
  EXPECT_FALSE(CResolutionUtils::IsRefreshRateMultiple(REFRESH_119_88, 24.0f));
  EXPECT_FALSE(CResolutionUtils::IsRefreshRateMultiple(60.0f, FPS_23_976));
  EXPECT_FALSE(CResolutionUtils::IsRefreshRateMultiple(0.0f, FPS_23_976));
  EXPECT_FALSE(CResolutionUtils::IsRefreshRateMultiple(REFRESH_119_88, 0.0f));
}

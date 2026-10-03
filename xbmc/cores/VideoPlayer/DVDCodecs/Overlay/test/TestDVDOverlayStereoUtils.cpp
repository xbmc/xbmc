/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayStereoUtils.h"

#include <gtest/gtest.h>

using namespace KODI::VIDEO::SUBTITLES;

TEST(TestDVDOverlayStereoUtils, DetectsStereoscopicOutputModes)
{
  EXPECT_FALSE(IsStereoscopicOutputMode(RenderStereoMode::OFF));
  EXPECT_FALSE(IsStereoscopicOutputMode(RenderStereoMode::HARDWAREBASED));
  EXPECT_TRUE(IsStereoscopicOutputMode(RenderStereoMode::SPLIT_VERTICAL));
  EXPECT_TRUE(IsStereoscopicOutputMode(RenderStereoMode::SPLIT_HORIZONTAL));
  EXPECT_TRUE(IsStereoscopicOutputMode(RenderStereoMode::ANAGLYPH_RED_CYAN));
  EXPECT_TRUE(IsStereoscopicOutputMode(RenderStereoMode::ANAGLYPH_GREEN_MAGENTA));
  EXPECT_TRUE(IsStereoscopicOutputMode(RenderStereoMode::ANAGLYPH_YELLOW_BLUE));
  EXPECT_TRUE(IsStereoscopicOutputMode(RenderStereoMode::INTERLACED));
  EXPECT_TRUE(IsStereoscopicOutputMode(RenderStereoMode::CHECKERBOARD));
  EXPECT_TRUE(IsStereoscopicOutputMode(RenderStereoMode::MONO));
}

TEST(TestDVDOverlayStereoUtils, SelectsEyeForLeftRightSource)
{
  EXPECT_TRUE(
      ShouldRenderStereoOverlay(DVDOverlayStereoView::BOTH, RenderStereoView::LEFT, "left_right"));
  EXPECT_TRUE(
      ShouldRenderStereoOverlay(DVDOverlayStereoView::LEFT, RenderStereoView::LEFT, "left_right"));
  EXPECT_FALSE(
      ShouldRenderStereoOverlay(DVDOverlayStereoView::RIGHT, RenderStereoView::LEFT, "left_right"));
  EXPECT_FALSE(
      ShouldRenderStereoOverlay(DVDOverlayStereoView::LEFT, RenderStereoView::RIGHT, "left_right"));
  EXPECT_TRUE(ShouldRenderStereoOverlay(DVDOverlayStereoView::RIGHT, RenderStereoView::RIGHT,
                                        "left_right"));
  EXPECT_TRUE(
      ShouldRenderStereoOverlay(DVDOverlayStereoView::LEFT, RenderStereoView::LEFT, "top_bottom"));
}

TEST(TestDVDOverlayStereoUtils, SelectsEyeForReversedSource)
{
  EXPECT_FALSE(
      ShouldRenderStereoOverlay(DVDOverlayStereoView::LEFT, RenderStereoView::LEFT, "right_left"));
  EXPECT_TRUE(
      ShouldRenderStereoOverlay(DVDOverlayStereoView::RIGHT, RenderStereoView::LEFT, "right_left"));
  EXPECT_TRUE(
      ShouldRenderStereoOverlay(DVDOverlayStereoView::LEFT, RenderStereoView::RIGHT, "bottom_top"));
  EXPECT_FALSE(ShouldRenderStereoOverlay(DVDOverlayStereoView::RIGHT, RenderStereoView::RIGHT,
                                         "bottom_top"));
}

TEST(TestDVDOverlayStereoUtils, SelectsFirstEyeWhenStereoViewIsOff)
{
  EXPECT_TRUE(
      ShouldRenderStereoOverlay(DVDOverlayStereoView::LEFT, RenderStereoView::OFF, "left_right"));
  EXPECT_FALSE(
      ShouldRenderStereoOverlay(DVDOverlayStereoView::RIGHT, RenderStereoView::OFF, "left_right"));
}

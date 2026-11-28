/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/GUITexture.h"

#include <array>

#include <gtest/gtest.h>

TEST(TestGUITexture, SnapToPixelRounds)
{
  EXPECT_EQ(CGUITexture::SnapToPixel(10.4f, false), 10.0f);
  EXPECT_EQ(CGUITexture::SnapToPixel(10.6f, false), 11.0f);
  EXPECT_EQ(CGUITexture::SnapToPixel(-0.6f, false), -1.0f);
}

TEST(TestGUITexture, SnapToPixelKeepsSubpixel)
{
  EXPECT_EQ(CGUITexture::SnapToPixel(10.4f, true), 10.4f);
  EXPECT_EQ(CGUITexture::SnapToPixel(10.6f, true), 10.6f);
  EXPECT_EQ(CGUITexture::SnapToPixel(-0.6f, true), -0.6f);
}

namespace
{
struct Corners
{
  std::array<float, 4> x;
  std::array<float, 4> y;
};

// corners of a screen rect in drawing order, snapped to whole pixels, then pushed
Corners Push(float x1, float y1, float x2, float y2)
{
  const float ux[4] = {x1, x2, x2, x1};
  const float uy[4] = {y1, y1, y2, y2};
  Corners c;
  for (size_t i = 0; i < 4; ++i)
  {
    c.x[i] = CGUITexture::SnapToPixel(ux[i], false);
    c.y[i] = CGUITexture::SnapToPixel(uy[i], false);
  }
  CGUITexture::PushThinCorners(ux, uy, c.x.data(), c.y.data());
  return c;
}
} // namespace

TEST(TestGUITexture, PushThinCornersHorizontal)
{
  const Corners c = Push(0.0f, 10.25f, 50.0f, 10.375f);
  EXPECT_EQ(c.x, (std::array<float, 4>{0.0f, 50.0f, 50.0f, 0.0f}));
  EXPECT_EQ(c.y, (std::array<float, 4>{10.0f, 10.0f, 11.0f, 11.0f}));
}

TEST(TestGUITexture, PushThinCornersVertical)
{
  // the right edge moves out, not the bottom corners
  const Corners c = Push(10.25f, 0.0f, 10.375f, 50.0f);
  EXPECT_EQ(c.x, (std::array<float, 4>{10.0f, 11.0f, 11.0f, 10.0f}));
  EXPECT_EQ(c.y, (std::array<float, 4>{0.0f, 0.0f, 50.0f, 50.0f}));
}

TEST(TestGUITexture, PushThinCornersMirrored)
{
  // mirrored, the left corners are the ones furthest right
  const Corners c = Push(10.375f, 0.0f, 10.25f, 50.0f);
  EXPECT_EQ(c.x, (std::array<float, 4>{11.0f, 10.0f, 10.0f, 11.0f}));
  EXPECT_EQ(c.y, (std::array<float, 4>{0.0f, 0.0f, 50.0f, 50.0f}));
}

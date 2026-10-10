/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/GUITexture.h"

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

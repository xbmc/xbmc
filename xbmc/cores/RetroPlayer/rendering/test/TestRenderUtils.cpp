/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/rendering/RenderUtils.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace RETRO;

TEST(TestRenderUtils, DisplayWidthKeepsSquarePixels)
{
  EXPECT_EQ(320u, CRenderUtils::GetDisplayWidth(320, 240, 0.0f));
  EXPECT_EQ(320u, CRenderUtils::GetDisplayWidth(320, 240, -1.0f));
}

TEST(TestRenderUtils, DisplayWidthFollowsAspectRatio)
{
  // SNES: 256x224 shown at 4:3
  EXPECT_EQ(299u, CRenderUtils::GetDisplayWidth(256, 224, 4.0f / 3.0f));

  // A wide frame squeezed into fewer pixels
  EXPECT_EQ(640u, CRenderUtils::GetDisplayWidth(320, 360, 16.0f / 9.0f));
}

TEST(TestRenderUtils, DisplayWidthIsNeverZero)
{
  EXPECT_EQ(1u, CRenderUtils::GetDisplayWidth(4, 1, 0.1f));
}

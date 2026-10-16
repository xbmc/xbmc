/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/GUIFontManager.h"
#include "windowing/Resolution.h"

#include <gtest/gtest.h>

// A legacy PAL layout's pixels are wider than they are tall, so on a square-pixel 4:3 screen the
// stretch to the screen and the pixel shape cancel, and a glyph keeps its own shape.
TEST(TestGUIFontManager, PALLayoutOnASquarePixelScreenKeepsTheGlyphShape)
{
  const RESOLUTION_INFO pal(720, 576, 4.0f / 3.0f);

  EXPECT_NEAR(GUIFontManager::StretchedFontAspect(1.0f, pal, 720.0f / 1024.0f, 576.0f / 768.0f),
              1.0f, 0.001f);
  EXPECT_NEAR(GUIFontManager::StretchedFontAspect(0.8f, pal, 720.0f / 1024.0f, 576.0f / 768.0f),
              0.8f, 0.001f);
}

// Estuary's 21:9 layout is 2560x1080, a 2.370 raster declared as 2.333, shown on a screen that is
// the declared shape.
TEST(TestGUIFontManager, AnamorphicLayoutAtItsDeclaredShapeKeepsTheGlyphShape)
{
  const RESOLUTION_INFO estuaryWide(2560, 1080, 21.0f / 9.0f);

  EXPECT_NEAR(
      GUIFontManager::StretchedFontAspect(1.0f, estuaryWide, 2560.0f / 2800.0f, 1080.0f / 1200.0f),
      1.0f, 0.001f);
}

TEST(TestGUIFontManager, SquarePixelLayoutStretchesWithTheScreen)
{
  const RESOLUTION_INFO skin(1920, 1080);

  EXPECT_NEAR(GUIFontManager::StretchedFontAspect(1.0f, skin, 0.5f, 0.5f), 1.0f, 0.001f);
  EXPECT_NEAR(GUIFontManager::StretchedFontAspect(1.0f, skin, 1920.0f / 2560.0f, 1.0f), 4.0f / 3.0f,
              0.001f);
}

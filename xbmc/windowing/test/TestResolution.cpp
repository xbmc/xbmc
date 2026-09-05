/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "windowing/Resolution.h"

#include <gtest/gtest.h>

// A declared shape is a statement about how the layout is meant to be seen, and DisplayRatio() is
// what selection and the proportions hold both compare against. The two must agree, or a variant
// competes and holds at a shape nobody stated.
TEST(TestResolution, DeclaredShapeIsTheShapeReported)
{
  const RESOLUTION_INFO wide(2560, 1080, 21.0f / 9.0f);

  EXPECT_NEAR(wide.DisplayRatio(), 21.0f / 9.0f, 0.001f);
}

// The case that hid this: when the declaration agrees with the pixels the correction is 1.0 and
// the arithmetic is a no-op whichever way round it is written. Every Estuary variant but one is
// this case.
TEST(TestResolution, PixelsAgreeingWithTheDeclarationNeedNoCorrection)
{
  const RESOLUTION_INFO square(1920, 1080, 16.0f / 9.0f);

  EXPECT_NEAR(square.fPixelRatio, 1.0f, 0.001f);
  EXPECT_NEAR(square.DisplayRatio(), 16.0f / 9.0f, 0.001f);
}

// Estuary's own 21:9 variant, which is self-contradictory - 2560x1080 is 2.370 and 21:9 is 2.333 -
// and is why both a 2.35 and a 2.40 raster select it while only one of them fills.
TEST(TestResolution, AnamorphicDeclarationCorrectsRatherThanCompounds)
{
  const RESOLUTION_INFO estuaryWide(2560, 1080, 21.0f / 9.0f);

  // The correction is the declared shape over the pixel shape, so it undoes the difference.
  EXPECT_NEAR(estuaryWide.fPixelRatio, (21.0f / 9.0f) / (2560.0f / 1080.0f), 0.001f);

  // Not (w/h) squared over the declaration, which is 2.408 - a shape nothing declared.
  EXPECT_LT(estuaryWide.DisplayRatio(), 2.4f);
}

// A declared shape that is not in the vocabulary is still a shape. Estuary's own widest layout
// is 2.37, which no entry names, and it has to compete at 2.37 rather than at a nearby entry.
TEST(TestResolution, AShapeOutsideTheVocabularyIsHonouredAsDeclared)
{
  const RESOLUTION_INFO odd(2560, 1080, 2.37f);

  EXPECT_NEAR(odd.DisplayRatio(), 2.37f, 0.001f);
}

// Declaring nothing means the pixels are the shape, which is what every skin shipping one
// resolution relies on.
TEST(TestResolution, NoDeclarationMeansThePixelsAreTheShape)
{
  const RESOLUTION_INFO plain(1920, 1080);

  EXPECT_NEAR(plain.fPixelRatio, 1.0f, 0.001f);
  EXPECT_NEAR(plain.DisplayRatio(), 1920.0f / 1080.0f, 0.001f);
}

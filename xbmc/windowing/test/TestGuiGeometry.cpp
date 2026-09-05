/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/Geometry.h"
#include "windowing/GuiGeometry.h"
#include "windowing/Resolution.h"

#include <cmath>

#include <gtest/gtest.h>

using namespace KODI::WINDOWING;

namespace
{

//! \brief A resolution whose calibration is the whole display, which the constructor does not
//! provide: it leaves Overscan at zero rather than at the display's bounds.
RESOLUTION_INFO MakeInfo(int width, int height, float pixelRatio = 1.0f)
{
  RESOLUTION_INFO info(width, height);
  info.Overscan.left = 0;
  info.Overscan.top = 0;
  info.Overscan.right = width;
  info.Overscan.bottom = height;
  info.fPixelRatio = pixelRatio;
  return info;
}

void ExpectRectNear(const CRect& rect, float x1, float y1, float x2, float y2, float within)
{
  EXPECT_NEAR(rect.x1, x1, within);
  EXPECT_NEAR(rect.y1, y1, within);
  EXPECT_NEAR(rect.x2, x2, within);
  EXPECT_NEAR(rect.y2, y2, within);
}

} // namespace

TEST(TestGuiGeometry, RasterUnstatedIsWholeDisplay)
{
  const RESOLUTION_INFO info = MakeInfo(3840, 2160);

  EXPECT_EQ(ComputeRasterRect(info, 0.0f), CRect(0, 0, 3840, 2160));
  EXPECT_EQ(ComputeRasterRect(info, -1.0f), CRect(0, 0, 3840, 2160));
}

// The vocabulary stores 1.78 and 16:9 is 1.7778, so a naive division would shave a
// three-pixel hairline off the display, on every ordinary television, whenever the stated
// shape is the one the screen already has. This is not a rounding bug to be "fixed" back
// into a division.
TEST(TestGuiGeometry, RasterStatedMatchingShapeIsWholeDisplay)
{
  const RESOLUTION_INFO info = MakeInfo(3840, 2160);

  EXPECT_EQ(ComputeRasterRect(info, 1.78f), CRect(0, 0, 3840, 2160));
}

TEST(TestGuiGeometry, RasterScopeLetterboxes)
{
  const RESOLUTION_INFO info = MakeInfo(3840, 2160);

  ExpectRectNear(ComputeRasterRect(info, 2.40f), 0.0f, 280.0f, 3840.0f, 1880.0f, 0.01f);
}

// The edges are whole pixels: the interface, the clip and the picture all derive from this
// rectangle through different rounding, and a fractional edge lets them disagree by a visible
// pixel at the seam. The exact division here is 322.4 / 2237.6.
TEST(TestGuiGeometry, RasterNarrowerThanDisplayPillarboxes)
{
  const RESOLUTION_INFO info = MakeInfo(2560, 1440);

  ExpectRectNear(ComputeRasterRect(info, 1.33f), 322.0f, 0.0f, 2238.0f, 1440.0f, 0.01f);
}

// The raster is a shape on the screen, so anamorphic pixels have to be accounted for before
// the stated ratio is applied to pixel counts.
TEST(TestGuiGeometry, RasterHonoursPixelRatio)
{
  const RESOLUTION_INFO info = MakeInfo(1440, 1080, 4.0f / 3.0f);

  ExpectRectNear(ComputeRasterRect(info, 2.40f), 0.0f, 140.0f, 1440.0f, 940.0f, 0.01f);
}

// The raster describes the physical screen, and the calibrated overscan rectangle is what the
// viewer has said their visible screen is - so the band is centred in the calibration, not in
// the pixel grid.
TEST(TestGuiGeometry, RasterIsCentredInTheCalibratedArea)
{
  RESOLUTION_INFO info = MakeInfo(3840, 2160);
  info.Overscan.left = 50;
  info.Overscan.top = 30;
  info.Overscan.right = 3790;
  info.Overscan.bottom = 2130;

  // The calibrated area is 3740x2100; a 2.40 band within it is 1558.33 tall, centred, and the
  // edges land on whole pixels (300.83 / 1859.17 exact).
  ExpectRectNear(ComputeRasterRect(info, 2.40f), 50.0f, 301.0f, 3790.0f, 1859.0f, 0.01f);
}

TEST(TestGuiGeometry, RasterMatchingCalibratedShapeIsTheCalibratedArea)
{
  RESOLUTION_INFO info = MakeInfo(3840, 2160);
  info.Overscan.left = 50;
  info.Overscan.top = 30;
  info.Overscan.right = 3790;
  info.Overscan.bottom = 2130;

  // 3740x2100 is 1.781, within the vocabulary tolerance of 1.78 - so all of it, exactly.
  EXPECT_EQ(ComputeRasterRect(info, 1.78f), CRect(50, 30, 3790, 2130));
}

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
  info.guiInsets = {};
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

TEST(TestGraphicContext, RasterUnstatedIsWholeDisplay)
{
  const RESOLUTION_INFO info = MakeInfo(3840, 2160);

  EXPECT_EQ(ComputeRasterRect(info, 0.0f), CRect(0, 0, 3840, 2160));
  EXPECT_EQ(ComputeRasterRect(info, -1.0f), CRect(0, 0, 3840, 2160));
}

// The vocabulary stores 1.78 and 16:9 is 1.7778, so a naive division would shave a
// three-pixel hairline off the display, on every ordinary television, whenever the stated
// shape is the one the screen already has. This is not a rounding bug to be "fixed" back
// into a division.
TEST(TestGraphicContext, RasterStatedMatchingShapeIsWholeDisplay)
{
  const RESOLUTION_INFO info = MakeInfo(3840, 2160);

  EXPECT_EQ(ComputeRasterRect(info, 1.78f), CRect(0, 0, 3840, 2160));
}

TEST(TestGraphicContext, RasterScopeLetterboxes)
{
  const RESOLUTION_INFO info = MakeInfo(3840, 2160);

  ExpectRectNear(ComputeRasterRect(info, 2.40f), 0.0f, 280.0f, 3840.0f, 1880.0f, 0.01f);
}

// The edges are whole pixels: the interface, the clip and the picture all derive from this
// rectangle through different rounding, and a fractional edge lets them disagree by a visible
// pixel at the seam. The exact division here is 322.4 / 2237.6.
TEST(TestGraphicContext, RasterNarrowerThanDisplayPillarboxes)
{
  const RESOLUTION_INFO info = MakeInfo(2560, 1440);

  ExpectRectNear(ComputeRasterRect(info, 1.33f), 322.0f, 0.0f, 2238.0f, 1440.0f, 0.01f);
}

// The raster is a shape on the screen, so anamorphic pixels have to be accounted for before
// the stated ratio is applied to pixel counts.
TEST(TestGraphicContext, RasterHonoursPixelRatio)
{
  const RESOLUTION_INFO info = MakeInfo(1440, 1080, 4.0f / 3.0f);

  ExpectRectNear(ComputeRasterRect(info, 2.40f), 0.0f, 140.0f, 1440.0f, 940.0f, 0.01f);
}

// The raster describes the physical screen, and the calibrated overscan rectangle is what the
// viewer has said their visible screen is - so the band is centred in the calibration, not in
// the pixel grid.
TEST(TestGraphicContext, RasterIsCentredInTheCalibratedArea)
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

TEST(TestGraphicContext, RasterMatchingCalibratedShapeIsTheCalibratedArea)
{
  RESOLUTION_INFO info = MakeInfo(3840, 2160);
  info.Overscan.left = 50;
  info.Overscan.top = 30;
  info.Overscan.right = 3790;
  info.Overscan.bottom = 2130;

  // 3740x2100 is 1.781, within the vocabulary tolerance of 1.78 - so all of it, exactly.
  EXPECT_EQ(ComputeRasterRect(info, 1.78f), CRect(50, 30, 3790, 2130));
}

TEST(TestGraphicContext, GuiRectDefaultIsIdentity)
{
  const RESOLUTION_INFO skin(1920, 1080);
  const RESOLUTION_INFO info = MakeInfo(3840, 2160);
  float scaleX = 0.0f;
  float scaleY = 0.0f;

  const CRect rect = ComputeGuiRect(skin, info, 0.0f, CRect{}, false, 0.0f, scaleX, scaleY);

  EXPECT_EQ(rect, CRect(0, 0, 3840, 2160));
  EXPECT_FLOAT_EQ(scaleX, 0.5f);
  EXPECT_FLOAT_EQ(scaleY, 0.5f);
}

// A windowed Kodi is a display of an arbitrary shape, and by default the interface stretches
// to fill it - stating no raster must leave that exactly alone, whatever the shape.
TEST(TestGraphicContext, GuiRectDefaultIsIdentityAtAnyWindowShape)
{
  const RESOLUTION_INFO skin(2040, 1080);
  const RESOLUTION_INFO info = MakeInfo(2560, 1369);
  float scaleX = 0.0f;
  float scaleY = 0.0f;

  const CRect rect = ComputeGuiRect(skin, info, 0.0f, CRect{}, false, 0.0f, scaleX, scaleY);

  EXPECT_EQ(rect, CRect(0, 0, 2560, 1369));
}

// Calibration may legally place the interface outside the pixel grid - the calibration UI
// offers a quarter of the display in each direction - and with no raster stated nothing may
// clamp it.
TEST(TestGraphicContext, GuiRectOutwardCalibrationIsUntouchedWhenUnstated)
{
  const RESOLUTION_INFO skin(1920, 1080);
  RESOLUTION_INFO info = MakeInfo(1920, 1080);
  info.Overscan.left = -40;
  info.Overscan.top = -20;
  info.Overscan.right = 1960;
  info.Overscan.bottom = 1100;
  float scaleX = 0.0f;
  float scaleY = 0.0f;

  const CRect rect = ComputeGuiRect(skin, info, 0.0f, CRect{}, false, 0.0f, scaleX, scaleY);

  EXPECT_EQ(rect, CRect(-40, -20, 1960, 1100));
}

// The pair that documents the skin resolution as the discriminator: Kodi picks the closest of
// the skin's declared resolutions to the display, so a 1.87 window runs Estuary's 17:9
// 2040x1080 variant, not 16:9 - and the held rectangle differs by 60 pixels a side.
TEST(TestGraphicContext, GuiRectHeldShapeFollowsTheSkinVariant)
{
  const RESOLUTION_INFO info = MakeInfo(2560, 1369);
  float scaleX = 0.0f;
  float scaleY = 0.0f;

  const CRect wide =
      ComputeGuiRect(RESOLUTION_INFO(2040, 1080), info, 2.40f, CRect{}, true, 0.0f, scaleX, scaleY);
  ExpectRectNear(wide, 272.28f, 151.0f, 2287.72f, 1218.0f, 0.05f);

  const CRect narrow =
      ComputeGuiRect(RESOLUTION_INFO(1920, 1080), info, 2.40f, CRect{}, true, 0.0f, scaleX, scaleY);
  ExpectRectNear(narrow, 331.56f, 151.0f, 2228.44f, 1218.0f, 0.05f);
}

// A skin already the shape of the area it is in must fill it. The selected skin resolution and
// the window routinely differ by a fraction of a percent - Estuary's 17:9 in a 1.87 window is
// 1% - and holding on that difference would letterbox the interface by a few permanent pixels.
TEST(TestGraphicContext, GuiRectHeldShapeFillsWhenAlreadyThatShape)
{
  const RESOLUTION_INFO skin(2040, 1080);
  const RESOLUTION_INFO info = MakeInfo(2560, 1369);
  float scaleX = 0.0f;
  float scaleY = 0.0f;

  const CRect rect = ComputeGuiRect(skin, info, 0.0f, CRect{}, true, 0.0f, scaleX, scaleY);

  EXPECT_EQ(rect, CRect(0, 0, 2560, 1369));
}

TEST(TestGraphicContext, GuiRectIntersectsThePlaybackConfinement)
{
  const RESOLUTION_INFO skin(1920, 1080);
  const RESOLUTION_INFO info = MakeInfo(3840, 2160);
  const CRect content{100, 50, 3700, 2000};
  float scaleX = 0.0f;
  float scaleY = 0.0f;

  const CRect rect = ComputeGuiRect(skin, info, 0.0f, content, false, 0.0f, scaleX, scaleY);

  EXPECT_EQ(rect, content);
}

TEST(TestGraphicContext, GuiRectZoomGrowsAroundTheCentre)
{
  const RESOLUTION_INFO skin(1920, 1080);
  const RESOLUTION_INFO info = MakeInfo(1920, 1080);
  float scaleX = 0.0f;
  float scaleY = 0.0f;

  const CRect rect = ComputeGuiRect(skin, info, 0.0f, CRect{}, false, 0.1f, scaleX, scaleY);

  ExpectRectNear(rect, -96.0f, -54.0f, 2016.0f, 1134.0f, 0.01f);
  EXPECT_NEAR(scaleX, 1920.0f / 2112.0f, 0.0001f);
}

// The screen tools are aligned against, not corrected: a frame drawn inside a stated 2.40 band
// would have the viewer align their optics to the last answer rather than to their screen, and
// an overscan marker inside it cannot be reached at all, since the clip bounds every window.
TEST(TestGraphicContext, ScreenToolSuspendsTheStatedShape)
{
  EXPECT_FLOAT_EQ(ComputeRasterAspectInForce(2.40f, false), 2.40f);
  EXPECT_FLOAT_EQ(ComputeRasterAspectInForce(2.40f, true), 0.0f);

  const RESOLUTION_INFO info = MakeInfo(3840, 2160);
  const float inForce = ComputeRasterAspectInForce(2.40f, true);

  EXPECT_EQ(ComputeRasterRect(info, inForce), CRect(0, 0, 3840, 2160));
}

TEST(TestGraphicContext, ScreenToolHoldsNoShape)
{
  EXPECT_TRUE(ComputeGuiKeepShape(true, false, false));
  EXPECT_FALSE(ComputeGuiKeepShape(true, true, false));
  EXPECT_FALSE(ComputeGuiKeepShape(true, false, true));
  EXPECT_FALSE(ComputeGuiKeepShape(false, false, false));
}

// The two suspensions together, which is the whole of what a screen tool needs: with a 2.40
// raster stated and the hold on, the clip is still the entire pixel grid.
TEST(TestGraphicContext, ClipIsTheWholeDisplayWhileAScreenToolIsUp)
{
  const RESOLUTION_INFO info = MakeInfo(3840, 2160);
  const CRect display{0, 0, 3840, 2160};
  const CRect held{498, 280, 3342, 1880};

  const CRect raster = ComputeRasterRect(info, ComputeRasterAspectInForce(2.40f, true));

  EXPECT_EQ(
      ComputeClipBounds(display, raster, ComputeGuiKeepShape(true, false, true) ? held : CRect{}),
      display);
}

// A raster is stated over the API as a number, and one small enough to round to no area at all
// is legal there. Dividing by it puts an infinity into the transform every coordinate passes
// through, so nothing draws - including the settings that would state a different one.
TEST(TestGraphicContext, AShapeThatRoundsToNoAreaLeavesTheInterfaceWhereItWas)
{
  const RESOLUTION_INFO skin(1920, 1080);
  const RESOLUTION_INFO info = MakeInfo(1920, 1080);
  float scaleX = 0.0f;
  float scaleY = 0.0f;

  const CRect rect = ComputeGuiRect(skin, info, 0.0001f, CRect{}, false, 0.0f, scaleX, scaleY);

  EXPECT_EQ(rect, CRect(0, 0, 1920, 1080));
  EXPECT_TRUE(std::isfinite(scaleX));
  EXPECT_TRUE(std::isfinite(scaleY));
  EXPECT_GT(scaleX, 0.0f);
  EXPECT_GT(scaleY, 0.0f);
}

TEST(TestGraphicContext, ClipIsTheDisplayWhenNothingNarrows)
{
  const CRect display{0, 0, 3840, 2160};

  EXPECT_EQ(ComputeClipBounds(display, display, CRect{}), display);
}

// Skins park panels outside their layout and slide them in - Estuary's sit 320 units off -
// and once the layout is confined to a band, "outside the layout" is on the screen, in the
// surround. The raster bounds them whichever way the interface is being filled: nothing
// exceeds the raster, whatever other settings say.
TEST(TestGraphicContext, ClipHoldsTheRasterWithoutKeepShape)
{
  const CRect display{0, 0, 3840, 2160};
  const CRect raster{0, 280, 3840, 1880};

  EXPECT_EQ(ComputeClipBounds(display, raster, CRect{}), raster);
}

TEST(TestGraphicContext, ClipNarrowsToTheHeldInterface)
{
  const CRect display{0, 0, 3840, 2160};
  const CRect raster{0, 280, 3840, 1880};
  const CRect gui{498, 280, 3342, 1880};

  EXPECT_EQ(ComputeClipBounds(display, raster, gui), gui);
}

// A zoomed interface may be laid out past the raster, but it may not be drawn there - in a
// masked room the area past the raster is the mask, and a cut edge beats text on it.
TEST(TestGraphicContext, ClipCutsAZoomedInterfaceAtTheRaster)
{
  const CRect display{0, 0, 3840, 2160};
  const CRect raster{0, 280, 3840, 1880};
  const CRect zoomedGui{400, 180, 3440, 1980};

  EXPECT_EQ(ComputeClipBounds(display, raster, zoomedGui), CRect(400, 280, 3440, 1880));
}

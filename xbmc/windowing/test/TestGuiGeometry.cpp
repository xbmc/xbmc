/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/Geometry.h"
#include "windowing/GuiGeometry.h"

#include <gtest/gtest.h>

using namespace KODI::WINDOWING;

namespace
{

CRect Display(float width, float height)
{
  return {0.0f, 0.0f, width, height};
}

void ExpectRectNear(const CRect& rect, float x1, float y1, float x2, float y2, float within)
{
  EXPECT_NEAR(rect.x1, x1, within);
  EXPECT_NEAR(rect.y1, y1, within);
  EXPECT_NEAR(rect.x2, x2, within);
  EXPECT_NEAR(rect.y2, y2, within);
}

} // namespace

TEST(TestGuiGeometry, AspectUnstatedIsTheWholeArea)
{
  const CRect display = Display(3840.0f, 2160.0f);

  EXPECT_EQ(ComputeAspectRect(display, 0.0f, 1.0f), display);
  EXPECT_EQ(ComputeAspectRect(display, -1.0f, 1.0f), display);
}

TEST(TestGuiGeometry, AspectMatchingTheAreaIsTheWholeArea)
{
  //! A display of exactly the ratio asked for divides out exactly, with nothing to centre.
  EXPECT_EQ(ComputeAspectRect(Display(3840.0f, 2160.0f), 16.0f / 9.0f, 1.0f),
            CRect(0, 0, 3840, 2160));
}

TEST(TestGuiGeometry, AspectLetterboxes)
{
  ExpectRectNear(ComputeAspectRect(Display(3840.0f, 2160.0f), 2.40f, 1.0f), 0.0f, 280.0f, 3840.0f,
                 1880.0f, 0.01f);
}

// The edges are whole pixels: the exact division here is 322.4 / 2237.6.
TEST(TestGuiGeometry, AspectPillarboxes)
{
  ExpectRectNear(ComputeAspectRect(Display(2560.0f, 1440.0f), 1.33f, 1.0f), 322.0f, 0.0f, 2238.0f,
                 1440.0f, 0.01f);
}

// The rectangle is a shape on the screen, so anamorphic pixels have to be accounted for before
// the stated ratio is applied to pixel counts.
TEST(TestGuiGeometry, AspectHonoursPixelRatio)
{
  ExpectRectNear(ComputeAspectRect(Display(1440.0f, 1080.0f), 2.40f, 4.0f / 3.0f), 0.0f, 140.0f,
                 1440.0f, 940.0f, 0.01f);
}

TEST(TestGuiGeometry, AspectIsCentredInAnAreaThatIsNotAtTheOrigin)
{
  //! The area is 3740x2100; a 2.40 band within it is 1558.33 tall, centred (300.83 / 1859.17).
  const CRect area{50.0f, 30.0f, 3790.0f, 2130.0f};

  ExpectRectNear(ComputeAspectRect(area, 2.40f, 1.0f), 50.0f, 301.0f, 3790.0f, 1859.0f, 0.01f);
}

/*!
 * A ratio a hair off the area's own shape still comes back at the ratio asked for.
 *
 * This is the property the tool exists for. 2560x1080 is 2.370, and 2.40 is 1.3% away from it -
 * near enough that a vocabulary would call a picture of that shape "2.40", and nowhere near
 * near enough to mask a screen against. Rounding it up to the panel would draw a border round
 * the whole display and label it a ratio the display does not have.
 */
TEST(TestGuiGeometry, AspectDoesNotRoundUpToAnAreaOfNearlyTheSameShape)
{
  const CRect held = ComputeAspectRect(Display(2560.0f, 1080.0f), 2.40f, 1.0f);

  EXPECT_NE(held, CRect(0, 0, 2560, 1080));
  ExpectRectNear(held, 0.0f, 7.0f, 2560.0f, 1073.0f, 0.01f);
}

/*!
 * The corollary: a ratio written to two decimals is not the ratio, and asking for one gets a
 * hairline. 1920/1.78 is 1078.65, so 1.78 on a 1920x1080 display letterboxes by a pixel.
 *
 * Callers pass a vocabulary entry's own ratio, which holds 16:9 as the exact fraction, so this
 * is what it looks like when one passes the label instead.
 */
TEST(TestGuiGeometry, ATwoDecimalRatioIsNotTheRatio)
{
  EXPECT_EQ(ComputeAspectRect(Display(1920.0f, 1080.0f), 16.0f / 9.0f, 1.0f),
            CRect(0, 0, 1920, 1080));

  ExpectRectNear(ComputeAspectRect(Display(1920.0f, 1080.0f), 1.78f, 1.0f), 0.0f, 1.0f, 1920.0f,
                 1079.0f, 0.01f);
}

TEST(TestGuiGeometry, AnEmptyAreaIsAnsweredWithItself)
{
  const CRect empty{100.0f, 100.0f, 100.0f, 100.0f};

  EXPECT_EQ(ComputeAspectRect(empty, 1.78f, 1.0f), empty);
}

/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/AspectRatioVocabulary.h"
#include "video/geometry/GeometryTransforms.h"
#include "video/geometry/test/GeometryTestHelpers.h"

#include <gtest/gtest.h>

using namespace KODI::VIDEO::GEOMETRY;
using namespace KODI::VIDEO::GEOMETRY::TEST;
using namespace KODI::UTILS;

TEST(TestGeometryTransforms, SquarePixelsWhenNoDisplayAspectIsDeclared)
{
  StreamGeometry stream{CRectInt{0, 0, 1920, 1080}, 0.0f, 0};
  EXPECT_FLOAT_EQ(1.0f, PixelAspectRatio(stream));

  // Display space must degenerate to coded space rather than to nothing, so that content
  // with no anamorphic coding is unaffected by the correction existing at all.
  ExpectRect(ToDisplaySpace(CRectInt{0, 140, 1920, 940}, stream), 0.0f, 140.0f, 1920.0f, 940.0f);
}

TEST(TestGeometryTransforms, AnamorphicPixelsAreWiderThanTheyAreCoded)
{
  EXPECT_NEAR(1.4222f, PixelAspectRatio(AnamorphicPal()), 0.0001f);
  ExpectRect(ToSquarePixels(CRectInt{0, 0, 720, 576}, AnamorphicPal()), 0.0f, 0.0f, 1024.0f,
             576.0f);
}

TEST(TestGeometryTransforms, MaintainedTargetNestsInsideTheOperatingArea)
{
  // The automation asks for 2.35 in a room whose raster is 2.40. The target is the 2.35 area
  // of that band - not of the display - so it is 40px in from each side and full height.
  ExpectRect(MaintainedRect(2.35f, 2.35f, ScopeRaster), 40.0f, 280.0f, 3800.0f, 1880.0f);
}

// The reason nesting is the reading this builds, rather than letting a maintained ratio
// replace the operating area. Taken against the display a 2.35 target is 1634 tall, which
// overhangs the 1600-tall raster by 17px top and bottom - picture off the screen, on a
// constitution where nothing exceeds the raster ever.
TEST(TestGeometryTransforms, MaintainedTargetCannotEscapeTheOperatingArea)
{
  const CRect display{0.0f, 0.0f, 3840.0f, 2160.0f};
  EXPECT_NEAR(1634.04f, FitAspect(2.35f, display).Height(), 0.01f);

  const CRect maintained = MaintainedRect(2.35f, 2.35f, ScopeRaster);
  EXPECT_GE(maintained.y1, ScopeRaster.y1);
  EXPECT_LE(maintained.y2, ScopeRaster.y2);
  EXPECT_GE(maintained.x1, ScopeRaster.x1);
  EXPECT_LE(maintained.x2, ScopeRaster.x2);
}

// The Dark Knight maintained at 2.35 in a 2.40 room, through its two geometries. Both land
// inside the same maintained target, which is what lets one lens position serve the whole film.
TEST(TestGeometryTransforms, MaintainedContentBindsOnWidthOrHeightAsItsOwnRatioRequires)
{
  // Scope sections: wider than the target, so they bind on width and letterbox within it.
  // Taken at the 2.40 the vocabulary labels the film, not at its own 2.3980 - the same target,
  // one and a third pixels apart.
  ExpectRect(MaintainedRect(2.35f, 2.40f, ScopeRaster), 40.0f, 296.67f, 3800.0f, 1863.33f);

  // IMAX sections: taller than the target, so they bind on height and pillarbox within it.
  ExpectRect(MaintainedRect(2.35f, 16.0f / 9.0f, ScopeRaster), 497.78f, 280.0f, 3342.22f, 1880.0f);
}

TEST(TestGeometryTransforms, AbsentMaintainAndUnknownContentBothDegradeToWhatIsAlreadyThere)
{
  // No maintain stated: the operating area is the target. Shown with content at the raster's
  // own ratio, which then fills it - the same content under a 2.35 maintain is inset to
  // 3760x1566.67, so this is the difference the override actually makes.
  ExpectRect(MaintainedRect(0.0f, 2.40f, ScopeRaster), 0.0f, 280.0f, 3840.0f, 1880.0f);

  // Content ratio unknown: fill what the automation named rather than guess a shape for it.
  // Predictability is the whole contract - the lens is already moving to this rectangle.
  ExpectRect(MaintainedRect(2.35f, 0.0f, ScopeRaster), 40.0f, 280.0f, 3800.0f, 1880.0f);
}

// Worth stating because it bounds what the override can do: a maintain only moves the picture
// for content wider than it. Content taller than the maintained ratio binds on the target's
// height, and the target is full height, so it lands where it would have landed anyway.
TEST(TestGeometryTransforms, MaintainDoesNothingToContentTallerThanItself)
{
  // Both are the 16:9 rectangle of a 1600-tall band, centred: 2844.44 wide at x = 497.78.
  ExpectRect(MaintainedRect(2.35f, 16.0f / 9.0f, ScopeRaster), 497.78f, 280.0f, 3342.22f, 1880.0f);
  ExpectRect(MaintainedRect(0.0f, 16.0f / 9.0f, ScopeRaster), 497.78f, 280.0f, 3342.22f, 1880.0f);
}

// The reason the resolver works in display space at all. A scope film on an anamorphic PAL
// DVD is coded at a ratio nothing was ever shot at - and that ratio is not merely meaningless,
// it is close enough to an unrelated entry to be accepted as it.
TEST(TestGeometryTransforms, CodedRatioOfAnamorphicScopeMatchesAnUnrelatedEntry)
{
  const CRectInt coded{0, 70, 720, 506};

  const float codedRatio = 720.0f / 436.0f;
  const auto asCoded = CAspectRatioVocabulary::Match(codedRatio, AspectRatioUse::Detect);
  ASSERT_TRUE(asCoded.has_value());
  EXPECT_FLOAT_EQ(1.66f, asCoded->ratio);

  const CRect upright = ToSquarePixels(coded, AnamorphicPal());
  const auto asDisplayed =
      CAspectRatioVocabulary::Match(upright.Width() / upright.Height(), AspectRatioUse::Detect);
  ASSERT_TRUE(asDisplayed.has_value());
  EXPECT_FLOAT_EQ(2.35f, asDisplayed->ratio);
}

TEST(TestGeometryTransforms, SquarePixelsAreExactWhenTheStreamAgreesWithItsCoding)
{
  // A stream declaring the ratio it is already coded at has square pixels, and must come back
  // out as the frame it went in as.
  const StreamGeometry stream{CRectInt{0, 0, 1920, 800}, 2.4f, 0};
  EXPECT_EQ(1.0f, PixelAspectRatio(stream));
  ExpectRect(ToDisplaySpace(CRectInt{0, 0, 1920, 800}, stream), 0.0f, 0.0f, 1920.0f, 800.0f);
}

/*!
 * The region and its pixel aspect are one answer, and the reason they are taken together rather
 * than separately. A half side-by-side view is half the packing's width at the packing's display
 * ratio, so its pixels are twice as wide - and a path that measured the view while taking the
 * packing's pixel aspect classified a scope picture as 1.2.
 */
TEST(TestGeometryTransforms, AStereoscopicViewCarriesItsOwnPixelAspect)
{
  const StreamGeometry sideBySide{MeasuredStreamGeometry("left_right", 1920, 1080, 16.0f / 9.0f)};
  EXPECT_EQ(CRectInt(0, 0, 960, 1080), sideBySide.coded);
  EXPECT_FLOAT_EQ(2.0f, PixelAspectRatio(sideBySide));

  const StreamGeometry topBottom{MeasuredStreamGeometry("top_bottom", 1920, 1080, 16.0f / 9.0f)};
  EXPECT_EQ(CRectInt(0, 0, 1920, 540), topBottom.coded);
  EXPECT_FLOAT_EQ(0.5f, PixelAspectRatio(topBottom));

  // Mono is the whole frame, and 16:9 coded at 16:9 is square pixels.
  const StreamGeometry mono{MeasuredStreamGeometry("mono", 1920, 1080, 16.0f / 9.0f)};
  EXPECT_EQ(CRectInt(0, 0, 1920, 1080), mono.coded);
  EXPECT_FLOAT_EQ(1.0f, PixelAspectRatio(mono));

  // A stream declaring no display ratio is taken as square pixels whatever it packs, so that
  // display space degenerates to coded space rather than to nothing.
  EXPECT_FLOAT_EQ(1.0f, PixelAspectRatio(MeasuredStreamGeometry("left_right", 1920, 1080, 0.0f)));
}

/*!
 * The renderer cuts its source to the rectangle measured from the frame it is drawing, which
 * is not the one the published answer describes - live detection stamps each picture with its
 * own. The ratio comes back with it, because what the picture is scaled to has to be the shape
 * of what was cut rather than of what was resolved.
 */
TEST(TestGeometryTransforms, TheCutFollowsTheFramesOwnMeasurement)
{
  RenderGeometry geometry;
  geometry.codedFrame = CRectInt{0, 0, 3840, 2160};
  geometry.par = 1.0f;
  geometry.aspect = 16.0f / 9.0f;

  const CRect source{0.0f, 0.0f, 3840.0f, 2160.0f};

  const ContentCut scope = CutToContent(geometry, CRectInt{0, 280, 3840, 1880}, source);
  ExpectRect(scope.source, 0.0f, 280.0f, 3840.0f, 1880.0f);
  EXPECT_NEAR(2.4f, scope.aspect, 0.01f);
}

/*!
 * Live detection off, or no reading taken yet. The published answer still stands, so the source
 * is cut to the rectangle the resolver settled on - returning it uncut while reporting the
 * resolved ratio would leave the coded bars on screen and then scale the picture as though they
 * had been removed, which squashes it by the difference between the two shapes.
 */
TEST(TestGeometryTransforms, WithNothingMeasuredOnTheFrameTheResolvedRectangleIsStillCut)
{
  RenderGeometry geometry;
  geometry.codedFrame = CRectInt{0, 0, 3840, 2160};
  geometry.par = 1.0f;
  geometry.aspect = 2.4f;

  // What the resolver settled on for the title: a 2.40 band inside a 16:9 frame.
  geometry.displayFrame = CRect{0.0f, 0.0f, 3840.0f, 2160.0f};
  geometry.displayRect = CRect{0.0f, 280.0f, 3840.0f, 1880.0f};

  const CRect source{0.0f, 0.0f, 3840.0f, 2160.0f};
  const ContentCut none = CutToContent(geometry, CRectInt{}, source);

  ExpectRect(none.source, 0.0f, 280.0f, 3840.0f, 1880.0f);
  EXPECT_FLOAT_EQ(2.4f, none.aspect);
}

/*!
 * A rotated stream is decoded upright and turned at draw time, so the frame's own measurement,
 * the coded frame and the source rectangle are all in the same unrotated space. The cut has to
 * leave them there: a quarter turn applied to it would put a letterbox crop down the sides. The
 * ratio is the one part that does carry the rotation, answered as it leaves the picture.
 */
TEST(TestGeometryTransforms, ARotatedStreamIsCutInCodedSpace)
{
  RenderGeometry geometry;
  geometry.codedFrame = CRectInt{0, 0, 3840, 2160};
  geometry.par = 1.0f;
  geometry.aspect = 16.0f / 9.0f;

  const CRect source{0.0f, 0.0f, 3840.0f, 2160.0f};
  const CRectInt content{0, 280, 3840, 1880};

  for (const int orientation : {0, 90, 180, 270})
  {
    geometry.orientation = orientation;

    const ContentCut cut = CutToContent(geometry, content, source);
    ExpectRect(cut.source, 0.0f, 280.0f, 3840.0f, 1880.0f);
    EXPECT_NEAR(orientation % 180 == 0 ? 2.4f : 1.0f / 2.4f, cut.aspect, 0.01f)
        << "at " << orientation << " degrees";
  }
}

//! With no resolved rectangle either there is nothing to cut to, and the source stands.
TEST(TestGeometryTransforms, WithNoResolvedRectangleTheSourceIsLeftAlone)
{
  RenderGeometry geometry;
  geometry.codedFrame = CRectInt{0, 0, 3840, 2160};
  geometry.aspect = 16.0f / 9.0f;

  const CRect source{0.0f, 0.0f, 3840.0f, 2160.0f};
  const ContentCut none = CutToContent(geometry, CRectInt{}, source);

  ExpectRect(none.source, 0.0f, 0.0f, 3840.0f, 2160.0f);
  EXPECT_FLOAT_EQ(16.0f / 9.0f, none.aspect);
}

TEST(TestGeometryTransforms, AQuarterTurnTransposesTheRectangle)
{
  const StreamGeometry stream = Uhd(90);
  ExpectRect(ToDisplaySpace(CRectInt{0, 0, 3840, 2160}, stream), 0.0f, 0.0f, 2160.0f, 3840.0f);
  ExpectRect(ToDisplaySpace(CRectInt{0, 280, 3840, 1880}, stream), 280.0f, 0.0f, 1880.0f, 3840.0f);

  ExpectRect(ToDisplaySpace(CRectInt{0, 280, 3840, 1880}, Uhd(180)), 0.0f, 280.0f, 3840.0f,
             1880.0f);
  ExpectRect(ToDisplaySpace(CRectInt{0, 280, 3840, 1880}, Uhd(270)), 280.0f, 0.0f, 1880.0f,
             3840.0f);
}

TEST(TestGeometryTransforms, AnUnusableRotationIsTreatedAsUpright)
{
  ExpectRect(ToDisplaySpace(CRectInt{0, 280, 3840, 1880}, Uhd(45)), 0.0f, 280.0f, 3840.0f, 1880.0f);
}

TEST(TestGeometryTransforms, SideBySideSelectsOneView)
{
  const CRectInt view = StereoViewRect("left_right", 3840, 2160);

  EXPECT_EQ(0, view.x1);
  EXPECT_EQ(0, view.y1);
  EXPECT_EQ(1920, view.x2);
  EXPECT_EQ(2160, view.y2);

  EXPECT_EQ(view.x2, StereoViewRect("right_left", 3840, 2160).x2);
}

TEST(TestGeometryTransforms, TopAndBottomSelectsOneView)
{
  const CRectInt view = StereoViewRect("top_bottom", 3840, 2160);

  EXPECT_EQ(3840, view.x2);
  EXPECT_EQ(1080, view.y2);

  EXPECT_EQ(view.y2, StereoViewRect("bottom_top", 3840, 2160).y2);
}

/*!
 * An unrecognised mode must be left whole rather than guessed at. Halving a mono frame
 * would report a pillarbox that is not there, and that is the direction that masks real
 * picture.
 */
TEST(TestGeometryTransforms, MonoAndUnknownModesAreLeftWhole)
{
  EXPECT_TRUE(StereoViewRect("", 3840, 2160).IsEmpty());
  EXPECT_TRUE(StereoViewRect("mono", 3840, 2160).IsEmpty());
  EXPECT_TRUE(StereoViewRect("block_lr", 3840, 2160).IsEmpty());
  EXPECT_TRUE(StereoViewRect("anaglyph_cyan_red", 3840, 2160).IsEmpty());
}

/*!
 * The region a measurement describes, which everything resolving one has to be told about.
 * StereoViewRect() answers "nothing to narrow" as an empty rectangle, and an empty rectangle
 * used as a frame is a frame with no picture in it - so the fallback belongs with the rule
 * rather than at each of the places that needs it.
 */
TEST(TestGeometryTransforms, TheMeasuredFrameIsOneViewOrTheWholeFrame)
{
  EXPECT_EQ(CRectInt(0, 0, 1920, 2160), MeasuredFrameRect("left_right", 3840, 2160));
  EXPECT_EQ(CRectInt(0, 0, 3840, 1080), MeasuredFrameRect("top_bottom", 3840, 2160));
  EXPECT_EQ(CRectInt(0, 0, 3840, 2160), MeasuredFrameRect("mono", 3840, 2160));
  EXPECT_EQ(CRectInt(0, 0, 3840, 2160), MeasuredFrameRect("", 3840, 2160));
  EXPECT_EQ(CRectInt(0, 0, 3840, 2160), MeasuredFrameRect("anaglyph_cyan_red", 3840, 2160));
}

/*!
 * The reduced-readback path detects on a small copy of the frame and returns to coded space
 * through ScaleRect. The 2.40 rectangle of a 4K frame read at 960 wide is the case that
 * ships: 960x540's scope rows are 70..470, and coded rows 280..1880 must come back exactly,
 * because the vocabulary entry's own rectangle is what the selector compares against.
 */
TEST(TestGeometryTransforms, ScaleRectReturnsAReducedReadingToCodedSpace)
{
  const CRectInt reduced{0, 70, 960, 470};
  const CRectInt coded = ScaleRect(reduced, 960, 540, 3840, 2160);

  EXPECT_EQ(0, coded.x1);
  EXPECT_EQ(280, coded.y1);
  EXPECT_EQ(3840, coded.x2);
  EXPECT_EQ(1880, coded.y2);
}

TEST(TestGeometryTransforms, ScaleRectRoundsToTheNearestPixel)
{
  // 1606 coded rows read at 402: one reduced row is 3.995 coded rows, so row 100 lands at
  // 399.5 and must round rather than truncate.
  const CRectInt reduced{0, 100, 960, 302};
  const CRectInt coded = ScaleRect(reduced, 960, 402, 3840, 1606);

  EXPECT_EQ(400, coded.y1);
  EXPECT_EQ(1206, coded.y2);
}

TEST(TestGeometryTransforms, ScaleRectLeavesADegenerateSpaceAlone)
{
  const CRectInt rect{1, 2, 3, 4};
  EXPECT_EQ(rect, ScaleRect(rect, 0, 540, 3840, 2160));
  EXPECT_EQ(rect, ScaleRect(rect, 960, 540, 0, 2160));
}

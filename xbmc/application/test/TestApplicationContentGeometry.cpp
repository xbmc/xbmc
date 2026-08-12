/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "application/ApplicationContentGeometry.h"
#include "video/geometry/test/GeometryTestHelpers.h"

#include <optional>

#include <gtest/gtest.h>

using namespace KODI::VIDEO::GEOMETRY;

namespace
{

GeometryOverrides Stated(float raster, std::optional<OsdPlacement> placement, float maintain)
{
  GeometryOverrides overrides;
  overrides.rasterAspect = raster;
  overrides.osdPlacement = placement;
  overrides.maintainAspect = maintain;
  return overrides;
}

//! \brief Open a file with nothing known about it, which is what promotes an armed instruction.
void OpenFile(CApplicationContentGeometry& geometry)
{
  geometry.SetFileInputs(ContentGeometryLookup{}, {}, 0.0f);
}

//! \brief A stored measurement of a scope title in a 16:9 frame, whose envelope caught a
//! taller sequence - so the envelope and the rectangle carry different heights.
ContentGeometryLookup MeasuredScope()
{
  ContentGeometryLookup lookup;
  lookup.record = TEST::ScopeHdRecord(CRectInt{0, 70, 1920, 1010});
  lookup.state = ContentGeometryState::VALID;
  return lookup;
}

} // unnamed namespace

// Stated with the request to play, so there is no stream to apply it to yet. It has to survive
// until the file opens, which is the thing these two calls are sequenced around.
TEST(TestApplicationContentGeometry, AnArmedInstructionTakesEffectWhenTheFileOpens)
{
  CApplicationContentGeometry geometry;
  EXPECT_FALSE(geometry.GetOverrides().Any());

  geometry.SetPendingOverrides(Stated(2.40f, OsdPlacement::Picture, 2.35f));

  // Not yet: nothing has opened.
  EXPECT_FALSE(geometry.GetOverrides().Any());

  OpenFile(geometry);

  EXPECT_FLOAT_EQ(2.40f, geometry.GetOverrides().rasterAspect);
  EXPECT_EQ(OsdPlacement::Picture,
            geometry.GetOverrides().osdPlacement.value_or(OsdPlacement::Raster));
  EXPECT_FLOAT_EQ(2.35f, geometry.GetOverrides().maintainAspect);
}

// The armed slot is consumed rather than left standing, or the instruction given for one title
// would arrive again on top of the next one.
TEST(TestApplicationContentGeometry, AnArmedInstructionIsConsumedByTheFileItWasArmedFor)
{
  CApplicationContentGeometry geometry;
  geometry.SetPendingOverrides(Stated(2.40f, std::nullopt, 0.0f));

  OpenFile(geometry);
  EXPECT_FLOAT_EQ(2.40f, geometry.GetOverrides().rasterAspect);

  // A second file, armed with nothing, must not inherit the first one's raster.
  OpenFile(geometry);
  EXPECT_FALSE(geometry.GetOverrides().Any());
}

// A request to play can end before any player exists - an unresolvable item, a disc whose
// playlist chooser was cancelled - and nothing announces a stop for a playback that never began.
// Without disarming there, what was armed for it waits in the slot for whatever plays next.
TEST(TestApplicationContentGeometry, ARequestThatNeverOpensDisarmsWhatItStated)
{
  CApplicationContentGeometry geometry;
  geometry.SetPendingOverrides(Stated(2.40f, OsdPlacement::Picture, 2.35f));

  geometry.ClearPendingOverrides();

  OpenFile(geometry);
  EXPECT_FALSE(geometry.GetOverrides().Any());
}

// Disarming is scoped to the armed slot. A failed request while a film is running must not take
// the running film's own instruction down with it.
TEST(TestApplicationContentGeometry, DisarmingLeavesThePlaybackInProgressAlone)
{
  CApplicationContentGeometry geometry;
  OpenFile(geometry);
  geometry.SetOverrides(Stated(2.40f, OsdPlacement::Picture, 2.35f));

  geometry.ClearPendingOverrides();

  EXPECT_FLOAT_EQ(2.40f, geometry.GetOverrides().rasterAspect);
  EXPECT_EQ(OsdPlacement::Picture,
            geometry.GetOverrides().osdPlacement.value_or(OsdPlacement::Raster));
  EXPECT_FLOAT_EQ(2.35f, geometry.GetOverrides().maintainAspect);
}

// The mid-playback half. Stated against the stream that is already running, so it applies at
// once rather than waiting for anything.
TEST(TestApplicationContentGeometry, AnInstructionGivenDuringPlaybackAppliesImmediately)
{
  CApplicationContentGeometry geometry;
  OpenFile(geometry);

  geometry.SetOverrides(Stated(0.0f, OsdPlacement::Picture, 1.78f));

  EXPECT_EQ(OsdPlacement::Picture,
            geometry.GetOverrides().osdPlacement.value_or(OsdPlacement::Raster));
  EXPECT_FLOAT_EQ(1.78f, geometry.GetOverrides().maintainAspect);
}

// The one that was missing, and the reason this file exists. An override outliving its film
// holds the room in that film's shape over the interface, with nothing on screen to explain it.
TEST(TestApplicationContentGeometry, StoppingRevertsEveryOverride)
{
  CApplicationContentGeometry geometry;
  OpenFile(geometry);
  geometry.SetOverrides(Stated(2.40f, OsdPlacement::Picture, 2.35f));
  ASSERT_TRUE(geometry.GetOverrides().Any());

  geometry.Clear();

  EXPECT_FALSE(geometry.GetOverrides().Any());
  EXPECT_FLOAT_EQ(0.0f, geometry.GetOverrides().rasterAspect);
  EXPECT_FALSE(geometry.GetOverrides().osdPlacement.has_value());
  EXPECT_FLOAT_EQ(0.0f, geometry.GetOverrides().maintainAspect);
}

// The picture's rectangle is the one thing an automation cannot work out for itself: a raster
// and a maintained ratio decide it together, and neither of those ratios is where it landed.
TEST(TestApplicationContentGeometry, WhereThePictureWasDrawnIsRememberedForWhoeverAsks)
{
  CApplicationContentGeometry geometry;
  EXPECT_FALSE(geometry.Drawn().Drawn());

  DrawnGeometry drawn;
  drawn.picture = CRect{173.0f, 237.0f, 1589.0f, 827.0f};
  drawn.raster = CRect{173.0f, 0.0f, 1589.0f, 1064.0f};
  geometry.SetDrawn(drawn);

  EXPECT_TRUE(geometry.Drawn().Drawn());
  EXPECT_EQ(drawn.picture, geometry.Drawn().picture);
  EXPECT_EQ(drawn.raster, geometry.Drawn().raster);
}

// This arrives once a frame, mapped through fractions of the frame, so an unmoved picture can
// still arrive differing in the last decimal. The screen has whole pixels, and a consumer moving
// a motorised mask must be told of the picture moving rather than of the arithmetic.
TEST(TestApplicationContentGeometry, TheDrawnPictureIsPublishedInWholePixels)
{
  CApplicationContentGeometry geometry;

  DrawnGeometry drawn;
  drawn.picture = CRect{172.6f, 237.4f, 1589.3f, 826.5f};
  drawn.raster = CRect{172.5f, 0.0f, 1588.5f, 1064.0f};
  geometry.SetDrawn(drawn);

  EXPECT_EQ(CRect(173.0f, 237.0f, 1589.0f, 827.0f), geometry.Drawn().picture);
  EXPECT_EQ(CRect(173.0f, 0.0f, 1589.0f, 1064.0f), geometry.Drawn().raster);
}

// Stopping leaves nothing on the screen, and the film's rectangle describes nowhere. Dropped as
// playback ends rather than at the next frame, so that the announcement stopping produces cannot
// carry a screen the film has already left.
TEST(TestApplicationContentGeometry, StoppingForgetsWhereThePictureWas)
{
  CApplicationContentGeometry geometry;
  OpenFile(geometry);

  DrawnGeometry drawn;
  drawn.picture = CRect{0.0f, 165.0f, 1762.0f, 899.0f};
  drawn.raster = CRect{0.0f, 0.0f, 1762.0f, 1064.0f};
  geometry.SetDrawn(drawn);
  ASSERT_TRUE(geometry.Drawn().Drawn());

  geometry.Clear();

  EXPECT_FALSE(geometry.Drawn().Drawn());
}

// A request to play that never reached a first frame must not leave its instruction armed for
// whatever plays next.
TEST(TestApplicationContentGeometry, StoppingAlsoDiscardsAnInstructionThatNeverOpened)
{
  CApplicationContentGeometry geometry;
  geometry.SetPendingOverrides(Stated(2.40f, OsdPlacement::Picture, 2.35f));

  geometry.Clear();
  OpenFile(geometry);

  EXPECT_FALSE(geometry.GetOverrides().Any());
}


/*!
 * What the masking was opened to at playback start, and therefore where it physically sits for
 * the whole title. A live reading can be larger than it - a measurement samples a few dozen
 * points and can have got the extent wrong - and the picture is then scaled down inside the
 * opening rather than spilling onto the mask.
 */
TEST(TestApplicationContentGeometry, TheMaskOpeningIsTheStoredEnvelope)
{
  CApplicationContentGeometry geometry;
  geometry.SetFileInputs(MeasuredScope(), {}, 0.0f);

  // The extent, not the dominant ratio: the body is 2.40, and opening the mask to that would
  // clip the taller sequence the measurement found.
  EXPECT_NEAR(1920.0f / 940.0f, geometry.GetRenderInputs().maskAspect, 0.001f);
  EXPECT_GT(2.4f, geometry.GetRenderInputs().maskAspect)
      << "the mask was opened to the dominant ratio";
}

TEST(TestApplicationContentGeometry, ThereIsNoMaskOpeningWithoutAMeasurement)
{
  CApplicationContentGeometry geometry;
  OpenFile(geometry);

  // Nothing was measured, so nothing was told to the room and nothing bounds the picture.
  EXPECT_FLOAT_EQ(0.0f, geometry.GetRenderInputs().maskAspect);
}

TEST(TestApplicationContentGeometry, AFailedMeasurementOpensNothing)
{
  ContentGeometryLookup failed{MeasuredScope()};
  failed.record.hasReading = false;

  CApplicationContentGeometry geometry;
  geometry.SetFileInputs(failed, {}, 0.0f);

  EXPECT_FLOAT_EQ(0.0f, geometry.GetRenderInputs().maskAspect);
}

TEST(TestApplicationContentGeometry, StoppingClosesTheMaskOpening)
{
  CApplicationContentGeometry geometry;
  geometry.SetFileInputs(MeasuredScope(), {}, 0.0f);
  geometry.Clear();

  EXPECT_FLOAT_EQ(0.0f, geometry.GetRenderInputs().maskAspect);
}

//! \brief What a watch learned, held for whoever writes it back once the playback ends.
TEST(TestApplicationContentGeometry, DiscoveredShapesSurviveUntilTheFileChanges)
{
  const std::vector<CRectInt> found{CRectInt{0, 0, 1920, 1080}};

  CApplicationContentGeometry geometry;
  geometry.SetFileInputs(MeasuredScope(), {}, 0.0f);
  geometry.SetLive(found.front(), false, found);

  EXPECT_EQ(found, geometry.Discovered());

  // The next file has learned nothing yet, and must not inherit the last one's findings.
  geometry.SetFileInputs(MeasuredScope(), {}, 0.0f);
  EXPECT_TRUE(geometry.Discovered().empty());
}

//! The render path reads these together under one lock rather than asking three times, so they
//! have to be the same answers the accessors they replaced give.
TEST(TestApplicationContentGeometry, TheRenderInputsAgreeWithTheAccessorsTheyReplace)
{
  CApplicationContentGeometry geometry;
  geometry.SetFileInputs(MeasuredScope(), {}, 0.0f);
  geometry.SetOverrides(Stated(2.40f, OsdPlacement::Picture, 2.35f));

  const CApplicationContentGeometry::RenderInputs inputs{geometry.GetRenderInputs()};

  EXPECT_FLOAT_EQ(geometry.GetOverrides().maintainAspect, inputs.maintainAspect);
  EXPECT_EQ(geometry.Get().codedFrame, inputs.geometry.codedFrame);
  EXPECT_FLOAT_EQ(geometry.Get().aspect, inputs.geometry.aspect);
  EXPECT_FLOAT_EQ(geometry.Get().par, inputs.geometry.par);
  EXPECT_EQ(geometry.Get().orientation, inputs.geometry.orientation);
}

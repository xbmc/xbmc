/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/AspectRatioVocabulary.h"
#include "video/geometry/GeometryPublication.h"
#include "video/geometry/test/GeometryTestHelpers.h"

#include <gtest/gtest.h>

using namespace KODI::VIDEO::GEOMETRY;
using namespace KODI::VIDEO::GEOMETRY::TEST;
using namespace KODI::UTILS;

TEST(TestGeometryPublication, StatingOneOverrideLeavesTheOthersAlone)
{
  // The read-modify-write an automation depends on: driving the lens must not silently
  // withdraw an OSD placement stated earlier in the same playback.
  GeometryOverrides overrides;
  overrides.rasterAspect = 2.40f;
  overrides.osdPlacement = OsdPlacement::Picture;
  overrides.maintainAspect = 2.35f;

  CVariant geometry{CVariant::VariantTypeObject};
  geometry["raster"] = 1.78;
  ParseGeometryOverrides(geometry, overrides);

  EXPECT_FLOAT_EQ(1.78f, overrides.rasterAspect);
  EXPECT_EQ(OsdPlacement::Picture, overrides.osdPlacement.value_or(OsdPlacement::Raster));
  EXPECT_FLOAT_EQ(2.35f, overrides.maintainAspect);
}

TEST(TestGeometryPublication, AnOverrideIsWithdrawnByZeroOrNullWithoutEndingPlayback)
{
  GeometryOverrides overrides;
  overrides.rasterAspect = 2.40f;
  overrides.osdPlacement = OsdPlacement::Picture;
  overrides.maintainAspect = 2.35f;

  CVariant geometry{CVariant::VariantTypeObject};
  geometry["raster"] = 0;
  geometry["maintain"] = 0;
  geometry["osdplacement"] = "setting";
  ParseGeometryOverrides(geometry, overrides);

  // All three back to "use the setting", which is what Any() is asking about.
  EXPECT_FLOAT_EQ(0.0f, overrides.rasterAspect);
  EXPECT_FLOAT_EQ(0.0f, overrides.maintainAspect);
  EXPECT_FALSE(overrides.osdPlacement.has_value());
  EXPECT_FALSE(overrides.Any());
}

// Over JSON-RPC a caller's omitted properties arrive filled in from the service description, so
// an explicit null is the only thing that can mean "not stated" - and it must leave the field
// alone rather than withdraw it, or driving one override wipes the others.
TEST(TestGeometryPublication, ANullFieldIsNotAWithdrawal)
{
  GeometryOverrides overrides;
  overrides.rasterAspect = 2.40f;
  overrides.osdPlacement = OsdPlacement::Picture;
  overrides.maintainAspect = 2.35f;

  CVariant geometry{CVariant::VariantTypeObject};
  geometry["raster"] = CVariant{CVariant::VariantTypeNull};
  geometry["osdplacement"] = CVariant{CVariant::VariantTypeNull};
  geometry["maintain"] = 1.78;
  ParseGeometryOverrides(geometry, overrides);

  EXPECT_FLOAT_EQ(1.78f, overrides.maintainAspect);
  EXPECT_FLOAT_EQ(2.40f, overrides.rasterAspect);
  EXPECT_EQ(OsdPlacement::Picture, overrides.osdPlacement.value_or(OsdPlacement::Raster));
}

TEST(TestGeometryPublication, OsdPlacementNamesTheTwoThingsItCanBeLaidOutAgainst)
{
  GeometryOverrides overrides;

  CVariant picture{CVariant::VariantTypeObject};
  picture["osdplacement"] = "picture";
  ParseGeometryOverrides(picture, overrides);
  EXPECT_EQ(OsdPlacement::Picture, overrides.osdPlacement.value_or(OsdPlacement::Raster));

  CVariant raster{CVariant::VariantTypeObject};
  raster["osdplacement"] = "raster";
  ParseGeometryOverrides(raster, overrides);
  EXPECT_EQ(OsdPlacement::Raster, overrides.osdPlacement.value_or(OsdPlacement::Picture));

  // The names are the whole of the encoding a caller sees, and they round-trip.
  EXPECT_STREQ("picture", OsdPlacementName(OsdPlacement::Picture));
  EXPECT_STREQ("raster", OsdPlacementName(OsdPlacement::Raster));
  EXPECT_EQ(OsdPlacement::Picture, OsdPlacementFromName("picture").value_or(OsdPlacement::Raster));
  EXPECT_EQ(OsdPlacement::Raster, OsdPlacementFromName("raster").value_or(OsdPlacement::Picture));
  EXPECT_FALSE(OsdPlacementFromName("setting").has_value());
}

TEST(TestGeometryPublication, NothingStatedChangesNothing)
{
  GeometryOverrides overrides;
  overrides.rasterAspect = 2.40f;

  // An absent options object, and an empty one, must both be no-ops rather than a reset - a
  // Player.Open carrying no geometry cannot be allowed to withdraw a standing instruction.
  ParseGeometryOverrides(CVariant{CVariant::VariantTypeNull}, overrides);
  ParseGeometryOverrides(CVariant{CVariant::VariantTypeObject}, overrides);

  EXPECT_FLOAT_EQ(2.40f, overrides.rasterAspect);
}

//! varies says one rectangle does not describe the title; the sections are what a consumer
//! acts on it with, so the two must not be able to disagree about whether they are there.
TEST(TestGeometryPublication, SectionsReachTheSerialisedForm)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();
  inputs.cached = Cached(CRectInt{0, 280, 3840, 1880}, CRectInt{0, 0, 3840, 2160}, true);
  inputs.sections = {CRectInt{0, 280, 3840, 1880}, CRectInt{0, 0, 3840, 2160}};

  CVariant value;
  SerializeEffectiveGeometry(ResolveEffectiveGeometry(inputs), value);

  ASSERT_TRUE(value["sections"].isArray());
  ASSERT_EQ(2u, value["sections"].size());
  EXPECT_TRUE(value["varies"].asBoolean());

  // Near rather than exact: the rectangle is the 2.40 entry fitted to the frame, and the
  // entry's ratio is a float, so the division carries its representation error - a fraction
  // of a thousandth of a pixel, which nothing physical can observe.
  EXPECT_NEAR(0.0, value["sections"][0]["x"].asDouble(), 0.001);
  EXPECT_NEAR(280.0, value["sections"][0]["y"].asDouble(), 0.001);
  EXPECT_NEAR(3840.0, value["sections"][0]["width"].asDouble(), 0.001);
  EXPECT_NEAR(1600.0, value["sections"][0]["height"].asDouble(), 0.001);
  EXPECT_EQ("2.40", value["sections"][0]["label"].asString());

  EXPECT_EQ(2160.0, value["sections"][1]["height"].asDouble());
  EXPECT_EQ("1.78", value["sections"][1]["label"].asString());
}

//! Both of these serve the frame and both report container. If the serialised form did not
//! carry the rejection, a consumer could not tell the file that needs measuring from the one
//! whose detector failed on it.
TEST(TestGeometryPublication, ARefusedMeasurementIsDistinguishableOnTheWireFromNoneAtAll)
{
  const GeometryInputs refused = RefusedCachedUhd();

  CVariant value;
  SerializeEffectiveGeometry(ResolveEffectiveGeometry(refused), value);

  EXPECT_EQ("container", value["source"].asString());
  EXPECT_TRUE(value["rejected"].asBoolean());

  GeometryInputs unmeasured;
  unmeasured.stream = Uhd();

  CVariant never;
  SerializeEffectiveGeometry(ResolveEffectiveGeometry(unmeasured), never);

  EXPECT_EQ("container", never["source"].asString());
  EXPECT_FALSE(never["rejected"].asBoolean());
}

//! An unmeasured file has no sections, and the array is still written - an absent one would
//! be a third state on top of the empty one.
TEST(TestGeometryPublication, TheSectionsArrayIsWrittenEvenWhenNothingWasMeasured)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();

  CVariant value;
  SerializeEffectiveGeometry(ResolveEffectiveGeometry(inputs), value);

  ASSERT_TRUE(value["sections"].isArray());
  EXPECT_EQ(0u, value["sections"].size());
  EXPECT_EQ("container", value["source"].asString());
}

//! What Kodi reports while nothing is playing. There is no picture and no resolution behind
//! one, so a rectangle here would be invented - and zeros are a rectangle a consumer can act
//! on, which is why the fields are absent rather than empty.
TEST(TestGeometryPublication, AShapeWithNoFrameIsSerialisedWithoutAnyPixels)
{
  EffectiveGeometry atRest;
  atRest.aspect = 2.40f;
  atRest.label = CAspectRatioVocabulary::Label(atRest.aspect);
  atRest.name = CAspectRatioVocabulary::Name(atRest.aspect);

  CVariant value;
  SerializeEffectiveGeometry(atRest, value);

  EXPECT_FALSE(value.isMember("x"));
  EXPECT_FALSE(value.isMember("y"));
  EXPECT_FALSE(value.isMember("width"));
  EXPECT_FALSE(value.isMember("height"));
  EXPECT_FALSE(value.isMember("framewidth"));
  EXPECT_FALSE(value.isMember("frameheight"));

  // The shape itself is always there, and is what a mask or a lens is driven from.
  EXPECT_DOUBLE_EQ(2.4, value["aspect"].asDouble());
  EXPECT_EQ("2.40", value["label"].asString());
  EXPECT_EQ("Scope", value["name"].asString());
  EXPECT_EQ("container", value["source"].asString());
  ASSERT_TRUE(value["sections"].isArray());
  EXPECT_EQ(0u, value["sections"].size());
}

//! Both rectangles, because they answer different instructions: a mask is driven to the lit
//! picture and a lens to the area Kodi is operating in, and the two coincide only when the
//! content is exactly the shape of the room.
TEST(TestGeometryPublication, WhatWasDrawnIsSerialisedAsTwoRectangles)
{
  DrawnGeometry drawn;
  drawn.picture = CRect{173.0f, 237.0f, 1589.0f, 827.0f};
  drawn.raster = CRect{173.0f, 0.0f, 1589.0f, 1064.0f};

  CVariant value;
  SerializeDrawnGeometry(drawn, value);

  EXPECT_DOUBLE_EQ(173.0, value["picture"]["x"].asDouble());
  EXPECT_DOUBLE_EQ(237.0, value["picture"]["y"].asDouble());
  EXPECT_DOUBLE_EQ(1416.0, value["picture"]["width"].asDouble());
  EXPECT_DOUBLE_EQ(590.0, value["picture"]["height"].asDouble());

  EXPECT_DOUBLE_EQ(173.0, value["raster"]["x"].asDouble());
  EXPECT_DOUBLE_EQ(0.0, value["raster"]["y"].asDouble());
  EXPECT_DOUBLE_EQ(1416.0, value["raster"]["width"].asDouble());
  EXPECT_DOUBLE_EQ(1064.0, value["raster"]["height"].asDouble());
}

//! A measured geometry keeps every pixel it always had, so the omission above cannot start
//! applying to a stream that has a frame.
TEST(TestGeometryPublication, AMeasuredGeometryStillCarriesItsRectangle)
{
  GeometryInputs inputs = ScopeCachedUhd();

  CVariant value;
  SerializeEffectiveGeometry(ResolveEffectiveGeometry(inputs), value);

  // Near rather than exact for the fitted rectangle: the entry's float ratio puts its
  // representation error - well under a thousandth of a pixel - into the division.
  EXPECT_NEAR(0.0, value["x"].asDouble(), 0.001);
  EXPECT_NEAR(280.0, value["y"].asDouble(), 0.001);
  EXPECT_NEAR(3840.0, value["width"].asDouble(), 0.001);
  EXPECT_NEAR(1600.0, value["height"].asDouble(), 0.001);
  EXPECT_EQ(3840.0, value["framewidth"].asDouble());
  EXPECT_EQ(2160.0, value["frameheight"].asDouble());
}

//! A float widened to a double serialises as sixteen digits of its own representation error,
//! which a caller cannot compare against the ratio it asked for.
TEST(TestGeometryPublication, ThePublishedRatioIsRoundedToTheVocabularysPrecision)
{
  GeometryInputs inputs;
  inputs.stream = Uhd();

  CVariant value;
  SerializeEffectiveGeometry(ResolveEffectiveGeometry(inputs), value);

  EXPECT_DOUBLE_EQ(1.7778, value["aspect"].asDouble());
}

/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "test/TestUtils.h"
#include "video/geometry/ContentGeometryRecord.h"
#include "video/geometry/test/GeometryTestHelpers.h"

#include <gtest/gtest.h>

using namespace KODI::VIDEO::GEOMETRY;
using namespace KODI::VIDEO::GEOMETRY::TEST;

TEST(TestFileIdentity, AnIdentityMatchesItself)
{
  const FileIdentity identity{1234567890, 1700000000};

  EXPECT_TRUE(identity.IsKnown());
  EXPECT_TRUE(identity.Matches(identity));
}

TEST(TestFileIdentity, EitherFieldDifferingIsADifferentFile)
{
  const FileIdentity stored{1234567890, 1700000000};

  EXPECT_FALSE(stored.Matches({1234567891, 1700000000}));
  EXPECT_FALSE(stored.Matches({1234567890, 1700000001}));
}

//! Neither field alone: a re-crop can preserve the byte count, and a restore can preserve
//! the size while changing the mtime.
TEST(TestFileIdentity, NeitherFieldAloneDecides)
{
  const FileIdentity recropped{1234567890, 1700009999};
  const FileIdentity restored{9999999999, 1700000000};
  const FileIdentity original{1234567890, 1700000000};

  EXPECT_FALSE(original.Matches(recropped));
  EXPECT_FALSE(original.Matches(restored));
}

//! An unknown identity never matches, so a caller that could not identify the file ends up
//! with no rectangle.
TEST(TestFileIdentity, AnUnknownIdentityMatchesNothingIncludingItself)
{
  const FileIdentity unknown;
  const FileIdentity known{100, 200};

  EXPECT_FALSE(unknown.IsKnown());
  EXPECT_FALSE(unknown.Matches(unknown));
  EXPECT_FALSE(unknown.Matches(known));
  EXPECT_FALSE(known.Matches(unknown));
}

TEST(TestFileIdentity, PartiallyKnownIsUnknown)
{
  EXPECT_FALSE((FileIdentity{100, -1}).IsKnown());
  EXPECT_FALSE((FileIdentity{-1, 200}).IsKnown());
}

TEST(TestFileIdentity, AMissingFileHasNoIdentity)
{
  EXPECT_FALSE(GetFileIdentity("special://temp/no-such-file-content-geometry.mkv").IsKnown());
}

TEST(TestFileIdentity, ARealFileHasOneAndItIsStable)
{
  XFILE::CFile* file{XBMC_CREATETEMPFILE(".mkv")};
  ASSERT_NE(nullptr, file);
  const std::string path{XBMC_TEMPFILEPATH(file)};

  const FileIdentity identity{GetFileIdentity(path)};
  EXPECT_TRUE(identity.IsKnown());

  // Reading it twice must give the same answer.
  EXPECT_TRUE(identity.Matches(GetFileIdentity(path)));

  EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
}

TEST(TestContentAspects, RoundTripThroughTheStoredForm)
{
  const std::vector<float> aspects{2.35f, 1.78f};

  EXPECT_EQ("2.35;1.78", EncodeContentAspects(aspects));
  EXPECT_EQ(aspects, DecodeContentAspects(EncodeContentAspects(aspects)));
}

TEST(TestContentAspects, NoRatiosIsAnEmptyValue)
{
  EXPECT_TRUE(EncodeContentAspects({}).empty());
  EXPECT_TRUE(DecodeContentAspects("").empty());
}

//! The column is plain text a user can edit, so a value naming no ratio costs only itself.
TEST(TestContentAspects, AnythingThatIsNotARatioIsSkipped)
{
  const std::vector<float> scope{2.40f};
  EXPECT_EQ(scope, DecodeContentAspects("2.40;wide;0;-1.78;nan;inf"));
}

TEST(TestContentAspects, ARatioIsHeldToTwoDecimals)
{
  EXPECT_FLOAT_EQ(2.35f, StoredAspect(3840.0f / 1632.0f));
  const std::vector<float> rounded{2.35f};
  EXPECT_EQ(rounded, DecodeContentAspects("2.3529"));
}

TEST(TestContentGeometryRecord, DefaultsToTheCurrentAlgorithmVersion)
{
  EXPECT_EQ(CONTENT_GEOMETRY_ALGORITHM_VERSION, ContentGeometryRecord{}.algorithmVersion);
}

TEST(TestContentGeometryRecord, AReadingIsARatioAndVaryingIsMoreThanOne)
{
  ContentGeometryRecord record;
  EXPECT_FALSE(record.HasReading());
  EXPECT_FALSE(record.Varies());

  record.aspects = {2.40f};
  EXPECT_TRUE(record.HasReading());
  EXPECT_FALSE(record.Varies());

  record.aspects.push_back(1.78f);
  EXPECT_TRUE(record.Varies());
  EXPECT_FLOAT_EQ(2.40f, WidestAspect(record));
}

TEST(TestContentGeometryLookup, MissingCarriesNoRecord)
{
  EXPECT_FALSE(ContentGeometryLookup{}.HasRecord());
  EXPECT_EQ(ContentGeometryState::MISSING, ContentGeometryLookup{}.state);
}

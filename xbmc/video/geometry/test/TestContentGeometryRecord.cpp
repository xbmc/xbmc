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

namespace
{

ContentGeometryDetails MakeDetails()
{
  ContentGeometryDetails details;
  details.detector = "contentbar";
  details.usable = 7;
  details.discarded = 2;
  details.unreadable = 3;
  // Every field off its default, so a field the codec drops fails the round trip.
  details.combining.tolerance = 6;
  details.combining.minConfidence = 0.10f;
  details.combining.minStationarySamples = 5;
  details.combining.variesShare = 0.25f;
  details.combining.minRivalSamples = 4;
  details.combining.distinctAspectShare = 0.08f;
  details.sampling.points = 11;
  details.sampling.leadInSeconds = 90.0;
  details.sampling.leadOutSeconds = 45.0;
  details.sampling.shortTitleWindow = 0.70;
  details.sampling.picturesPerPoint = 2;
  details.sampling.escalatedPoints = 33;
  details.sampling.escalateDiscardShare = 0.50f;
  details.samples.push_back({CRectInt{0, 20, 320, 220}, 0.9f, false, 61.5});
  details.samples.push_back({CRectInt{0, 0, 320, 240}, 0.1f, true, 122.0});
  details.clusters.push_back({CRectInt{0, 20, 320, 220}, 6, 5.4f});

  return details;
}


constexpr int WIDTH = 1920;
constexpr int HEIGHT = 1080;

//! \brief A scanned record of a scope title: one shape, envelope equal to the rectangle. Read
//! back, so it carries no diagnostics - which is the state every merge starts from.
ContentGeometryRecord ScannedScope()
{
  ContentGeometryRecord record = ScopeHdRecord();
  record.sections.push_back(record.rect);
  return record;
}

} // unnamed namespace

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

TEST(TestContentGeometryDetails, RoundTripsThroughJson)
{
  const ContentGeometryDetails original{MakeDetails()};
  const ContentGeometryDetails decoded{
      DecodeContentGeometryDetails(EncodeContentGeometryDetails(original))};

  EXPECT_EQ(original.detector, decoded.detector);
  EXPECT_EQ(original.usable, decoded.usable);
  EXPECT_EQ(original.discarded, decoded.discarded);

  //! The count that closes the accounting: without it a stored row reporting eight readings
  //! from nine points cannot be told from arithmetic that does not add up.
  EXPECT_EQ(original.unreadable, decoded.unreadable);
  EXPECT_EQ(original.combining.tolerance, decoded.combining.tolerance);
  EXPECT_FLOAT_EQ(original.combining.minConfidence, decoded.combining.minConfidence);
  EXPECT_EQ(original.combining.minStationarySamples, decoded.combining.minStationarySamples);
  EXPECT_FLOAT_EQ(original.combining.variesShare, decoded.combining.variesShare);
  EXPECT_EQ(original.combining.minRivalSamples, decoded.combining.minRivalSamples);
  EXPECT_FLOAT_EQ(original.combining.distinctAspectShare, decoded.combining.distinctAspectShare);
  EXPECT_EQ(original.sampling.points, decoded.sampling.points);
  EXPECT_DOUBLE_EQ(original.sampling.leadInSeconds, decoded.sampling.leadInSeconds);
  EXPECT_DOUBLE_EQ(original.sampling.leadOutSeconds, decoded.sampling.leadOutSeconds);
  EXPECT_DOUBLE_EQ(original.sampling.shortTitleWindow, decoded.sampling.shortTitleWindow);
  EXPECT_EQ(original.sampling.picturesPerPoint, decoded.sampling.picturesPerPoint);
  EXPECT_EQ(original.sampling.escalatedPoints, decoded.sampling.escalatedPoints);
  EXPECT_FLOAT_EQ(original.sampling.escalateDiscardShare, decoded.sampling.escalateDiscardShare);

  ASSERT_EQ(original.samples.size(), decoded.samples.size());
  for (size_t i = 0; i < original.samples.size(); ++i)
  {
    EXPECT_EQ(original.samples[i].rect, decoded.samples[i].rect);
    EXPECT_FLOAT_EQ(original.samples[i].confidence, decoded.samples[i].confidence);
    EXPECT_EQ(original.samples[i].degenerate, decoded.samples[i].degenerate);
    EXPECT_DOUBLE_EQ(original.samples[i].position, decoded.samples[i].position);
  }

  ASSERT_EQ(original.clusters.size(), decoded.clusters.size());
  EXPECT_EQ(original.clusters[0].rect, decoded.clusters[0].rect);
  EXPECT_EQ(original.clusters[0].samples, decoded.clusters[0].samples);
  EXPECT_FLOAT_EQ(original.clusters[0].weight, decoded.clusters[0].weight);
}

//! A corrupt blob costs the diagnostics it carried and nothing more.
TEST(TestContentGeometryDetails, GarbageDecodesToDefaults)
{
  for (const std::string& json : {std::string{}, std::string{"not json"}, std::string{"[1,2,3]"},
                                  std::string{R"({"samples": 4})"}})
  {
    const ContentGeometryDetails decoded{DecodeContentGeometryDetails(json)};
    EXPECT_TRUE(decoded.detector.empty());
    EXPECT_TRUE(decoded.samples.empty());
    EXPECT_TRUE(decoded.clusters.empty());
  }
}

TEST(TestGeometrySections, RoundTripThroughTheStoredForm)
{
  const std::vector<CRectInt> sections{CRectInt{0, 140, 1920, 940}, CRectInt{240, 0, 1680, 1080}};

  EXPECT_EQ("0,140,1920,800;240,0,1440,1080", EncodeGeometrySections(sections));
  EXPECT_EQ(sections, DecodeGeometrySections(EncodeGeometrySections(sections)));
}

//! A stereoscopic scan measures one view, whose frame is an offset region of the picture, so
//! an origin left of the frame's is a real measurement rather than a corrupt one.
TEST(TestGeometrySections, ANegativeOriginSurvives)
{
  const std::vector<CRectInt> sections{CRectInt{-120, -8, 840, 568}};

  EXPECT_EQ(sections, DecodeGeometrySections(EncodeGeometrySections(sections)));
}

TEST(TestGeometrySections, NoSectionsIsAnEmptyValue)
{
  EXPECT_TRUE(EncodeGeometrySections({}).empty());
  EXPECT_TRUE(DecodeGeometrySections("").empty());
}

//! A value that stops making sense costs the shapes past that point rather than the record.
TEST(TestGeometrySections, AMalformedValueKeepsWhatParsed)
{
  EXPECT_TRUE(DecodeGeometrySections("not a rectangle").empty());
  EXPECT_TRUE(DecodeGeometrySections("0,140,1920").empty()) << "a short shape is not a shape";

  const std::vector<CRectInt> first{CRectInt{0, 140, 1920, 940}};
  EXPECT_EQ(first, DecodeGeometrySections("0,140,1920,800;240,0"));
}

/*!
 * The column is plain text a user can edit, and the far edge of a shape is an origin plus a
 * size - so two values each inside the range can still add to something outside it.
 */
TEST(TestGeometrySections, AnExtentOutsideTheRangeIsRefused)
{
  EXPECT_TRUE(DecodeGeometrySections("2000000000,0,2000000000,1080").empty());
  EXPECT_TRUE(DecodeGeometrySections("0,2000000000,1920,2000000000").empty());
  EXPECT_TRUE(DecodeGeometrySections("-2000000000,0,-2000000000,1080").empty());

  // A value outside the range on its own was already refused, and still is.
  EXPECT_TRUE(DecodeGeometrySections("99999999999,0,1920,1080").empty());

  // The shapes read before the impossible one are kept, as with any other malformed tail.
  const std::vector<CRectInt> first{CRectInt{0, 140, 1920, 940}};
  EXPECT_EQ(first, DecodeGeometrySections("0,140,1920,800;2000000000,0,2000000000,1080"));
}

//! Either separator is accepted wherever it appears - see DecodeGeometrySections().
TEST(TestGeometrySections, EitherSeparatorReadsTheSameShapes)
{
  const std::vector<CRectInt> sections{CRectInt{0, 140, 1920, 940}, CRectInt{240, 0, 1920, 1080}};

  EXPECT_EQ(sections, DecodeGeometrySections("0,140,1920,800;240,0,1680,1080"));
  EXPECT_EQ(sections, DecodeGeometrySections("0;140;1920;800,240,0,1680,1080"));
}

TEST(TestContentGeometryRecord, DefaultsToTheCurrentAlgorithmVersion)
{
  EXPECT_EQ(CONTENT_GEOMETRY_ALGORITHM_VERSION, ContentGeometryRecord{}.algorithmVersion);
}

TEST(TestContentGeometryRecord, ARecordWithoutACodedFrameIsNotValid)
{
  ContentGeometryRecord record;
  EXPECT_FALSE(record.IsValid());

  record.coded = CRectInt{0, 0, 1920, 1080};
  EXPECT_TRUE(record.IsValid());
}

TEST(TestContentGeometryLookup, MissingCarriesNoRecord)
{
  EXPECT_FALSE(ContentGeometryLookup{}.HasRecord());
  EXPECT_EQ(ContentGeometryState::MISSING, ContentGeometryLookup{}.state);
}

//! Rows written before the count existed are already in every library that has run a sweep,
//! and they decode without it. Zero is the truthful answer for them: nothing was recorded as
//! unreadable, which is exactly what those rows know.
TEST(TestContentGeometryDetails, DetailWrittenBeforeTheUnreadableCountDecodesAsZero)
{
  const ContentGeometryDetails decoded{DecodeContentGeometryDetails(
      R"({"detector":"contentbar","usable":7,"discarded":2,"samples":[],"clusters":[]})")};

  EXPECT_EQ(7u, decoded.usable);
  EXPECT_EQ(2u, decoded.discarded);
  EXPECT_EQ(0u, decoded.unreadable);
}


/*!
 * What a watch teaches a record that a scan could not. Measured on a title cut in three
 * ratios: an escalated twenty-seven point scan found one of them, and the ratio a viewer
 * actually watched for two minutes was not in the record at all.
 */
TEST(TestMergeDiscoveredGeometry, ARatioTheScanMissedIsAdded)
{
  const ContentGeometryRecord record{ScannedScope()};
  const CRectInt academy{240, 0, WIDTH - 240, HEIGHT};

  const std::optional<ContentGeometryRecord> merged{MergeDiscoveredGeometry(record, {academy})};

  ASSERT_TRUE(merged.has_value());
  ASSERT_EQ(2u, merged->sections.size());
  EXPECT_EQ(academy, merged->sections[1]);
}

/*!
 * It was watched, not sampled. The diagnostics are the scan's account of what it sampled, so
 * a watch does not get to write into them - and a merge starts from a record that was read
 * back, which carries none, so rewriting them could only erase what the scan recorded.
 */
TEST(TestMergeDiscoveredGeometry, AWatchLeavesTheDiagnosticsAlone)
{
  const std::optional<ContentGeometryRecord> merged{
      MergeDiscoveredGeometry(ScannedScope(), {CRectInt{240, 0, WIDTH - 240, HEIGHT}})};

  ASSERT_TRUE(merged.has_value());
  EXPECT_FALSE(merged->details.has_value());
}

TEST(TestMergeDiscoveredGeometry, AShapeTheRecordAlreadyHasIsNotAddedAgain)
{
  const ContentGeometryRecord record{ScannedScope()};

  // A line out from the stored cluster, which is inside the combiner's own tolerance.
  const std::optional<ContentGeometryRecord> merged{
      MergeDiscoveredGeometry(record, {CRectInt{0, 141, WIDTH, HEIGHT - 140}})};

  EXPECT_FALSE(merged.has_value()) << "nothing was learned, so nothing should be written";
}

/*!
 * The match window is inclusive, and the boundary is where a shape stops being one the record
 * already has. Eight is the combiner's tolerance, so a shape exactly that far out is the same
 * shape and a shape one further is a discovery - the case above sits a line out and would not
 * catch the comparison narrowing to <.
 */
TEST(TestMergeDiscoveredGeometry, AShapeExactlyTheToleranceOutIsTheSameShape)
{
  const std::optional<ContentGeometryRecord> onBoundary{
      MergeDiscoveredGeometry(ScannedScope(), {CRectInt{8, 148, WIDTH - 8, HEIGHT - 148}})};
  EXPECT_FALSE(onBoundary.has_value()) << "eight out is inside the window, so nothing was learned";

  const std::optional<ContentGeometryRecord> pastIt{
      MergeDiscoveredGeometry(ScannedScope(), {CRectInt{9, 149, WIDTH - 9, HEIGHT - 149}})};
  ASSERT_TRUE(pastIt.has_value()) << "nine out is a shape the record does not have";
  EXPECT_EQ(2u, pastIt->sections.size());
}

TEST(TestMergeDiscoveredGeometry, TheEnvelopeWidensOnEachAxisIndependently)
{
  // A title can contain a shape wider than anything sampled and another taller than it.
  const std::optional<ContentGeometryRecord> merged{MergeDiscoveredGeometry(
      ScannedScope(), {CRectInt{240, 0, WIDTH - 240, HEIGHT}, CRectInt{0, 300, WIDTH, HEIGHT}})};

  ASSERT_TRUE(merged.has_value());
  EXPECT_EQ(CRectInt(0, 0, WIDTH, HEIGHT), merged->envelope);
}

/*!
 * The scan samples uniformly across the runtime, which is the right instrument for what a
 * title is mostly in. Live coverage is shaped by how far the viewer watched, so it may not
 * decide that.
 */
TEST(TestMergeDiscoveredGeometry, TheDominantRatioIsNeverTouched)
{
  const ContentGeometryRecord record{ScannedScope()};

  const std::optional<ContentGeometryRecord> merged{MergeDiscoveredGeometry(
      record, {CRectInt{240, 0, WIDTH - 240, HEIGHT}, CRectInt{240, 0, WIDTH - 240, HEIGHT}})};

  ASSERT_TRUE(merged.has_value());
  EXPECT_EQ(record.rect, merged->rect);
  EXPECT_EQ(record.coded, merged->coded);
  EXPECT_EQ(record.sections[0], merged->sections[0]);
}

/*!
 * The combiner decides varies from counts and share together, deliberately - a rival of one
 * sample in twenty-seven is not a title that changes shape. Counting the shapes afterwards
 * overturns that the first time a watch widens the envelope by a pixel, which is a rewrite
 * for a different reason entirely.
 */
TEST(TestMergeDiscoveredGeometry, ARivalTheCombinerRefusedIsNotPromotedByARewrite)
{
  ContentGeometryRecord record{ScannedScope()};
  record.varies = false;
  record.sections.push_back(CRectInt{240, 0, WIDTH - 240, HEIGHT});

  // Within tolerance of the stored shape, so it teaches nothing new, but a line above the
  // stored envelope, so there is something to write.
  const std::optional<ContentGeometryRecord> merged{
      MergeDiscoveredGeometry(record, {CRectInt{0, 139, WIDTH, HEIGHT - 140}})};

  ASSERT_TRUE(merged.has_value());
  EXPECT_FALSE(merged->varies) << "counting shapes overturned the combiner's decision";
}

/*!
 * A record holding no shapes at all - an NFO import, or a measurement taken before they were
 * stored - watched through a title that is one shape throughout. The watch learns that shape,
 * which is the first the record holds rather than a second one, so the title does not vary.
 */
TEST(TestMergeDiscoveredGeometry, TheFirstShapeLearnedIsNotASecondShape)
{
  ContentGeometryRecord record{ScannedScope()};
  record.varies = false;
  record.sections.clear();

  const std::optional<ContentGeometryRecord> merged{
      MergeDiscoveredGeometry(record, {CRectInt{0, 140, WIDTH, HEIGHT - 140}})};

  ASSERT_TRUE(merged.has_value());
  ASSERT_EQ(1u, merged->sections.size());
  EXPECT_FALSE(merged->varies) << "one shape is not a title that changes shape";
}

//! \brief A record that already varies is not talked out of it by a watch that saw one shape.
//! An NFO import carries no shapes at all, so this is the state that reaches the merge.
TEST(TestMergeDiscoveredGeometry, VariesIsNeverLoweredByAWatch)
{
  ContentGeometryRecord record{ScannedScope()};
  record.varies = true;
  record.sections.clear();

  const std::optional<ContentGeometryRecord> merged{
      MergeDiscoveredGeometry(record, {CRectInt{240, 0, WIDTH - 240, HEIGHT}})};

  ASSERT_TRUE(merged.has_value());
  EXPECT_TRUE(merged->varies);
}

TEST(TestMergeDiscoveredGeometry, ASecondShapeMakesTheTitleVary)
{
  const std::optional<ContentGeometryRecord> merged{
      MergeDiscoveredGeometry(ScannedScope(), {CRectInt{240, 0, WIDTH - 240, HEIGHT}})};

  ASSERT_TRUE(merged.has_value());
  EXPECT_TRUE(merged->varies);
}

TEST(TestMergeDiscoveredGeometry, NothingSeenWritesNothing)
{
  EXPECT_FALSE(MergeDiscoveredGeometry(ScannedScope(), {}).has_value());
}

//! \brief A record that came to nothing has no set of shapes to add to.
TEST(TestMergeDiscoveredGeometry, ARecordWithNoReadingIsLeftAlone)
{
  ContentGeometryRecord failed{ScannedScope()};
  failed.hasReading = false;

  EXPECT_FALSE(MergeDiscoveredGeometry(failed, {CRectInt{240, 0, WIDTH - 240, HEIGHT}}).has_value());
}

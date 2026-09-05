/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "XBDateTime.h"
#include "video/geometry/SampledGeometry.h"

#include <gtest/gtest.h>

using namespace KODI::VIDEO::GEOMETRY;

namespace
{

constexpr int CODED_WIDTH{3840};
constexpr int CODED_HEIGHT{2160};

//! A 2.39:1 picture in a 16:9 frame.
constexpr int CONTENT_TOP{264};
constexpr int CONTENT_BOTTOM{1896};

const FileIdentity IDENTITY{8'000'000'000, 1'700'000'000};
const CDateTime COMPUTED{2026, 8, 7, 14, 0, 0};

SampledGeometry MakeScan()
{
  SampledGeometry scan;
  scan.succeeded = true;
  scan.coded = CRectInt{0, 0, CODED_WIDTH, CODED_HEIGHT};
  scan.displayAspect = 1.7777778f;
  scan.samples = {{CRectInt{0, CONTENT_TOP, CODED_WIDTH, CONTENT_BOTTOM}, 0.9f, false, 300.0},
                  {CRectInt{0, CONTENT_TOP, CODED_WIDTH, CONTENT_BOTTOM}, 0.8f, false, 900.0}};

  scan.combined.rect = CRectInt{0, CONTENT_TOP, CODED_WIDTH, CONTENT_BOTTOM};
  scan.combined.envelope = CRectInt{0, CONTENT_TOP, CODED_WIDTH, CONTENT_BOTTOM};
  scan.combined.hasReading = true;
  scan.combined.share = 0.85f;
  scan.combined.usable = 2;
  scan.combined.discarded = 0;
  scan.combined.clusters = {{CRectInt{0, CONTENT_TOP, CODED_WIDTH, CONTENT_BOTTOM}, 2, 1.7f}};

  return scan;
}

} // unnamed namespace

TEST(TestSampledGeometry, ASucceededScanBecomesAMeasurement)
{
  const ContentGeometryRecord record{MakeContentGeometryRecord(MakeScan(), IDENTITY, COMPUTED)};

  EXPECT_EQ(ContentGeometryOutcome::Measured, record.outcome);
  EXPECT_TRUE(record.IsValid());
  EXPECT_EQ(CRectInt(0, 0, CODED_WIDTH, CODED_HEIGHT), record.coded);
  EXPECT_EQ(CRectInt(0, CONTENT_TOP, CODED_WIDTH, CONTENT_BOTTOM), record.rect);
  EXPECT_FLOAT_EQ(1.7777778f, record.displayAspect);
  EXPECT_TRUE(record.hasReading);
  EXPECT_FALSE(record.varies);
  EXPECT_FLOAT_EQ(0.85f, record.confidence);
  EXPECT_EQ(CONTENT_GEOMETRY_ALGORITHM_VERSION, record.algorithmVersion);
  EXPECT_EQ(IDENTITY.size, record.identity.size);
  EXPECT_EQ(IDENTITY.time, record.identity.time);
  EXPECT_EQ(COMPUTED.GetAsDBDateTime(), record.computed.GetAsDBDateTime());
}

//! The diagnostics blob is the only place the individual readings survive, so the parameters
//! that produced them have to travel with them or the answer cannot be explained afterwards.
TEST(TestSampledGeometry, TheDetailsCarryTheReadingsAndWhatProducedThem)
{
  SampledGeometry scan{MakeScan()};
  scan.sampling.points = 27;
  scan.combining.tolerance = 12;

  const ContentGeometryRecord record{MakeContentGeometryRecord(scan, IDENTITY, COMPUTED)};
  ASSERT_TRUE(record.details.has_value());
  const ContentGeometryDetails details{DecodeContentGeometryDetails(*record.details)};

  EXPECT_EQ(CONTENT_GEOMETRY_DETECTOR, details.detector);
  EXPECT_EQ(27u, details.sampling.points);
  EXPECT_EQ(12u, details.combining.tolerance);
  EXPECT_EQ(2u, details.samples.size());
  EXPECT_EQ(1u, details.clusters.size());
  EXPECT_EQ(2u, details.usable);
  EXPECT_EQ(0u, details.discarded);
}

/*!
 * The shapes the title contains, kept where a list can read them: the same clusters are in
 * the diagnostics, but a listing answering a per-item ratio cannot afford to parse those.
 */
TEST(TestSampledGeometry, TheClustersBecomeTheSections)
{
  SampledGeometry scan{MakeScan()};
  scan.combined.clusters.push_back({CRectInt{240, 0, CODED_WIDTH - 240, CODED_HEIGHT}, 1, 0.4f});

  const ContentGeometryRecord record{MakeContentGeometryRecord(scan, IDENTITY, COMPUTED)};

  ASSERT_EQ(2u, record.sections.size());
  EXPECT_EQ(CRectInt(0, CONTENT_TOP, CODED_WIDTH, CONTENT_BOTTOM), record.sections[0]);
  EXPECT_EQ(CRectInt(240, 0, CODED_WIDTH - 240, CODED_HEIGHT), record.sections[1]);
}

/*!
 * A combiner that found nothing to take an extent of leaves the envelope empty, and an empty
 * envelope stored verbatim reads back as a measurement claiming there is no picture at all.
 */
TEST(TestSampledGeometry, AnUnsetEnvelopeFallsBackToTheRectangle)
{
  SampledGeometry scan{MakeScan()};
  scan.combined.envelope = CRectInt{};

  const ContentGeometryRecord record{MakeContentGeometryRecord(scan, IDENTITY, COMPUTED)};

  EXPECT_EQ(record.rect, record.envelope);
}

TEST(TestSampledGeometry, AVaryingTitleKeepsBothRectangles)
{
  SampledGeometry scan{MakeScan()};
  scan.combined.varies = true;
  scan.combined.envelope = CRectInt{0, 0, CODED_WIDTH, CODED_HEIGHT};

  const ContentGeometryRecord record{MakeContentGeometryRecord(scan, IDENTITY, COMPUTED)};

  EXPECT_TRUE(record.varies);
  EXPECT_EQ(CRectInt(0, CONTENT_TOP, CODED_WIDTH, CONTENT_BOTTOM), record.rect);
  EXPECT_EQ(CRectInt(0, 0, CODED_WIDTH, CODED_HEIGHT), record.envelope);
}

/*!
 * A file that could not be read becomes a row rather than nothing, so that a sweep over a
 * large library does not attempt it again every time. It carries no rectangle, because none
 * was ever measured.
 */
TEST(TestSampledGeometry, AFailedScanBecomesAFailureRecord)
{
  const SampledGeometry scan; // never opened

  const ContentGeometryRecord record{MakeContentGeometryRecord(scan, IDENTITY, COMPUTED)};

  EXPECT_EQ(ContentGeometryOutcome::Failed, record.outcome);
  EXPECT_FALSE(record.IsValid());
  EXPECT_FALSE(record.hasReading);
  EXPECT_TRUE(record.sections.empty());

  // Stated as empty rather than left unsaid, so that storing this clears the diagnostics an
  // earlier measurement of the same file left behind.
  ASSERT_TRUE(record.details.has_value());
  EXPECT_TRUE(record.details->empty());

  EXPECT_EQ(IDENTITY.size, record.identity.size);
  EXPECT_EQ(IDENTITY.time, record.identity.time);
}

//! A scan that opened the file but whose samples all came to nothing is not a failure: the
//! coded frame is known and is the honest answer, and it should not be measured again.
TEST(TestSampledGeometry, AScanThatFoundNoBarsIsStillAMeasurement)
{
  SampledGeometry scan{MakeScan()};
  scan.samples.clear();
  scan.combined = {};
  scan.combined.rect = scan.coded;
  scan.combined.hasReading = false;
  scan.combined.discarded = 9;

  const ContentGeometryRecord record{MakeContentGeometryRecord(scan, IDENTITY, COMPUTED)};

  EXPECT_EQ(ContentGeometryOutcome::Measured, record.outcome);
  EXPECT_TRUE(record.IsValid());
  EXPECT_FALSE(record.hasReading);
  EXPECT_EQ(scan.coded, record.rect);
}

TEST(TestSampledGeometry, NothingIsNeededForAnUpToDateMeasurement)
{
  ContentGeometryAttempt attempt;
  attempt.exists = true;
  attempt.algorithmVersion = CONTENT_GEOMETRY_ALGORITHM_VERSION;
  attempt.identity = IDENTITY;

  EXPECT_FALSE(NeedsContentGeometry(attempt, IDENTITY));
}

TEST(TestSampledGeometry, AFileWithNoAttemptNeedsMeasuring)
{
  EXPECT_TRUE(NeedsContentGeometry({}, IDENTITY));
}

TEST(TestSampledGeometry, ASupersededMeasurementNeedsMeasuringAgain)
{
  ContentGeometryAttempt attempt;
  attempt.exists = true;
  attempt.algorithmVersion = CONTENT_GEOMETRY_ALGORITHM_VERSION - 1;
  attempt.identity = IDENTITY;

  EXPECT_TRUE(NeedsContentGeometry(attempt, IDENTITY));
}

TEST(TestSampledGeometry, AMeasurementOfDifferentContentNeedsMeasuringAgain)
{
  ContentGeometryAttempt attempt;
  attempt.exists = true;
  attempt.algorithmVersion = CONTENT_GEOMETRY_ALGORITHM_VERSION;
  attempt.identity = IDENTITY;

  EXPECT_TRUE(NeedsContentGeometry(attempt, {IDENTITY.size + 1, IDENTITY.time}));
  EXPECT_TRUE(NeedsContentGeometry(attempt, {IDENTITY.size, IDENTITY.time + 1}));
}

/*!
 * The whole point of recording a failure. Asked again about a file that has not changed since
 * it could not be read, the answer is no - otherwise every sweep pays for an open over the
 * network for every broken file in the library.
 */
TEST(TestSampledGeometry, AFailedAttemptCountsAsDoneUntilTheFileChanges)
{
  ContentGeometryAttempt attempt;
  attempt.exists = true;
  attempt.algorithmVersion = CONTENT_GEOMETRY_ALGORITHM_VERSION;
  attempt.identity = IDENTITY;
  attempt.outcome = ContentGeometryOutcome::Failed;

  EXPECT_FALSE(NeedsContentGeometry(attempt, IDENTITY));
  EXPECT_TRUE(NeedsContentGeometry(attempt, {IDENTITY.size + 1, IDENTITY.time}));
}

//! An identity nothing could establish matches nothing, including itself, so a caller that
//! passed one would remeasure the file on every single run.
TEST(TestSampledGeometry, AnUnknownIdentityAlwaysAsksForMeasurement)
{
  ContentGeometryAttempt attempt;
  attempt.exists = true;
  attempt.algorithmVersion = CONTENT_GEOMETRY_ALGORITHM_VERSION;
  attempt.identity = {};

  EXPECT_TRUE(NeedsContentGeometry(attempt, {}));
}

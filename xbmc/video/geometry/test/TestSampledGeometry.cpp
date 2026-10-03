/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "video/geometry/SampledGeometry.h"

#include <gtest/gtest.h>

using namespace KODI::VIDEO::GEOMETRY;

namespace
{

constexpr int CODED_WIDTH{3840};
constexpr int CODED_HEIGHT{2160};

//! A 2.35:1 picture in a 16:9 frame.
constexpr int CONTENT_TOP{264};
constexpr int CONTENT_BOTTOM{1896};

const FileIdentity IDENTITY{8'000'000'000, 1'700'000'000};

SampledGeometry MakeScan()
{
  SampledGeometry scan;
  scan.succeeded = true;
  scan.coded = CRectInt{0, 0, CODED_WIDTH, CODED_HEIGHT};
  scan.displayAspect = 1.7777778f;
  scan.samples = {{CRectInt{0, CONTENT_TOP, CODED_WIDTH, CONTENT_BOTTOM}, 0.9f, false},
                  {CRectInt{0, CONTENT_TOP, CODED_WIDTH, CONTENT_BOTTOM}, 0.8f, false}};

  const CRectInt scope{0, CONTENT_TOP, CODED_WIDTH, CONTENT_BOTTOM};
  scan.combined.rect = scope;
  scan.combined.shapes = {scope};
  scan.combined.hasReading = true;
  scan.combined.usable = 2;
  scan.combined.clusters = {{scope, 2, 1.7f}};

  return scan;
}

} // unnamed namespace

TEST(TestSampledGeometry, ASucceededScanBecomesItsDisplayRatio)
{
  const ContentGeometryRecord record{MakeContentGeometryRecord(MakeScan(), IDENTITY)};

  ASSERT_EQ(1u, record.aspects.size());
  EXPECT_FLOAT_EQ(2.35f, record.aspects[0]);
  EXPECT_TRUE(record.HasReading());
  EXPECT_FALSE(record.Varies());
  EXPECT_EQ(CONTENT_GEOMETRY_ALGORITHM_VERSION, record.algorithmVersion);
  EXPECT_EQ(IDENTITY.size, record.identity.size);
  EXPECT_EQ(IDENTITY.time, record.identity.time);
}

//! The ratio is the displayed one: an anamorphic frame's coded pixels are not square.
TEST(TestSampledGeometry, AnAnamorphicScanStoresTheRatioItIsSeenAt)
{
  SampledGeometry scan;
  scan.succeeded = true;
  scan.coded = CRectInt{0, 0, 720, 576};
  scan.displayAspect = 16.0f / 9.0f;
  scan.combined.hasReading = true;
  scan.combined.shapes = {CRectInt{0, 70, 720, 506}};

  const ContentGeometryRecord record{MakeContentGeometryRecord(scan, IDENTITY)};

  ASSERT_EQ(1u, record.aspects.size());
  EXPECT_FLOAT_EQ(2.35f, record.aspects[0]);
}

TEST(TestSampledGeometry, AVaryingTitleStoresEachRatioDominantFirst)
{
  SampledGeometry scan{MakeScan()};
  scan.combined.shapes.push_back(CRectInt{0, 0, CODED_WIDTH, CODED_HEIGHT});

  const ContentGeometryRecord record{MakeContentGeometryRecord(scan, IDENTITY)};

  ASSERT_EQ(2u, record.aspects.size());
  EXPECT_FLOAT_EQ(2.35f, record.aspects[0]);
  EXPECT_FLOAT_EQ(1.78f, record.aspects[1]);
  EXPECT_TRUE(record.Varies());
}

//! Two stretches at one ratio are one ratio, so a title is not said to vary for moving its
//! bars by a row.
TEST(TestSampledGeometry, TheSameRatioTwiceIsStoredOnce)
{
  SampledGeometry scan{MakeScan()};
  scan.combined.shapes.push_back(CRectInt{0, CONTENT_TOP + 1, CODED_WIDTH, CONTENT_BOTTOM + 1});

  const ContentGeometryRecord record{MakeContentGeometryRecord(scan, IDENTITY)};

  EXPECT_EQ(1u, record.aspects.size());
  EXPECT_FALSE(record.Varies());
}

/*!
 * A file that could not be read, or that read nothing, still becomes a row, so that a sweep
 * over a large library does not attempt it again every time. It carries no ratio.
 */
TEST(TestSampledGeometry, AScanThatReadNothingStoresNoRatio)
{
  const ContentGeometryRecord failed{MakeContentGeometryRecord(SampledGeometry{}, IDENTITY)};
  EXPECT_FALSE(failed.HasReading());
  EXPECT_EQ(IDENTITY.size, failed.identity.size);

  SampledGeometry empty{MakeScan()};
  empty.combined = {};
  empty.combined.rect = empty.coded;
  empty.combined.discarded = 9;

  EXPECT_FALSE(MakeContentGeometryRecord(empty, IDENTITY).HasReading());
}

TEST(TestSampledGeometry, NothingIsNeededForAnUpToDateMeasurement)
{
  ContentGeometryRecord stored;
  stored.aspects = {2.39f};
  stored.identity = IDENTITY;

  EXPECT_FALSE(NeedsContentGeometry(stored, IDENTITY));
}

TEST(TestSampledGeometry, AFileWithNoAttemptNeedsMeasuring)
{
  EXPECT_TRUE(NeedsContentGeometry(std::nullopt, IDENTITY));
}

TEST(TestSampledGeometry, ASupersededMeasurementNeedsMeasuringAgain)
{
  ContentGeometryRecord stored;
  stored.aspects = {2.39f};
  stored.algorithmVersion = CONTENT_GEOMETRY_ALGORITHM_VERSION - 1;
  stored.identity = IDENTITY;

  EXPECT_TRUE(NeedsContentGeometry(stored, IDENTITY));
}

TEST(TestSampledGeometry, AMeasurementOfDifferentContentNeedsMeasuringAgain)
{
  ContentGeometryRecord stored;
  stored.aspects = {2.39f};
  stored.identity = IDENTITY;

  EXPECT_TRUE(NeedsContentGeometry(stored, {IDENTITY.size + 1, IDENTITY.time}));
  EXPECT_TRUE(NeedsContentGeometry(stored, {IDENTITY.size, IDENTITY.time + 1}));
}

/*!
 * The whole point of recording an attempt that found nothing. Asked again about a file that has
 * not changed since, the answer is no - otherwise every sweep pays for an open over the network
 * for every unreadable file in the library.
 */
TEST(TestSampledGeometry, AnAttemptThatFoundNothingCountsAsDoneUntilTheFileChanges)
{
  ContentGeometryRecord stored;
  stored.identity = IDENTITY;

  EXPECT_FALSE(NeedsContentGeometry(stored, IDENTITY));
  EXPECT_TRUE(NeedsContentGeometry(stored, {IDENTITY.size + 1, IDENTITY.time}));
}

//! An identity nothing could establish matches nothing, including itself, so a caller that
//! passed one would remeasure the file on every single run.
TEST(TestSampledGeometry, AnUnknownIdentityAlwaysAsksForMeasurement)
{
  ContentGeometryRecord stored;
  stored.aspects = {2.39f};

  EXPECT_TRUE(NeedsContentGeometry(stored, FileIdentity{}));
}

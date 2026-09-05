/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "video/geometry/FrameSampling.h"

#include <algorithm>
#include <cmath>

#include <gtest/gtest.h>

using namespace KODI::VIDEO::GEOMETRY;

namespace
{
constexpr double RUNTIME = 6000.0; // a 100 minute film
}

TEST(TestFrameSampling, DefaultScheduleSpansTheWindow)
{
  const std::vector<double> offsets = SampleOffsets(RUNTIME);

  ASSERT_EQ(9u, offsets.size());
  EXPECT_DOUBLE_EQ(120.0, offsets.front());
  EXPECT_DOUBLE_EQ(RUNTIME - 60.0, offsets.back());
}

//! A film and an episode together, because a tenth of the film is most of the episode.
TEST(TestFrameSampling, TheExclusionsDoNotScaleWithRuntime)
{
  const auto [filmStart, filmEnd] = SampleWindow(RUNTIME);
  const auto [episodeStart, episodeEnd] = SampleWindow(1596.0);

  EXPECT_DOUBLE_EQ(filmStart, episodeStart);
  EXPECT_DOUBLE_EQ(RUNTIME - filmEnd, 1596.0 - episodeEnd);
}

/*!
 * An episode whose 4:3 body gives way to a scope closing act. A window that stops short of the
 * end lands its last sample before the change, so the whole closing section goes unrecorded and
 * the episode is stored as one shape - which is why the schedule reaches as far as it does.
 */
TEST(TestFrameSampling, ReachesAClosingSectionAShortWindowMissed)
{
  constexpr double EPISODE = 1596.0;
  constexpr double SCOPE_BEGINS = 1380.0;

  EXPECT_LT(SampleOffsets(EPISODE).back(), EPISODE);
  EXPECT_GT(SampleOffsets(EPISODE).back(), SCOPE_BEGINS);
}

TEST(TestFrameSampling, NeverSamplesTheStart)
{
  for (unsigned int points = 1; points <= 40; ++points)
  {
    SamplingParams params;
    params.points = points;
    for (const double offset : SampleOffsets(RUNTIME, params))
      EXPECT_GT(offset, 0.0) << "points = " << points;
  }
}

/*!
 * Credits are worth excluding twice over: frequently full-container on a scope title, and
 * text on black returns a rectangle describing the text rather than any picture.
 */
TEST(TestFrameSampling, StopsShortOfTheEnd)
{
  for (unsigned int points = 1; points <= 40; ++points)
  {
    SamplingParams params;
    params.points = points;
    for (const double offset : SampleOffsets(RUNTIME, params))
      EXPECT_LE(offset, RUNTIME - 60.0) << "points = " << points;
  }
}

//! A library of features with long credit rolls is what raising the exclusion is for.
TEST(TestFrameSampling, TheExclusionsAreHonoured)
{
  SamplingParams params;
  params.leadInSeconds = 300.0;
  params.leadOutSeconds = 600.0;

  const std::vector<double> offsets = SampleOffsets(RUNTIME, params);
  EXPECT_DOUBLE_EQ(300.0, offsets.front());
  EXPECT_DOUBLE_EQ(5400.0, offsets.back());
}

TEST(TestFrameSampling, OffsetsAreOrderedAndDistinct)
{
  const std::vector<double> offsets = SampleOffsets(RUNTIME);
  for (size_t i = 1; i < offsets.size(); ++i)
    EXPECT_GT(offsets[i], offsets[i - 1]);
}

TEST(TestFrameSampling, SinglePointLandsMidWindow)
{
  SamplingParams params;
  params.points = 1;

  const std::vector<double> offsets = SampleOffsets(RUNTIME, params);
  ASSERT_EQ(1u, offsets.size());
  EXPECT_DOUBLE_EQ((120.0 + RUNTIME - 60.0) / 2.0, offsets.front());
}

TEST(TestFrameSampling, UnusableInputsYieldNoSchedule)
{
  EXPECT_TRUE(SampleOffsets(0.0).empty());
  EXPECT_TRUE(SampleOffsets(-1.0).empty());

  SamplingParams none;
  none.points = 0;
  EXPECT_TRUE(SampleOffsets(RUNTIME, none).empty());
}

//! A clip shorter than the two exclusions combined would otherwise have no window at all.
TEST(TestFrameSampling, ShortItem)
{
  const std::vector<double> offsets = SampleOffsets(20.0);

  ASSERT_EQ(9u, offsets.size());
  EXPECT_DOUBLE_EQ(2.0, offsets.front());
  EXPECT_DOUBLE_EQ(18.0, offsets.back());
}

TEST(TestFrameSampling, ExclusionsThatCannotFitDoNotProduceGarbage)
{
  SamplingParams params;
  params.leadInSeconds = RUNTIME;
  params.leadOutSeconds = RUNTIME;

  for (const double offset : SampleOffsets(RUNTIME, params))
  {
    EXPECT_GT(offset, 0.0);
    EXPECT_LT(offset, RUNTIME);
  }
}

// --- escalation -----------------------------------------------------------------------

/*!
 * Measured on a hybrid IMAX title: nine points landed on no full-frame section at all and
 * reported a fixed ratio, where twenty-seven found both geometries. What marked the short
 * pass as untrustworthy was not the rectangle - every surviving sample agreed - but that
 * four of nine samples had been thrown away.
 */
TEST(TestFrameSampling, HighDiscardRateEscalates)
{
  EXPECT_TRUE(ShouldEscalate(1, 5, 4)); // the varying title: 44% discarded
  EXPECT_FALSE(ShouldEscalate(1, 8, 1)); // 11% discarded
  EXPECT_FALSE(ShouldEscalate(1, 9, 0)); // unanimous
}

TEST(TestFrameSampling, MoreThanOneGeometryEscalates)
{
  EXPECT_TRUE(ShouldEscalate(2, 9, 0)) << "densify before believing a title varies";
}

TEST(TestFrameSampling, NothingSurvivingEscalates)
{
  EXPECT_TRUE(ShouldEscalate(0, 0, 9));
}

/*!
 * The reading that a unanimous pass would otherwise erase without trace. Corroboration rules
 * are about repetition, which is the right test for a boundary a pixel or two out and the
 * wrong one for a rectangle of an entirely different ratio: that is either a section the
 * sampling nearly missed or a detector failure, and only a denser pass separates them.
 */
TEST(TestFrameSampling, AShapeTheAnswerCannotExplainEscalates)
{
  EXPECT_TRUE(ShouldEscalate(1, 8, 1, 1)) << "one is enough; a ratio is not a rounding error";
  EXPECT_FALSE(ShouldEscalate(1, 8, 1, 0)) << "a discarded reading of the same shape is noise";
}

TEST(TestFrameSampling, EscalationCanBeDisabled)
{
  SamplingParams params;
  params.escalatedPoints = 0;

  EXPECT_FALSE(ShouldEscalate(2, 5, 4, 0, params));

  params.escalatedPoints = params.points; // no denser than we already were
  EXPECT_FALSE(ShouldEscalate(2, 5, 4, 0, params));
}

TEST(TestFrameSampling, EscalationOnlyAddsPositionsNotAlreadyMeasured)
{
  const std::vector<double> first = SampleOffsets(RUNTIME);

  SamplingParams denser;
  denser.points = 27;
  const std::vector<double> all = SampleOffsets(RUNTIME, denser);
  const std::vector<double> extra = UnsampledOffsets(all, first);

  // Nothing already measured is measured again...
  for (const double offset : extra)
  {
    for (const double taken : first)
      EXPECT_GE(std::abs(offset - taken), 1.0);
  }

  // ...and nothing wanted is dropped: every denser position is either scheduled or was
  // already covered. Only the few positions the two schedules share drop out - three of
  // twenty-seven here, not nine, because the schedules do not nest.
  for (const double offset : all)
  {
    const bool scheduled = std::find(extra.begin(), extra.end(), offset) != extra.end();
    const bool covered = std::any_of(first.begin(), first.end(),
                                     [&](double taken) { return std::abs(taken - offset) < 1.0; });
    EXPECT_TRUE(scheduled || covered) << "position " << offset << " would never be sampled";
  }

  EXPECT_GT(extra.size(), first.size());
  EXPECT_LT(extra.size(), all.size());
}

TEST(TestFrameSampling, LiveExclusionIsPositional)
{
  EXPECT_TRUE(WithinLiveLeadExclusion(0.0, RUNTIME, 120.0, 60.0));
  EXPECT_TRUE(WithinLiveLeadExclusion(119.0, RUNTIME, 120.0, 60.0));
  EXPECT_FALSE(WithinLiveLeadExclusion(121.0, RUNTIME, 120.0, 60.0));
  EXPECT_FALSE(WithinLiveLeadExclusion(2700.0, RUNTIME, 120.0, 60.0));
}

TEST(TestFrameSampling, LiveExclusionCoversTheCredits)
{
  EXPECT_FALSE(WithinLiveLeadExclusion(RUNTIME - 61.0, RUNTIME, 120.0, 60.0));
  EXPECT_TRUE(WithinLiveLeadExclusion(RUNTIME - 59.0, RUNTIME, 120.0, 60.0));
  EXPECT_TRUE(WithinLiveLeadExclusion(RUNTIME, RUNTIME, 120.0, 60.0));
}

//! A source with no duration has no position in a title for a window to be relative to, so
//! no window applies - live TV and streams are explicitly out of the exclusion's scope.
TEST(TestFrameSampling, LiveExclusionDoesNotApplyWithoutADuration)
{
  EXPECT_FALSE(WithinLiveLeadExclusion(0.0, 0.0, 120.0, 60.0));
  EXPECT_FALSE(WithinLiveLeadExclusion(30.0, 0.0, 120.0, 60.0));
}

//! The same short-title fallback as the scan: when the exclusions do not fit, the central
//! window is sampled rather than nothing.
TEST(TestFrameSampling, LiveExclusionFallsBackOnAShortTitle)
{
  constexpr double SHORT = 150.0;
  EXPECT_TRUE(WithinLiveLeadExclusion(10.0, SHORT, 120.0, 60.0));
  EXPECT_FALSE(WithinLiveLeadExclusion(75.0, SHORT, 120.0, 60.0));
  EXPECT_TRUE(WithinLiveLeadExclusion(145.0, SHORT, 120.0, 60.0));
}

TEST(TestFrameSampling, LiveExclusionCanBeTurnedOff)
{
  EXPECT_FALSE(WithinLiveLeadExclusion(0.0, RUNTIME, 0.0, 0.0));
  EXPECT_FALSE(WithinLiveLeadExclusion(RUNTIME, RUNTIME, 0.0, 0.0));
  EXPECT_FALSE(WithinLiveLeadExclusion(0.0, 0.0, 0.0, 0.0));
}


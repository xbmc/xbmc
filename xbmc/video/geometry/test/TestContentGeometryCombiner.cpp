/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "video/geometry/ContentGeometryCombiner.h"
#include "video/geometry/test/GeometryTestHelpers.h"

#include <vector>

#include <gtest/gtest.h>

using namespace KODI::VIDEO::GEOMETRY;
using KODI::VIDEO::GEOMETRY::TEST::ExpectRect;

namespace
{

constexpr int WIDTH = 3840;
constexpr int HEIGHT = 2160;
const CRectInt CODED{0, 0, WIDTH, HEIGHT};

//! \brief A letterboxed reading with equal bars top and bottom.
GeometrySample Letterbox(int bar, float confidence)
{
  return {CRectInt{0, bar, WIDTH, HEIGHT - bar}, confidence, false};
}

GeometrySample FullFrame(float confidence)
{
  return {CODED, confidence, false};
}

GeometrySample Degenerate()
{
  return {CODED, 0.0f, true};
}

//! \brief A pillarboxed reading with equal bars left and right.
GeometrySample Pillarbox(int bar, float confidence)
{
  return {CRectInt{bar, 0, WIDTH - bar, HEIGHT}, confidence, false};
}

CombinedGeometry Combine(const std::vector<GeometrySample>& samples,
                         const CombinerParams& params = {})
{
  return CombineGeometrySamples(samples, CODED, params);
}

} // namespace

TEST(TestContentGeometryCombiner, UnanimousSamples)
{
  std::vector<GeometrySample> samples;
  for (int i = 0; i < 9; ++i)
    samples.push_back(Letterbox(263, 0.9f));

  const CombinedGeometry result = Combine(samples);

  ExpectRect(result.rect, 0, 263, WIDTH, HEIGHT - 263);
  EXPECT_TRUE(result.hasReading);
  EXPECT_FALSE(result.varies);
  EXPECT_EQ(1u, result.clusters.size());
  EXPECT_EQ(9u, result.usable);
}

/*!
 * The Menu, 2026-08-06: eight samples at 263/263 and one dark-scene reading of 3062x1015.
 * The outlier must vanish without trace. A "widest wins" rule would take the outlier's
 * width in some frames and collapse the film; a median over edges would survive this but
 * not the variable-aspect case below.
 */
TEST(TestContentGeometryCombiner, LoneOutlierIsAbsorbed)
{
  std::vector<GeometrySample> samples;
  for (int i = 0; i < 8; ++i)
    samples.push_back(Letterbox(263, 0.8f));
  samples.push_back({CRectInt{59, 869, 3121, 1884}, 0.4f, false});

  const CombinedGeometry result = Combine(samples);

  ExpectRect(result.rect, 0, 263, WIDTH, HEIGHT - 263);
  EXPECT_FALSE(result.varies);
  EXPECT_EQ(2u, result.clusters.size());
  EXPECT_EQ(8u, result.clusters.front().samples);
}

/*!
 * A cut to black is dark and flat on every line, so the detector reports maximum bars. That
 * is not a very narrow reading, it is no reading, and combining it as narrow is the most
 * likely way a mask ends up closing onto picture.
 */
TEST(TestContentGeometryCombiner, DegenerateSamplesAreDiscarded)
{
  std::vector<GeometrySample> samples;
  for (int i = 0; i < 5; ++i)
    samples.push_back(Letterbox(263, 0.9f));
  for (int i = 0; i < 4; ++i)
    samples.push_back(Degenerate());

  const CombinedGeometry result = Combine(samples);

  ExpectRect(result.rect, 0, 263, WIDTH, HEIGHT - 263);
  EXPECT_EQ(5u, result.usable);
  EXPECT_EQ(4u, result.discarded);
  EXPECT_FALSE(result.varies);
}

TEST(TestContentGeometryCombiner, LowConfidenceSamplesAreDiscarded)
{
  std::vector<GeometrySample> samples;
  for (int i = 0; i < 5; ++i)
    samples.push_back(Letterbox(263, 0.9f));
  for (int i = 0; i < 3; ++i)
    samples.push_back(Letterbox(700, 0.01f));

  const CombinedGeometry result = Combine(samples);

  ExpectRect(result.rect, 0, 263, WIDTH, HEIGHT - 263);
  EXPECT_EQ(5u, result.usable);
  EXPECT_EQ(3u, result.discarded);
}

/*!
 * Mandalorian & Grogu, IMAX HYBRID: alternates between full frame and 2.40 through the
 * film. Two stationary clusters is what varying geometry looks like.
 */
TEST(TestContentGeometryCombiner, TwoStationaryClustersMeanVaries)
{
  std::vector<GeometrySample> samples;
  for (int i = 0; i < 4; ++i)
    samples.push_back(FullFrame(1.0f));
  for (int i = 0; i < 2; ++i)
    samples.push_back(Letterbox(275, 0.9f));

  const CombinedGeometry result = Combine(samples);

  ExpectRect(result.rect, 0, 0, WIDTH, HEIGHT);
  EXPECT_TRUE(result.varies);
  EXPECT_EQ(2u, result.clusters.size());
}

/*!
 * The bias this rule exists to defeat, and the reason varies is counted rather than
 * weighted. A full-frame reading scores top marks by construction - no edges means no soft
 * boundary to dock it for - while the letterboxed readings of the same title score far
 * lower because they have boundaries. Weighted, the minority geometry disappears; counted,
 * it survives. Measured on a real variable-aspect title, where weighting hid it completely.
 */
TEST(TestContentGeometryCombiner, VariesIsCountedNotWeighted)
{
  std::vector<GeometrySample> samples;
  for (int i = 0; i < 4; ++i)
    samples.push_back(FullFrame(1.0f));
  for (int i = 0; i < 2; ++i)
    samples.push_back(Letterbox(275, 0.07f));

  const CombinedGeometry result = Combine(samples);

  EXPECT_TRUE(result.varies) << "confidence weighting has suppressed the minority geometry";
}

//! \brief Sixteen scope samples with two full-frame rivals: two of eighteen is a ninth,
//! exactly at the default varies share.
std::vector<GeometrySample> ANinthInRivals()
{
  std::vector<GeometrySample> samples;
  for (int i = 0; i < 16; ++i)
    samples.push_back(Letterbox(280, 0.9f));
  for (int i = 0; i < 2; ++i)
    samples.push_back(FullFrame(0.9f));
  return samples;
}

/*!
 * The threshold itself, which decides a published field and is a tenth by default.
 *
 * Measured on a title cut in three ratios: a thorough scan found a genuine scope sequence in
 * twenty of a hundred and seventy-nine usable samples - a ninth - while the fifth this rule
 * once demanded published the title as one that does not vary, contradicting its own stored
 * detail, which listed the second shape.
 */
TEST(TestContentGeometryCombiner, ANinthOfTheSamplesIsEnoughToVary)
{
  const CombinedGeometry result = Combine(ANinthInRivals());

  EXPECT_TRUE(result.varies) << "two of eighteen is a ninth, and the title plainly varies";
}

TEST(TestContentGeometryCombiner, BelowTheShareTheTitleDoesNotVary)
{
  std::vector<GeometrySample> samples;
  for (int i = 0; i < 22; ++i)
    samples.push_back(Letterbox(280, 0.9f));
  for (int i = 0; i < 2; ++i)
    samples.push_back(FullFrame(0.9f));

  const CombinedGeometry result = Combine(samples);

  EXPECT_FALSE(result.varies) << "two of twenty-four is below a tenth";
  EXPECT_EQ(2u, result.clusters.size()) << "and the reading is still retained";
}

TEST(TestContentGeometryCombiner, TheShareIsSettable)
{
  // advancedsettings.xml can move it, because how much of a film has to be in another shape
  // before that is worth reporting depends on what is reading the answer.
  CombinerParams strict;
  strict.variesShare = 0.20f;

  EXPECT_FALSE(Combine(ANinthInRivals(), strict).varies);
}

/*!
 * The Wolf of Wall Street: one sample of five landed in a genuine pillarboxed sequence.
 * One sample is a point, not a cluster, and stationarity is a claim about repetition - so
 * this is not enough to call the title variable, however real the reading turned out to be.
 */
TEST(TestContentGeometryCombiner, SingleRivalSampleIsNotACluster)
{
  std::vector<GeometrySample> samples;
  for (int i = 0; i < 4; ++i)
    samples.push_back(FullFrame(1.0f));
  samples.push_back({CRectInt{415, 0, WIDTH - 410, HEIGHT}, 0.26f, false});

  const CombinedGeometry result = Combine(samples);

  ExpectRect(result.rect, 0, 0, WIDTH, HEIGHT);
  EXPECT_FALSE(result.varies);
  EXPECT_EQ(2u, result.clusters.size()) << "the reading must still be retained";
}

/*!
 * Never narrower under uncertainty. With nothing to go on the answer is the coded frame,
 * which shows bars at worst; anything narrower risks masking real picture.
 */
TEST(TestContentGeometryCombiner, NoUsableSamplesReportsTheCodedFrame)
{
  const CombinedGeometry result = Combine({Degenerate(), Degenerate()});

  ExpectRect(result.rect, 0, 0, WIDTH, HEIGHT);
  EXPECT_FALSE(result.hasReading);
  EXPECT_FALSE(result.varies);
  EXPECT_EQ(0u, result.usable);
}

TEST(TestContentGeometryCombiner, NoSamplesAtAllReportsTheCodedFrame)
{
  const CombinedGeometry result = Combine({});

  ExpectRect(result.rect, 0, 0, WIDTH, HEIGHT);
  EXPECT_FALSE(result.hasReading);
}

//! Boundary ringing moves an edge by a line or two; that is the same edge.
TEST(TestContentGeometryCombiner, EdgesWithinToleranceAreOneCluster)
{
  const CombinedGeometry result =
      Combine({Letterbox(263, 0.9f), Letterbox(265, 0.9f), Letterbox(262, 0.9f),
               Letterbox(264, 0.9f), Letterbox(263, 0.9f)});

  EXPECT_EQ(1u, result.clusters.size());
  EXPECT_EQ(263, result.rect.y1) << "cluster centre should be the median of its members";
  EXPECT_FALSE(result.varies);
}

//! Beyond the tolerance they are different edges, and two of them means the title varies.
TEST(TestContentGeometryCombiner, EdgesBeyondToleranceAreSeparateClusters)
{
  const CombinedGeometry result =
      Combine({Letterbox(263, 0.9f), Letterbox(263, 0.9f), Letterbox(263, 0.9f),
               Letterbox(140, 0.9f), Letterbox(140, 0.9f)});

  EXPECT_EQ(2u, result.clusters.size());
  EXPECT_EQ(263, result.rect.y1);
  EXPECT_TRUE(result.varies);
}

/*!
 * The dominant cluster is chosen on weight, so a handful of confident samples beat a larger
 * group of doubtful ones - but only for the rectangle. varies is unaffected, by design.
 */
TEST(TestContentGeometryCombiner, DominantClusterIsChosenOnWeight)
{
  std::vector<GeometrySample> samples;
  for (int i = 0; i < 3; ++i)
    samples.push_back(Letterbox(263, 1.0f));
  for (int i = 0; i < 4; ++i)
    samples.push_back(Letterbox(140, 0.2f));

  const CombinedGeometry result = Combine(samples);

  EXPECT_EQ(263, result.rect.y1);
  EXPECT_EQ(3u, result.clusters.front().samples);
}

/*!
 * WandaVision S01E01, 4:3 pillarboxed black-and-white: the picture beside the bar never
 * clears the detector's margin, so every sample scores zero, and every sample agrees to
 * within 2px across the runtime. Discarding these loses the only reading there is and the
 * episode reports 16:9.
 */
TEST(TestContentGeometryCombiner, StationaryUnscoredSamplesStillRead)
{
  std::vector<GeometrySample> samples;
  for (int i = 0; i < 8; ++i)
    samples.push_back(Pillarbox(478 + i % 3, 0.0f));

  const CombinedGeometry result = Combine(samples);

  EXPECT_TRUE(result.hasReading);
  EXPECT_EQ(479, result.rect.x1);
  EXPECT_EQ(8u, result.usable);
  EXPECT_EQ(0u, result.discarded);
}

//! One unscored reading is a point, and two is a coincidence. Neither is stationarity.
TEST(TestContentGeometryCombiner, TooFewUnscoredSamplesDoNotRead)
{
  const CombinedGeometry result = Combine({Pillarbox(478, 0.0f), Pillarbox(479, 0.0f)});

  EXPECT_FALSE(result.hasReading);
  ExpectRect(result.rect, 0, 0, WIDTH, HEIGHT); // never narrower than the coded frame
  EXPECT_EQ(2u, result.discarded);
}

/*!
 * The rescue applies only where the title scored nothing at all. A confident reading
 * elsewhere means confidence is working here, so an unscored cluster is the suspect one.
 */
TEST(TestContentGeometryCombiner, UnscoredSamplesAreStillDroppedWhenTheTitleScored)
{
  std::vector<GeometrySample> samples;
  for (int i = 0; i < 4; ++i)
    samples.push_back(Letterbox(263, 0.9f));
  for (int i = 0; i < 4; ++i)
    samples.push_back(Pillarbox(478, 0.0f));

  const CombinedGeometry result = Combine(samples);

  ExpectRect(result.rect, 0, 263, WIDTH, HEIGHT - 263);
  EXPECT_EQ(4u, result.usable);
  EXPECT_EQ(4u, result.discarded);
  EXPECT_FALSE(result.varies);
}

TEST(TestContentGeometryCombiner, ClusterCentreIsTheMedianOfItsMembers)
{
  const CombinedGeometry result =
      Combine({Letterbox(260, 0.9f), Letterbox(263, 0.9f), Letterbox(266, 0.9f)});

  EXPECT_EQ(1u, result.clusters.size());
  EXPECT_EQ(263, result.clusters.front().rect.y1);
  EXPECT_EQ(HEIGHT - 263, result.clusters.front().rect.y2);
}

//! Pillarboxing, to prove clustering is on the whole rectangle and not on height alone.
TEST(TestContentGeometryCombiner, PillarboxAndLetterboxAreDistinguished)
{
  std::vector<GeometrySample> samples;
  for (int i = 0; i < 4; ++i)
    samples.push_back({CRectInt{480, 0, WIDTH - 480, HEIGHT}, 0.9f, false});
  for (int i = 0; i < 4; ++i)
    samples.push_back(Letterbox(480, 0.9f));

  const CombinedGeometry result = Combine(samples);

  EXPECT_EQ(2u, result.clusters.size()) << "same bar thickness, different rectangle";
  EXPECT_TRUE(result.varies);
}

/*!
 * WandaVision S01E01 in miniature: a 4:3 sitcom body whose closing act is scope, on content
 * dark enough beside the pillarbox that every sample scores nothing. Corroboration then falls
 * to repetition, and the lone scope reading has none - correctly, since one sample is not a
 * section. Erasing it silently is the problem: it is the only sign the sampling caught the
 * edge of something, and a title stored as fixed 4:3 is never revisited.
 */
TEST(TestContentGeometryCombiner, AnUnexplainedShapeSurvivesAsAReasonToLookAgain)
{
  std::vector<GeometrySample> samples;
  for (int i = 0; i < 8; ++i)
    samples.push_back(Pillarbox(480, 0.0f));
  samples.push_back(Letterbox(276, 0.0f));

  const CombinedGeometry result = Combine(samples);

  EXPECT_EQ(2880, result.rect.Width()) << "the answer is still the corroborated 4:3";
  EXPECT_FALSE(result.varies) << "one sample is not a section";
  EXPECT_EQ(1u, result.unexplainedShapes);
}

//! The rule must not fire on the noise it was carved out of: a reading a few pixels off is
//! the same shape, and clustering already has it.
TEST(TestContentGeometryCombiner, ANearbyDiscardedReadingIsNotAnUnexplainedShape)
{
  std::vector<GeometrySample> samples;
  for (int i = 0; i < 8; ++i)
    samples.push_back(Letterbox(276, 0.0f));
  samples.push_back(Letterbox(292, 0.0f));

  const CombinedGeometry result = Combine(samples);

  EXPECT_EQ(0u, result.unexplainedShapes);
}

//! The shapes a title is in: the dominant one, then each rival seen often enough to count.
TEST(TestContentGeometryCombiner, TheShapesAreTheAnswerThenEachRivalThatCounts)
{
  std::vector<GeometrySample> samples;
  for (int i = 0; i < 6; ++i)
    samples.push_back(Letterbox(276, 0.9f));
  for (int i = 0; i < 3; ++i)
    samples.push_back(Letterbox(140, 0.9f));

  const CombinedGeometry result = Combine(samples);

  ASSERT_EQ(2u, result.clusters.size());
  EXPECT_EQ(CRectInt(0, 276, WIDTH, HEIGHT - 276), result.rect) << "the dominant shape answers";
  ASSERT_EQ(2u, result.shapes.size());
  EXPECT_EQ(result.rect, result.shapes[0]);
  EXPECT_EQ(CRectInt(0, 140, WIDTH, HEIGHT - 140), result.shapes[1]);
  EXPECT_TRUE(result.varies);
}

TEST(TestContentGeometryCombiner, AFixedTitleIsInOneShape)
{
  std::vector<GeometrySample> samples;
  for (int i = 0; i < 6; ++i)
    samples.push_back(Letterbox(276, 0.9f));

  const CombinedGeometry result = Combine(samples);

  ASSERT_EQ(1u, result.shapes.size());
  EXPECT_EQ(result.rect, result.shapes[0]);
  EXPECT_FALSE(result.varies);
}

/*!
 * The share rule is >=, so a rival at exactly variesShare varies. Tested either side of 0.10
 * already; this is the value itself, which is the one a reader has to guess at.
 */
TEST(TestContentGeometryCombiner, ARivalAtExactlyTheVariesShareVaries)
{
  // The count rule is neutralised, or a rival of exactly two samples sits on both boundaries
  // at once and the test passes whichever of the two is >=.
  CombinerParams params;
  params.minRivalSamples = 1;

  std::vector<GeometrySample> samples;
  for (int i = 0; i < 18; ++i)
    samples.push_back(Letterbox(276, 0.9f));
  for (int i = 0; i < 2; ++i)
    samples.push_back(Letterbox(140, 0.9f));

  const CombinedGeometry result = Combine(samples, params);

  ASSERT_EQ(2u, result.clusters.size());
  EXPECT_EQ(2u, result.clusters[1].samples);
  EXPECT_TRUE(result.varies) << "2 of 20 is exactly the 0.10 share, and the rule is >=";
}

//! And the count rule beside it, also >=: minRivalSamples of 2 means two samples qualify.
TEST(TestContentGeometryCombiner, ARivalAtExactlyTheMinimumSampleCountVaries)
{
  CombinerParams params;
  params.variesShare = 0.0f; // isolate the count rule

  std::vector<GeometrySample> samples;
  for (int i = 0; i < 10; ++i)
    samples.push_back(Letterbox(276, 0.9f));
  for (int i = 0; i < 2; ++i)
    samples.push_back(Letterbox(140, 0.9f));

  EXPECT_TRUE(Combine(samples, params).varies);

  std::vector<GeometrySample> one;
  for (int i = 0; i < 10; ++i)
    one.push_back(Letterbox(276, 0.9f));
  one.push_back(Letterbox(140, 0.9f));

  EXPECT_FALSE(Combine(one, params).varies) << "one below the minimum is not a section";
}

/*!
 * Two readings exactly tolerance apart are the same shape. The same 8 is the live selector's
 * jitter floor and the merge's match window, so three places have to agree what "the same
 * edge" means - and each of them tests either side of it rather than on it.
 */
TEST(TestContentGeometryCombiner, ReadingsExactlyToleranceApartAreOneShape)
{
  const CombinerParams params;

  std::vector<GeometrySample> samples;
  for (int i = 0; i < 4; ++i)
    samples.push_back(Letterbox(276, 0.9f));
  for (int i = 0; i < 4; ++i)
    samples.push_back(Letterbox(276 - static_cast<int>(params.tolerance), 0.9f));

  const CombinedGeometry result = Combine(samples, params);

  EXPECT_EQ(1u, result.clusters.size()) << "exactly the tolerance is still the same edge";
  EXPECT_FALSE(result.varies);
}

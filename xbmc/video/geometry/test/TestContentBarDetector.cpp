/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SyntheticFrame.h"
#include "video/geometry/ContentBarDetector.h"
#include "video/geometry/ContentBarHistogram.h"
#include "video/geometry/test/GeometryTestHelpers.h"

#include <algorithm>
#include <array>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::VIDEO::GEOMETRY;
using KODI::VIDEO::GEOMETRY::TEST::ExpectRect;
using namespace KODI::VIDEO::GEOMETRY::TEST;

namespace
{

/*!
 * \brief Build a plainly letterboxed frame: bars of \p bar lines top and bottom.
 */
CSyntheticFrame Letterboxed(unsigned int width,
                            unsigned int height,
                            unsigned int bar,
                            unsigned int bitDepth = 8,
                            ColorRange range = ColorRange::Limited,
                            ChromaSubsampling subsampling = ChromaSubsampling::YUV420)
{
  CSyntheticFrame frame(width, height, bitDepth, range, subsampling);
  frame.FillPicture(bar, height - bar, frame.Scaled(60), frame.Scaled(220), 1);
  frame.FillRows(0, bar, frame.Black(), frame.Neutral(), frame.Neutral());
  frame.FillRows(height - bar, height, frame.Black(), frame.Neutral(), frame.Neutral());
  return frame;
}

} // namespace

// --- baseline -------------------------------------------------------------------------

TEST(TestContentBarDetector, CleanBarsEightBitLimited)
{
  const CSyntheticFrame frame = Letterboxed(640, 360, 60);
  const DetectionResult result = DetectContentRect(frame.Ref());

  ExpectRect(result.rect, 0, 60, 640, 300);
  EXPECT_FALSE(result.degenerate);
  EXPECT_GT(result.confidence, 0.9f);
  EXPECT_TRUE(result.top.measured);
  EXPECT_TRUE(result.bottom.measured);
  EXPECT_FALSE(result.left.measured);
  EXPECT_FALSE(result.right.measured);
  EXPECT_EQ(60u, result.top.thickness);
  EXPECT_EQ(60u, result.bottom.thickness);
  EXPECT_EQ(28u, result.thresholdUsed);
}

/*!
 * The headline failure this detector exists to avoid: an 8-bit constant meeting a 10-bit
 * plane. Studio black is 64 here, so any threshold derived without the bit depth reports
 * the full frame - confidently, and with no error.
 */
TEST(TestContentBarDetector, CleanBarsTenBitLimited)
{
  const CSyntheticFrame frame = Letterboxed(640, 360, 60, 10);
  const DetectionResult result = DetectContentRect(frame.Ref());

  ExpectRect(result.rect, 0, 60, 640, 300);
  EXPECT_FALSE(result.degenerate);
  EXPECT_GT(result.confidence, 0.9f);
  EXPECT_EQ(112u, result.thresholdUsed); // (16 + 12) << 2
  EXPECT_EQ(64u, result.blackFloorObserved);
}

TEST(TestContentBarDetector, CleanBarsTwelveBitLimited)
{
  const CSyntheticFrame frame = Letterboxed(640, 360, 60, 12);
  const DetectionResult result = DetectContentRect(frame.Ref());

  ExpectRect(result.rect, 0, 60, 640, 300);
  EXPECT_EQ(448u, result.thresholdUsed); // (16 + 12) << 4
}

// --- robust flatness ------------------------------------------------------------------

namespace
{

unsigned int WorstRowSpread(const CSyntheticFrame& frame,
                            unsigned int width,
                            unsigned int y0,
                            unsigned int y1)
{
  unsigned int worst = 0;
  for (unsigned int y = y0; y < y1; ++y)
  {
    unsigned int lowest = ~0u;
    unsigned int highest = 0;
    for (unsigned int x = 0; x < width; ++x)
    {
      const unsigned int value = frame.Luma(x, y);
      lowest = std::min(lowest, value);
      highest = std::max(highest, value);
    }
    worst = std::max(worst, highest - lowest);
  }
  return worst;
}

} // namespace

/*!
 * The decisive case for a robust statistic. HandBrake's test is "every pixel within +/-16
 * of the row average", which a handful of ringing or stuck pixels in a 3840-wide row
 * defeats outright. A high percentile tolerates half a percent of the line, so ten
 * outliers cost nothing, and the row is still read as the bar it plainly is.
 */
TEST(TestContentBarDetector, SparseOutliersDoNotDefeatABar)
{
  CSyntheticFrame frame = Letterboxed(3840, 256, 64);
  for (unsigned int y = 0; y < 256; ++y)
  {
    if (y >= 64 && y < 192)
      continue;
    for (unsigned int k = 0; k < 10; ++k)
      frame.SetLuma(37 + k * 331, y, 200);
  }

  EXPECT_GT(WorstRowSpread(frame, 3840, 0, 64), 32u)
      << "corpus does not actually defeat a max-deviation test";

  const DetectionResult result = DetectContentRect(frame.Ref());
  ExpectRect(result.rect, 0, 64, 3840, 192);
  EXPECT_FALSE(result.degenerate);
}

/*!
 * Dither and lightly retained grain in the bar, at a level that is actually seen in
 * delivered encodes. Note that a maximum-deviation test survives this too: the robust
 * statistic's advantage is outlier tolerance, not noise tolerance, and claiming otherwise
 * would overstate it.
 */
TEST(TestContentBarDetector, DitheredBarsAreStillBars)
{
  CSyntheticFrame frame = Letterboxed(640, 256, 64);
  frame.AddLumaGrain(3, 42, 0, 64);
  frame.AddLumaGrain(3, 43, 192, 256);

  const DetectionResult result = DetectContentRect(frame.Ref());
  ExpectRect(result.rect, 0, 64, 640, 192);
}

/*!
 * The limit, pinned deliberately. No flatness band that is any use can accept a bar
 * carrying sigma 8 of noise, and no threshold derivation should be widened until it can.
 * The detector declines to call it a bar and reports the full frame - too wide, which is
 * the harmless direction.
 */
TEST(TestContentBarDetector, HeavilyGrainedBarsAreReportedWideNotNarrow)
{
  CSyntheticFrame frame = Letterboxed(640, 256, 64);
  frame.AddLumaGrain(8, 42, 0, 64);
  frame.AddLumaGrain(8, 43, 192, 256);

  const DetectionResult result = DetectContentRect(frame.Ref());
  ExpectRect(result.rect, 0, 0, 640, 256);
}

// --- threshold derivation -------------------------------------------------------------

TEST(TestContentBarDetector, FullRangeBlackAtZero)
{
  const CSyntheticFrame frame = Letterboxed(640, 360, 60, 8, ColorRange::Full);
  const DetectionResult result = DetectContentRect(frame.Ref());

  ExpectRect(result.rect, 0, 60, 640, 300);
  EXPECT_EQ(12u, result.thresholdUsed);
  EXPECT_EQ(0u, result.blackFloorObserved);
}

/*!
 * Content whose black really is 0 but whose flag says limited range. The limited threshold
 * is the more generous of the two, so the reading is still correct - which is why an
 * unknown range resolves to limited rather than to full.
 */
TEST(TestContentBarDetector, FullRangeContentFlaggedAsLimited)
{
  CSyntheticFrame frame(640, 360, 8, ColorRange::Full);
  frame.FillPicture(60, 300, 60, 220, 1);
  frame.FillRows(0, 60, 0, frame.Neutral(), frame.Neutral());
  frame.FillRows(300, 360, 0, frame.Neutral(), frame.Neutral());

  FrameRef reference = frame.Ref();
  reference.range = ColorRange::Limited;

  const DetectionResult result = DetectContentRect(reference);
  ExpectRect(result.rect, 0, 60, 640, 300);
}

TEST(TestContentBarDetector, UnspecifiedRangeAssumesLimitedAndCostsConfidence)
{
  const CSyntheticFrame frame = Letterboxed(640, 360, 60);

  FrameRef unspecified = frame.Ref();
  unspecified.range = ColorRange::Unspecified;

  const DetectionResult known = DetectContentRect(frame.Ref());
  const DetectionResult assumed = DetectContentRect(unspecified);

  ExpectRect(assumed.rect, 0, 60, 640, 300);
  EXPECT_TRUE(assumed.rangeAssumed);
  EXPECT_FALSE(known.rangeAssumed);
  EXPECT_LT(assumed.confidence, known.confidence);
}

// --- chroma neutrality ----------------------------------------------------------------

/*!
 * Dark saturated picture - a night sky, sodium haze - is dark and flat in luma, so on PQ
 * the luma tests barely discriminate. A true bar is achromatic; this is not.
 */
TEST(TestContentBarDetector, DarkSaturatedPictureIsNotABar)
{
  CSyntheticFrame frame(640, 360, 10);
  const unsigned int saturated = frame.Neutral() + frame.Scaled(40);
  frame.FillRows(0, 360, frame.Scaled(18), saturated, frame.Neutral());
  frame.FillRows(0, 40, frame.Black(), frame.Neutral(), frame.Neutral());

  const DetectionResult result = DetectContentRect(frame.Ref());

  ExpectRect(result.rect, 0, 40, 640, 360);
  EXPECT_TRUE(result.chromaTested);

  // The control: identical luma, achromatic. Nothing but the chroma test separates the
  // two, and without it the whole frame reads as bar.
  CSyntheticFrame control(640, 360, 10);
  control.FillRows(0, 360, control.Scaled(18), control.Neutral(), control.Neutral());
  control.FillRows(0, 40, control.Black(), control.Neutral(), control.Neutral());

  EXPECT_TRUE(DetectContentRect(control.Ref()).degenerate);
}

/*!
 * The acknowledged hard case, pinned so a later change cannot quietly claim to have
 * solved it. Flat-shaded animation can be genuinely dark, genuinely flat and genuinely
 * achromatic, so it is indistinguishable from a bar in a single frame. The detector
 * over-crops by the depth of the flat region and reports it with high confidence; only
 * evidence from other frames can resolve it, which is the sampler's problem.
 */
TEST(TestContentBarDetector, FlatShadedDarkPictureIsIndistinguishableFromABar)
{
  CSyntheticFrame frame(640, 360, 10);
  frame.FillPicture(80, 280, frame.Scaled(60), frame.Scaled(220), 5);
  frame.FillRows(40, 80, frame.Scaled(18), frame.Neutral(), frame.Neutral());
  frame.FillRows(280, 320, frame.Scaled(18), frame.Neutral(), frame.Neutral());
  frame.FillRows(0, 40, frame.Black(), frame.Neutral(), frame.Neutral());
  frame.FillRows(320, 360, frame.Black(), frame.Neutral(), frame.Neutral());

  const DetectionResult result = DetectContentRect(frame.Ref());

  ExpectRect(result.rect, 0, 80, 640, 280); // truth is 40 / 320
  EXPECT_FALSE(result.degenerate);
}

// --- overlays -------------------------------------------------------------------------

TEST(TestContentBarDetector, SubtitleInBarDoesNotMoveTheRect)
{
  CSyntheticFrame clean = Letterboxed(640, 360, 60);
  CSyntheticFrame subtitled = Letterboxed(640, 360, 60);
  subtitled.FillRect(192, 316, 448, 340, subtitled.Scaled(200), subtitled.Neutral(),
                     subtitled.Neutral());

  const DetectionResult expected = DetectContentRect(clean.Ref());
  const DetectionResult result = DetectContentRect(subtitled.Ref());

  ExpectRect(result.rect, expected.rect.x1, expected.rect.y1, expected.rect.x2, expected.rect.y2);
  EXPECT_GT(result.overlayLines, 0u);
  EXPECT_FALSE(result.overlaySuspected);
}

TEST(TestContentBarDetector, LogoInBarDoesNotMoveTheRect)
{
  CSyntheticFrame frame = Letterboxed(640, 360, 60);
  frame.FillRect(540, 12, 620, 44, frame.Scaled(210), frame.Neutral(), frame.Neutral());

  const DetectionResult result = DetectContentRect(frame.Ref());

  ExpectRect(result.rect, 0, 60, 640, 300);
  EXPECT_GT(result.overlayLines, 0u);
}

/*!
 * An overlay wider than half the line is not admitted, so the walk stops and the bar is
 * kept. That is the wider, safe direction, and it is exactly what would have happened
 * without an overlay rule at all - the rule can only improve on the baseline, never make
 * it worse. The flag lets the sampler down-weight the frame.
 */
TEST(TestContentBarDetector, OverWideOverlayFailsWide)
{
  CSyntheticFrame frame = Letterboxed(640, 360, 60);
  frame.FillRect(64, 316, 512, 340, frame.Scaled(200), frame.Neutral(), frame.Neutral());

  const DetectionResult result = DetectContentRect(frame.Ref());

  EXPECT_GT(result.rect.y2, 300);
  EXPECT_TRUE(result.overlaySuspected);
}

// --- degenerate and near-degenerate ---------------------------------------------------

/*!
 * A cut to black finds no picture at all. This must never be treated as a very narrow
 * reading; the rect stays at the full frame so that even a caller which ignores the flag
 * fails wide rather than closing a mask onto picture.
 */
TEST(TestContentBarDetector, CutToBlackIsDegenerate)
{
  const CSyntheticFrame frame(640, 360, 10);
  const DetectionResult result = DetectContentRect(frame.Ref());

  EXPECT_TRUE(result.degenerate);
  ExpectRect(result.rect, 0, 0, 640, 360);
  EXPECT_FLOAT_EQ(0.0f, result.confidence);
}

/*!
 * A fade does not trip the degenerate flag - a few lines stay marginally above black - so
 * the boolean alone is not enough to discard it. The boundary is soft, and confidence has
 * to collapse instead.
 */
TEST(TestContentBarDetector, FadeToBlackCollapsesConfidence)
{
  CSyntheticFrame frame(640, 360, 10);
  frame.FillPicture(60, 300, frame.Scaled(29), frame.Scaled(31), 9);
  frame.FillRows(0, 60, frame.Black(), frame.Neutral(), frame.Neutral());
  frame.FillRows(300, 360, frame.Black(), frame.Neutral(), frame.Neutral());

  const DetectionResult result = DetectContentRect(frame.Ref());

  ExpectRect(result.rect, 0, 60, 640, 300);
  EXPECT_FALSE(result.degenerate);
  EXPECT_LT(result.confidence, 0.5f);
}

// --- negative controls ----------------------------------------------------------------

TEST(TestContentBarDetector, ContentAtItsTrueAspectGetsNoInventedBars)
{
  CSyntheticFrame frame(640, 360, 10);
  frame.FillPicture(0, 360, frame.Scaled(60), frame.Scaled(220), 11);

  const DetectionResult result = DetectContentRect(frame.Ref());

  ExpectRect(result.rect, 0, 0, 640, 360);
  EXPECT_FALSE(result.degenerate);
  EXPECT_FALSE(result.top.measured);
  EXPECT_FALSE(result.bottom.measured);
  EXPECT_FALSE(result.left.measured);
  EXPECT_FALSE(result.right.measured);
  EXPECT_GT(result.confidence, 0.9f);
}

/*!
 * A dark first row is not a bar unless it is also flat and achromatic. Real picture that
 * happens to be dark still has range in it, and that is the whole discriminator.
 */
TEST(TestContentBarDetector, DarkButStructuredFirstRowIsNotABar)
{
  CSyntheticFrame frame(640, 360, 10);
  frame.FillPicture(0, 360, frame.Scaled(60), frame.Scaled(220), 13);
  frame.FillPicture(0, 1, frame.Scaled(17), frame.Scaled(45), 17);

  const DetectionResult result = DetectContentRect(frame.Ref());
  ExpectRect(result.rect, 0, 0, 640, 360);
}

// --- columns --------------------------------------------------------------------------

TEST(TestContentBarDetector, PillarboxOnly)
{
  CSyntheticFrame frame(640, 360, 10);
  frame.FillPicture(0, 360, frame.Scaled(60), frame.Scaled(220), 19);
  frame.FillRect(0, 0, 80, 360, frame.Black(), frame.Neutral(), frame.Neutral());
  frame.FillRect(560, 0, 640, 360, frame.Black(), frame.Neutral(), frame.Neutral());

  const DetectionResult result = DetectContentRect(frame.Ref());

  ExpectRect(result.rect, 80, 0, 560, 360);
  EXPECT_TRUE(result.left.measured);
  EXPECT_TRUE(result.right.measured);
  EXPECT_FALSE(result.top.measured);
}

/*!
 * The correction that the design document does not carry: on a letterboxed frame every
 * column contains the bars, so a column walk run independently of the row walk is dark
 * and flat all the way across and consumes the whole frame. Columns must be judged only
 * over the rows that survived the row walk.
 */
TEST(TestContentBarDetector, WindowboxNeedsColumnsJudgedOverPictureRowsOnly)
{
  CSyntheticFrame frame(640, 360, 10);
  frame.FillPicture(0, 360, frame.Scaled(60), frame.Scaled(220), 23);
  frame.FillRows(0, 40, frame.Black(), frame.Neutral(), frame.Neutral());
  frame.FillRows(320, 360, frame.Black(), frame.Neutral(), frame.Neutral());
  frame.FillRect(0, 0, 80, 360, frame.Black(), frame.Neutral(), frame.Neutral());
  frame.FillRect(560, 0, 640, 360, frame.Black(), frame.Neutral(), frame.Neutral());

  const DetectionResult result = DetectContentRect(frame.Ref());

  ExpectRect(result.rect, 80, 40, 560, 320);
  EXPECT_FALSE(result.degenerate);
}

// --- symmetry -------------------------------------------------------------------------

/*!
 * Off-centre bars are real, and reporting a rect rather than a ratio exists precisely so
 * they can be expressed. Asymmetry therefore costs confidence and must never alter the
 * rectangle.
 */
TEST(TestContentBarDetector, OffCentreBarsAreReportedNotCorrected)
{
  CSyntheticFrame frame(640, 360, 10);
  frame.FillPicture(40, 280, frame.Scaled(60), frame.Scaled(220), 29);
  frame.FillRows(0, 40, frame.Black(), frame.Neutral(), frame.Neutral());
  frame.FillRows(280, 360, frame.Black(), frame.Neutral(), frame.Neutral());

  const DetectionResult result = DetectContentRect(frame.Ref());

  ExpectRect(result.rect, 0, 40, 640, 280);
  EXPECT_LT(result.symmetry, 1.0f);
  EXPECT_FALSE(result.degenerate);
}

/*!
 * A sliver is not a bar. From real content (2026-08-06): a two-pixel edge alongside a
 * 263-line letterbox scored zero on its own boundary and took the whole frame's confidence
 * with it, on a reading otherwise correct to within a row; and on already-cropped content
 * every spurious finding across 135 samples was exactly one line. Real letterboxing is
 * tens to hundreds of lines. Sub-floor findings are dropped and flagged.
 */
TEST(TestContentBarDetector, SubFloorEdgeIsNotABar)
{
  CSyntheticFrame frame(640, 360, 10);
  frame.FillPicture(60, 300, frame.Scaled(29), frame.Scaled(220), 43);
  frame.FillRows(0, 60, frame.Black(), frame.Neutral(), frame.Neutral());
  frame.FillRows(300, 360, frame.Black(), frame.Neutral(), frame.Neutral());
  frame.FillRect(0, 60, 3, 300, frame.Black(), frame.Neutral(), frame.Neutral());

  const DetectionResult result = DetectContentRect(frame.Ref());

  ExpectRect(result.rect, 0, 60, 640, 300);
  EXPECT_TRUE(result.subFloorEdgesIgnored);
  EXPECT_FALSE(result.left.measured);
  EXPECT_GT(result.confidence, 0.7f);
}

/*!
 * The case this is really for: content already coded at its true aspect, where a single
 * boundary line reads as black. It must come back exactly full-frame, not one line short.
 */
TEST(TestContentBarDetector, SingleBlackLineOnCorrectlyCodedContentIsIgnored)
{
  CSyntheticFrame frame(640, 360, 10);
  frame.FillPicture(0, 360, frame.Scaled(60), frame.Scaled(220), 47);
  frame.FillRows(0, 1, frame.Black(), frame.Neutral(), frame.Neutral());
  frame.FillRows(359, 360, frame.Black(), frame.Neutral(), frame.Neutral());

  const DetectionResult result = DetectContentRect(frame.Ref());

  ExpectRect(result.rect, 0, 0, 640, 360);
  EXPECT_TRUE(result.subFloorEdgesIgnored);
}

/*!
 * Also from The Menu: one marginal line at the transition, which is where ringing lives,
 * zeroed separation across a 263-line bar. Taking the worst line is the maximum-deviation
 * mistake one level up; the bar as a whole is what the score is about.
 */
TEST(TestContentBarDetector, OneMarginalBarLineDoesNotZeroSeparation)
{
  CSyntheticFrame frame = Letterboxed(640, 360, 60, 10);
  for (unsigned int x = 0; x < 640; ++x)
  {
    frame.SetLuma(x, 59, frame.Scaled(28)); // sitting exactly on the threshold
    frame.SetLuma(x, 300, frame.Scaled(28));
  }

  const DetectionResult result = DetectContentRect(frame.Ref());

  ExpectRect(result.rect, 0, 60, 640, 300);
  EXPECT_GT(result.separation, 0.8f);
  EXPECT_GT(result.confidence, 0.8f);
}

// --- subsampling ----------------------------------------------------------------------

TEST(TestContentBarDetector, Subsampling422)
{
  const CSyntheticFrame frame =
      Letterboxed(640, 360, 60, 10, ColorRange::Limited, ChromaSubsampling::YUV422);
  ExpectRect(DetectContentRect(frame.Ref()).rect, 0, 60, 640, 300);
}

TEST(TestContentBarDetector, Subsampling444)
{
  const CSyntheticFrame frame =
      Letterboxed(640, 360, 60, 10, ColorRange::Limited, ChromaSubsampling::YUV444);
  ExpectRect(DetectContentRect(frame.Ref()).rect, 0, 60, 640, 300);
}

/*!
 * On 4:2:0 the chroma line straddling the boundary carries picture chroma, so the last
 * bar line fails the neutrality test and the walk stops one line early. One row of bar is
 * kept - the wider, safe direction - and that is asserted rather than left to be
 * discovered later.
 */
TEST(TestContentBarDetector, OddBoundaryOn420LosesOneLineWide)
{
  CSyntheticFrame frame(640, 360, 10);
  frame.FillRows(0, 41, frame.Black(), frame.Neutral(), frame.Neutral());
  frame.FillRows(319, 360, frame.Black(), frame.Neutral(), frame.Neutral());
  frame.FillPicture(41, 319, frame.Scaled(60), frame.Scaled(220), 31);
  // Colour the picture, so the chroma line the boundary shares really is contaminated.
  frame.FillChromaRows(41, 319, frame.Neutral() + frame.Scaled(40), frame.Neutral());

  const DetectionResult result = DetectContentRect(frame.Ref());

  EXPECT_EQ(40, result.rect.y1); // truth is 41
  EXPECT_EQ(320, result.rect.y2); // truth is 319
}

TEST(TestContentBarDetector, MissingChromaPlanesSkipTheNeutralityTest)
{
  CSyntheticFrame frame = Letterboxed(640, 360, 60, 10);
  frame.DropChroma();

  const DetectionResult result = DetectContentRect(frame.Ref());

  ExpectRect(result.rect, 0, 60, 640, 300);
  EXPECT_FALSE(result.chromaTested);
  EXPECT_LT(result.confidence, 1.0f);
}

// --- region of interest ---------------------------------------------------------------

/*!
 * How a side-by-side frame is handled without the detector knowing what stereoscopy is:
 * the caller, which can see stereo_mode, hands over one view. The rectangle comes back in
 * full-frame coordinates.
 */
TEST(TestContentBarDetector, RoiSelectsOneViewOfSideBySide)
{
  CSyntheticFrame frame(1280, 360, 10);
  frame.FillPicture(0, 360, frame.Scaled(60), frame.Scaled(220), 37);
  frame.FillRect(0, 0, 640, 60, frame.Black(), frame.Neutral(), frame.Neutral());
  frame.FillRect(0, 300, 640, 360, frame.Black(), frame.Neutral(), frame.Neutral());

  FrameRef reference = frame.Ref();
  reference.roi = CRectInt(0, 0, 640, 360);

  const DetectionResult result = DetectContentRect(reference);
  ExpectRect(result.rect, 0, 60, 640, 300);

  reference.roi = CRectInt(640, 0, 1280, 360);
  const DetectionResult other = DetectContentRect(reference);
  ExpectRect(other.rect, 640, 0, 1280, 360);
}

// --- robustness -----------------------------------------------------------------------

TEST(TestContentBarDetector, OddDimensions)
{
  const CSyntheticFrame frame = Letterboxed(1919, 1079, 137, 10);
  const DetectionResult result = DetectContentRect(frame.Ref());

  EXPECT_EQ(137, result.rect.y1);
  EXPECT_EQ(1079 - 137, result.rect.y2);
  EXPECT_EQ(0, result.rect.x1);
  EXPECT_EQ(1919, result.rect.x2);
}

TEST(TestContentBarDetector, TinyFrameDoesNotMisbehave)
{
  CSyntheticFrame frame(16, 16, 8);
  frame.FillPicture(0, 16, 60, 220, 41);

  const DetectionResult result = DetectContentRect(frame.Ref());
  ExpectRect(result.rect, 0, 0, 16, 16);
}

TEST(TestContentBarDetector, EmptyFrameIsRejected)
{
  FrameRef reference;
  const DetectionResult result = DetectContentRect(reference);

  EXPECT_TRUE(result.degenerate);
  EXPECT_FLOAT_EQ(0.0f, result.confidence);
}

// --- the histogram a line is classified from --------------------------------------------

namespace
{

//! \brief The four the detector asks for, which is what the bulk answer exists to serve.
constexpr std::array<double, 4> LINE_QUANTILES{0.005, 0.25, 0.75, 0.995};

CHistogram Filled(const std::vector<std::pair<unsigned int, unsigned int>>& bucketsAndCounts)
{
  CHistogram histogram;
  for (const auto& [bucket, count] : bucketsAndCounts)
  {
    for (unsigned int i = 0; i < count; ++i)
      histogram.Add(bucket);
  }
  return histogram;
}

void ExpectBothWaysAgree(const CHistogram& histogram)
{
  const std::array<unsigned int, 4> bulk{histogram.Percentiles(LINE_QUANTILES)};
  for (size_t i = 0; i < LINE_QUANTILES.size(); ++i)
  {
    EXPECT_EQ(histogram.Percentile(LINE_QUANTILES[i]), bulk[i])
        << "quantile " << LINE_QUANTILES[i] << " read differently in the single walk";
  }
}

} // namespace

/*!
 * The bulk answer exists only to save three walks of the buckets, so the one thing it owes is
 * agreeing with the single one everywhere. Its loop advances through the quantiles while the
 * single one restarts for each, and the two decide "past this target" in separate places -
 * which is a divergence that would surface as a content edge in the wrong row and nothing else.
 */
TEST(TestContentBarHistogram, BothWaysOfAskingAgree)
{
  // Flat, so every target falls inside one bucket's run.
  ExpectBothWaysAgree(Filled({{16, 100}}));

  // A bar and a picture, which is the shape a real line has.
  ExpectBothWaysAgree(Filled({{16, 900}, {180, 100}}));

  // Two hundred samples put the first target on 1 and the last on 199, so a run ending exactly
  // on one is reachable - which is where "accumulated > target" and ">=" would part company.
  ExpectBothWaysAgree(Filled({{16, 1}, {17, 49}, {90, 100}, {200, 49}, {255, 1}}));
  ExpectBothWaysAgree(Filled({{0, 100}, {1, 99}, {255, 1}}));

  // One sample per bucket, so consecutive quantiles land in different buckets.
  std::vector<std::pair<unsigned int, unsigned int>> spread;
  for (unsigned int bucket = 0; bucket < BUCKET_COUNT; ++bucket)
    spread.emplace_back(bucket, 1);
  ExpectBothWaysAgree(Filled(spread));

  // Everything in the last bucket, where the walk runs off the end and both fall back to it.
  ExpectBothWaysAgree(Filled({{BUCKET_COUNT - 1, 64}}));
}

//! An empty histogram has no quantile to give, and says so the same way whichever is asked.
TEST(TestContentBarHistogram, AnEmptyHistogramAnswersZero)
{
  const CHistogram histogram;

  EXPECT_EQ(0u, histogram.Total());
  EXPECT_EQ(0u, histogram.Percentile(0.5));
  EXPECT_EQ((std::array<unsigned int, 4>{0, 0, 0, 0}), histogram.Percentiles(LINE_QUANTILES));
}

//! Reset returns it to that state rather than to a stale one - it is reused line to line.
TEST(TestContentBarHistogram, ResetEmptiesIt)
{
  CHistogram histogram{Filled({{16, 10}, {200, 10}})};
  ASSERT_EQ(20u, histogram.Total());

  histogram.Reset();
  EXPECT_EQ(0u, histogram.Total());
  EXPECT_EQ(0u, histogram.Percentile(0.995));
}

/*!
 * Reset clears only the buckets that were written, so what it leaves behind has to be checked
 * by refilling rather than by asking an empty histogram - which answers zero without reading a
 * bucket at all, and so passes whatever the bins hold.
 *
 * A bar line is read, then a picture line, then a bar line again: counts surviving the first
 * reset would put the second bar line's high percentile up among the picture values and stop
 * the walk on a row that is a bar.
 */
TEST(TestContentBarHistogram, ResetLeavesNoCountsBehind)
{
  CHistogram reused{Filled({{16, 100}})};
  const std::array<unsigned int, 4> bar{reused.Percentiles(LINE_QUANTILES)};

  reused.Reset();
  for (unsigned int i = 0; i < 100; ++i)
    reused.Add(180);
  ASSERT_EQ(100u, reused.Total());
  EXPECT_EQ((std::array<unsigned int, 4>{180, 180, 180, 180}), reused.Percentiles(LINE_QUANTILES));

  reused.Reset();
  for (unsigned int i = 0; i < 100; ++i)
    reused.Add(16);
  EXPECT_EQ(bar, reused.Percentiles(LINE_QUANTILES));
  EXPECT_EQ(16u, reused.Percentile(0.995));

  // The written span is tracked across every Add, not just the first and last: a bucket in the
  // middle of the span left dirty reads as extra weight below the high percentile.
  reused.Reset();
  reused.Add(0);
  reused.Add(128);
  reused.Add(255);
  reused.Reset();
  reused.Add(64);
  EXPECT_EQ(1u, reused.Total());
  EXPECT_EQ(64u, reused.Percentile(0.005));
  EXPECT_EQ(64u, reused.Percentile(0.995));
}

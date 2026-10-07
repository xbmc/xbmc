/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "cores/VideoPlayer/VideoFileGeometry.h"
#include "test/TestUtils.h"

#include <cstdlib>
#include <string>

#include <gtest/gtest.h>

using namespace KODI::VIDEO::GEOMETRY;

/*
 * End to end cover for the sampler: open a file, seek, decode, detect, combine.
 *
 * testdata/letterbox_320x240_bars40.mp4 is generated, not sampled from anything - three
 * seconds of SMPTE bars at 320x160, padded to 320x240 with 40 lines of black top and
 * bottom, H.264 in yuv420p at limited range. 4.5 kB. The expected answer is therefore known
 * by construction rather than by measurement, and `ffmpeg -vf cropdetect` independently
 * reports crop=320:160:0:40 on it.
 *
 * To regenerate:
 *
 *   ffmpeg -f lavfi -i "smptebars=s=320x160:r=10:d=3" -vf "pad=320:240:0:40:black" \
 *          -c:v libx264 -preset veryslow -crf 24 -pix_fmt yuv420p -color_range tv \
 *          -x264-params "range=tv:keyint=10" letterbox_320x240_bars40.mp4
 *
 * letterbox_320x240_bars40_12bit.mkv is the same picture as HEVC in yuv420p12, a format the
 * decoder converts before handing it over:
 *
 *   ffmpeg -f lavfi -i "smptebars=s=320x160:r=10:d=3" \
 *          -vf "pad=320:240:0:40:black,format=yuv420p12le" -c:v libx265 -preset veryslow \
 *          -crf 24 -color_range tv -x265-params "range=limited:keyint=10" \
 *          letterbox_320x240_bars40_12bit.mkv
 *
 * sbs_640x240_bars40.mkv is that picture twice, side by side, declared left_right by the
 * Matroska container only, with nothing in the H.264 stream itself:
 *
 *   ffmpeg -f lavfi -i "smptebars=s=320x160:r=10:d=3" \
 *          -filter_complex "[0]pad=320:240:0:40:black,split[a][b];[a][b]hstack" \
 *          -c:v libx264 -preset veryslow -crf 24 -pix_fmt yuv420p -color_range tv \
 *          -x264-params "range=tv:keyint=10" -metadata:s:v:0 stereo_mode=left_right \
 *          sbs_640x240_bars40.mkv
 */

namespace
{

constexpr int CODED_WIDTH = 320;
constexpr int CODED_HEIGHT = 240;
constexpr int BAR = 40;

CFileItem Clip(const std::string& name)
{
  return CFileItem(XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/test/testdata/" + name), false);
}

CFileItem LetterboxedClip()
{
  return Clip("letterbox_320x240_bars40.mp4");
}

} // namespace

TEST(TestContentGeometrySampler, FindsTheBarsInALetterboxedFile)
{
  const SampledGeometry scan = CVideoFileGeometry::ExtractContentGeometry(LetterboxedClip());

  ASSERT_TRUE(scan.succeeded);
  EXPECT_EQ(CODED_WIDTH, scan.coded.Width());
  EXPECT_EQ(CODED_HEIGHT, scan.coded.Height());

  EXPECT_EQ(0, scan.combined.rect.x1);
  EXPECT_EQ(BAR, scan.combined.rect.y1);
  EXPECT_EQ(CODED_WIDTH, scan.combined.rect.x2);
  EXPECT_EQ(CODED_HEIGHT - BAR, scan.combined.rect.y2);

  EXPECT_TRUE(scan.combined.hasReading);
  EXPECT_FALSE(scan.combined.varies) << "the clip has one geometry throughout";
}

//! The frame is read in the format the decoder converted it to, not the one it decoded.
TEST(TestContentGeometrySampler, FindsTheBarsInAFileTheDecoderConverts)
{
  const SampledGeometry scan =
      CVideoFileGeometry::ExtractContentGeometry(Clip("letterbox_320x240_bars40_12bit.mkv"));

  ASSERT_TRUE(scan.succeeded);
  EXPECT_EQ(0, scan.combined.rect.x1);
  EXPECT_EQ(BAR, scan.combined.rect.y1);
  EXPECT_EQ(CODED_WIDTH, scan.combined.rect.x2);
  EXPECT_EQ(CODED_HEIGHT - BAR, scan.combined.rect.y2);
}

//! A stereo mode only the container declares still confines the measurement to one view.
TEST(TestContentGeometrySampler, MeasuresOneViewOfAContainerDeclaredStereoFile)
{
  const SampledGeometry scan =
      CVideoFileGeometry::ExtractContentGeometry(Clip("sbs_640x240_bars40.mkv"));

  ASSERT_TRUE(scan.succeeded);
  EXPECT_EQ(CODED_WIDTH, scan.coded.Width());
  EXPECT_EQ(CODED_HEIGHT, scan.coded.Height());

  EXPECT_EQ(0, scan.combined.rect.x1);
  EXPECT_EQ(BAR, scan.combined.rect.y1);
  EXPECT_EQ(CODED_WIDTH, scan.combined.rect.x2);
  EXPECT_EQ(CODED_HEIGHT - BAR, scan.combined.rect.y2);
}

//! Per-sample readings are retained, because a wrong cached answer can only be explained
//! afterwards if the readings behind it survived.
TEST(TestContentGeometrySampler, RetainsThePerSampleReadings)
{
  const SampledGeometry scan = CVideoFileGeometry::ExtractContentGeometry(LetterboxedClip());

  ASSERT_TRUE(scan.succeeded);
  ASSERT_FALSE(scan.samples.empty());

  for (const auto& sample : scan.samples)
  {
    EXPECT_FALSE(sample.degenerate);
    EXPECT_EQ(BAR, sample.rect.y1);
    EXPECT_EQ(CODED_HEIGHT - BAR, sample.rect.y2);
  }
}

//! Sampling is a measurement, so it has to give the same answer twice. If it ever does not,
//! something in the decode path is order dependent and cached geometry cannot survive a
//! rescan.
TEST(TestContentGeometrySampler, IsRepeatable)
{
  const SampledGeometry first = CVideoFileGeometry::ExtractContentGeometry(LetterboxedClip());
  const SampledGeometry second = CVideoFileGeometry::ExtractContentGeometry(LetterboxedClip());

  ASSERT_TRUE(first.succeeded);
  ASSERT_TRUE(second.succeeded);

  EXPECT_EQ(first.combined.rect.Width(), second.combined.rect.Width());
  EXPECT_EQ(first.combined.rect.Height(), second.combined.rect.Height());
  EXPECT_EQ(first.combined.varies, second.combined.varies);
  EXPECT_EQ(first.samples.size(), second.samples.size());
}

TEST(TestContentGeometrySampler, HonoursTheRequestedPointCount)
{
  KODI::VIDEO::GEOMETRY::SamplingParams sampling;
  sampling.points = 3;
  sampling.escalatedPoints = 0; // no densification, so the count is exactly what was asked

  const SampledGeometry scan =
      CVideoFileGeometry::ExtractContentGeometry(LetterboxedClip(), sampling);

  ASSERT_TRUE(scan.succeeded);
  EXPECT_EQ(3u, scan.samples.size());
}

//! The gate: an internet stream is not something we may open to measure.
TEST(TestContentGeometrySampler, RefusesItemsThatCannotBeMeasured)
{
  const CFileItem stream("http://example.invalid/stream.m3u8", false);
  ASSERT_FALSE(CVideoFileGeometry::CanMeasure(stream));

  const SampledGeometry scan = CVideoFileGeometry::ExtractContentGeometry(stream);

  EXPECT_FALSE(scan.succeeded);
  EXPECT_TRUE(scan.samples.empty());
  EXPECT_FALSE(scan.combined.hasReading);
  EXPECT_FALSE(scan.combined.varies);
}

//! A Blu-ray title opens without a player, so it is measured like any other file - only the
//! thumbnail and stream detail extraction policy turns discs away.
TEST(TestContentGeometrySampler, ABlurayTitleIsMeasurable)
{
  const CFileItem title("bluray://smb%3a%2f%2fnas%2fShare%2fFilm%2f/BDMV/PLAYLIST/00000.mpls",
                        false);
  const CFileItem disc("smb://nas/Share/Film/BDMV/index.bdmv", false);

  EXPECT_TRUE(CVideoFileGeometry::CanMeasure(title));
  EXPECT_TRUE(CVideoFileGeometry::CanMeasure(disc));
}

//! Needs a real Blu-ray, which CI does not have. To run it, set KODI_TEST_BLURAY to a title's
//! bluray:// playlist path and pass --gtest_also_run_disabled_tests.
TEST(TestContentGeometrySampler, DISABLED_MeasuresABlurayTitle)
{
  const char* path = std::getenv("KODI_TEST_BLURAY");
  if (!path || !*path)
    GTEST_SKIP() << "KODI_TEST_BLURAY is not set";

  const SampledGeometry scan = CVideoFileGeometry::ExtractContentGeometry(CFileItem(path, false));

  ASSERT_TRUE(scan.succeeded);
  EXPECT_TRUE(scan.combined.hasReading);
  EXPECT_FALSE(scan.samples.empty());
}

//! Never narrower under uncertainty: a file that cannot be opened reports nothing at all
//! rather than a narrow rectangle.
TEST(TestContentGeometrySampler, MissingFileYieldsNoReading)
{
  const CFileItem missing(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/test/testdata/nonexistent.mp4"), false);

  const SampledGeometry scan = CVideoFileGeometry::ExtractContentGeometry(missing);

  EXPECT_FALSE(scan.succeeded);
  EXPECT_FALSE(scan.combined.hasReading);
}

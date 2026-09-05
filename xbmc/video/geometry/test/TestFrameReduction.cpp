/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "video/geometry/FrameReduction.h"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::VIDEO::GEOMETRY;

namespace
{

//! \brief An owning NV12-layout frame: 8-bit luma plane, interleaved 8-bit chroma plane.
struct NV12Frame
{
  NV12Frame(unsigned int width, unsigned int height, uint8_t luma, uint8_t cb, uint8_t cr)
    : width(width),
      height(height),
      y(static_cast<size_t>(width) * height, luma)
  {
    uv.reserve(static_cast<size_t>(width) * height / 2);
    for (size_t i = 0; i < static_cast<size_t>(width) * height / 4; ++i)
    {
      uv.push_back(cb);
      uv.push_back(cr);
    }
  }

  ReductionSource Source() const
  {
    ReductionSource source;
    source.width = width;
    source.height = height;
    source.bitDepth = 8;
    source.chroma = ChromaLayout::Interleaved;
    source.y = y.data();
    source.yStrideBytes = static_cast<int>(width);
    source.u = uv.data();
    source.uStrideBytes = static_cast<int>(width);
    return source;
  }

  void FillLumaRows(unsigned int from, unsigned int to, uint8_t value)
  {
    for (unsigned int row = from; row < to; ++row)
      std::fill_n(y.begin() + static_cast<size_t>(row) * width, width, value);
  }

  unsigned int width;
  unsigned int height;
  std::vector<uint8_t> y;
  std::vector<uint8_t> uv;
};

} // unnamed namespace

TEST(TestFrameReduction, UniformFrameStaysUniform)
{
  const NV12Frame frame{256, 144, 100, 90, 160};

  ReducedFrame out;
  ASSERT_TRUE(ReduceFrame(frame.Source(), 64, out));

  EXPECT_EQ(64u, out.width);
  EXPECT_EQ(36u, out.height);
  for (const uint8_t sample : out.y)
    EXPECT_EQ(100, sample);
  for (const uint8_t sample : out.u)
    EXPECT_EQ(90, sample);
  for (const uint8_t sample : out.v)
    EXPECT_EQ(160, sample);
}

/*!
 * The property the whole reduced path rests on: a letterbox boundary survives the
 * downscale at its scaled position. Bars at coded rows 0..280 of 2160 reduce by 4 to
 * rows 0..70 of 540; the rows either side of the boundary must be cleanly bar and
 * cleanly picture, because that is what the detector's dark-and-flat walk reads.
 */
TEST(TestFrameReduction, LetterboxBoundarySurvivesAtItsScaledPosition)
{
  NV12Frame frame{3840, 2160, 128, 128, 128};
  frame.FillLumaRows(0, 280, 16);
  frame.FillLumaRows(1880, 2160, 16);

  ReducedFrame out;
  ASSERT_TRUE(ReduceFrame(frame.Source(), 960, out));

  ASSERT_EQ(960u, out.width);
  ASSERT_EQ(540u, out.height);
  EXPECT_EQ(16, out.y[static_cast<size_t>(69) * out.width]); // last bar row
  EXPECT_EQ(128, out.y[static_cast<size_t>(70) * out.width]); // first picture row
  EXPECT_EQ(128, out.y[static_cast<size_t>(469) * out.width]); // last picture row
  EXPECT_EQ(16, out.y[static_cast<size_t>(470) * out.width]); // first bar row below
}

TEST(TestFrameReduction, TopAlignedTenBitReadsAsItsHighByte)
{
  // P010-style: 10 significant bits at the top of a 16-bit word. Luma 300 in 10-bit terms
  // is 75 in 8-bit terms, and the word carries it as 300 << 6.
  constexpr unsigned int WIDTH = 64;
  constexpr unsigned int HEIGHT = 36;
  std::vector<uint16_t> luma(static_cast<size_t>(WIDTH) * HEIGHT, 300u << 6);
  std::vector<uint16_t> chroma(static_cast<size_t>(WIDTH) * HEIGHT / 2, 512u << 6);

  ReductionSource source;
  source.width = WIDTH;
  source.height = HEIGHT;
  source.bitDepth = 10;
  source.highAligned = true;
  source.chroma = ChromaLayout::Interleaved;
  source.y = reinterpret_cast<const uint8_t*>(luma.data());
  source.yStrideBytes = static_cast<int>(WIDTH * sizeof(uint16_t));
  source.u = reinterpret_cast<const uint8_t*>(chroma.data());
  source.uStrideBytes = static_cast<int>(WIDTH * sizeof(uint16_t));

  ReducedFrame out;
  ASSERT_TRUE(ReduceFrame(source, 32, out));

  for (const uint8_t sample : out.y)
    EXPECT_EQ(75, sample);
  for (const uint8_t sample : out.u)
    EXPECT_EQ(128, sample);
  for (const uint8_t sample : out.v)
    EXPECT_EQ(128, sample);
}

TEST(TestFrameReduction, ASourceSmallerThanTheTargetIsCopiedAtItsOwnSize)
{
  const NV12Frame frame{640, 360, 64, 128, 128};

  ReducedFrame out;
  ASSERT_TRUE(ReduceFrame(frame.Source(), 960, out));

  EXPECT_EQ(640u, out.width);
  EXPECT_EQ(360u, out.height);
  EXPECT_EQ(64, out.y[0]);
}

//! A GPU reducer sizes its blit target with ReductionOutputSize and hands the result to
//! ReduceFrame, so the two must agree on the extent for any source shape.
TEST(TestFrameReduction, OutputSizeIsTheExtentReduceFrameProduces)
{
  const std::pair<unsigned int, unsigned int> shapes[] = {
      {1920u, 1080u}, {1917u, 1077u}, {640u, 360u}, {320u, 240u}};
  for (const auto& [sourceWidth, sourceHeight] : shapes)
  {
    const NV12Frame frame{sourceWidth, sourceHeight, 100, 128, 128};

    ReducedFrame out;
    ASSERT_TRUE(ReduceFrame(frame.Source(), 384, out));

    const ReducedSize size = ReductionOutputSize(sourceWidth, sourceHeight, 384);
    EXPECT_EQ(size.width, out.width);
    EXPECT_EQ(size.height, out.height);
  }

  const ReducedSize none = ReductionOutputSize(0, 1080, 384);
  EXPECT_EQ(0u, none.width);
  EXPECT_EQ(0u, none.height);
}

TEST(TestFrameReduction, AnUndescribableSourceIsRefused)
{
  const NV12Frame frame{256, 144, 100, 128, 128};

  ReducedFrame out;

  ReductionSource noPlanes = frame.Source();
  noPlanes.y = nullptr;
  EXPECT_FALSE(ReduceFrame(noPlanes, 64, out));

  ReductionSource badDepth = frame.Source();
  badDepth.bitDepth = 17;
  EXPECT_FALSE(ReduceFrame(badDepth, 64, out));

  ReductionSource planarWithoutV = frame.Source();
  planarWithoutV.chroma = ChromaLayout::Planar;
  EXPECT_FALSE(ReduceFrame(planarWithoutV, 64, out));

  EXPECT_FALSE(ReduceFrame(frame.Source(), 0, out));
}

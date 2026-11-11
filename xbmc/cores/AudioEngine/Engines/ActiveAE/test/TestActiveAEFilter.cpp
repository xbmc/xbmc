/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/AudioEngine/Engines/ActiveAE/ActiveAEFilter.h"

#include <cmath>
#include <cstdint>
#include <numbers>
#include <vector>

#include <gtest/gtest.h>

extern "C"
{
#include <libavutil/channel_layout.h>
}

using namespace ActiveAE;

namespace
{
constexpr int SAMPLE_RATE = 48000;
constexpr int CHUNK = 1024;
constexpr int MAX_OUTPUT = 16384;

struct OutputFrame
{
  int samples;
  float tempo;
};

void InitStereoFloat(CActiveAEFilter& filter)
{
  filter.Init(AV_SAMPLE_FMT_FLT, SAMPLE_RATE, AV_CH_LAYOUT_STEREO);
}

/*!
 * \brief Feed a 440 Hz tone in chunks and take every frame the filter puts out
 *
 * \return The samples fed
 */
int Feed(CActiveAEFilter& filter, int samples, float tempo, std::vector<OutputFrame>& frames)
{
  std::vector<float> src(2 * CHUNK);
  std::vector<float> dst(2 * MAX_OUTPUT);
  uint8_t* srcPlanes[] = {reinterpret_cast<uint8_t*>(src.data())};
  uint8_t* dstPlanes[] = {reinterpret_cast<uint8_t*>(dst.data())};

  int fed = 0;
  while (fed < samples)
  {
    for (int i = 0; i < CHUNK; ++i)
    {
      const auto value =
          static_cast<float>(std::sin(2.0 * std::numbers::pi * 440.0 * (fed + i) / SAMPLE_RATE));
      src[2 * i] = value;
      src[2 * i + 1] = value;
    }

    int out = filter.ProcessFilter(dstPlanes, MAX_OUTPUT, srcPlanes, CHUNK,
                                   CHUNK * 2 * static_cast<int>(sizeof(float)));
    fed += CHUNK;
    while (out > 0)
    {
      frames.push_back({out, tempo});
      if (filter.NeedData() || filter.IsEof())
        break;
      out = filter.ProcessFilter(dstPlanes, MAX_OUTPUT, nullptr, 0, 0);
    }
  }
  return fed;
}

class CTestFilter : public CActiveAEFilter
{
public:
  void SetSampleCounts(int64_t in, double consumed)
  {
    m_SamplesIn = in;
    m_samplesConsumed = consumed;
    m_hasData = false;
  }
};
} // namespace

TEST(TestActiveAEFilter, ChangeTempoNeedsARunningFilter)
{
  CActiveAEFilter filter;
  InitStereoFloat(filter);
  EXPECT_FALSE(filter.ChangeTempo(1.5f));

  ASSERT_TRUE(filter.SetTempo(1.5f));
  EXPECT_FALSE(filter.ChangeTempo(1.0f));
}

TEST(TestActiveAEFilter, ChangeTempoKeepsWhatTheFilterBuffered)
{
  CActiveAEFilter filter;
  InitStereoFloat(filter);
  ASSERT_TRUE(filter.SetTempo(1.5f));

  std::vector<OutputFrame> frames;
  Feed(filter, SAMPLE_RATE / 2, 1.5f, frames);
  const int buffered = filter.GetBufferedSamples();
  ASSERT_GT(buffered, 0);

  EXPECT_TRUE(filter.ChangeTempo(1.6f));
  EXPECT_TRUE(filter.IsActive());
  EXPECT_EQ(filter.GetBufferedSamples(), buffered);
}

TEST(TestActiveAEFilter, SetTempoStartsAnEmptyFilter)
{
  // A flush relies on this to drop what the filter buffered
  CActiveAEFilter filter;
  InitStereoFloat(filter);
  ASSERT_TRUE(filter.SetTempo(1.5f));

  std::vector<OutputFrame> frames;
  Feed(filter, SAMPLE_RATE / 2, 1.5f, frames);
  ASSERT_GT(filter.GetBufferedSamples(), 0);

  ASSERT_TRUE(filter.SetTempo(1.5f));
  EXPECT_EQ(filter.GetBufferedSamples(), 0);
}

TEST(TestActiveAEFilter, BufferedSamplesCountOutputAtTheTempoItCameOutAt)
{
  CActiveAEFilter filter;
  InitStereoFloat(filter);
  ASSERT_TRUE(filter.SetTempo(1.5f));

  std::vector<OutputFrame> frames;
  int fed = Feed(filter, SAMPLE_RATE, 1.5f, frames);
  ASSERT_TRUE(filter.ChangeTempo(2.0f));
  fed += Feed(filter, SAMPLE_RATE, 2.0f, frames);
  ASSERT_GE(frames.size(), 2u);

  // An output frame counts as consumed input once the next one has come out
  double consumed = 0.0;
  for (size_t i = 0; i + 1 < frames.size(); ++i)
    consumed += frames[i].samples * static_cast<double>(frames[i].tempo);

  EXPECT_NEAR(filter.GetBufferedSamples(), fed - consumed, 1.0);
}

TEST(TestActiveAEFilter, BufferedSamplesBeyondIntMax)
{
  // Over 12 hours at 48 kHz without re-creating the filter
  CTestFilter filter;
  InitStereoFloat(filter);
  filter.SetSampleCounts(3'000'000'000, 2'999'999'000.0);

  EXPECT_EQ(filter.GetBufferedSamples(), 1000);
}

TEST(TestActiveAEFilter, ChangeTempoKeepsTheNumberOfStages)
{
  CActiveAEFilter filter;
  InitStereoFloat(filter);
  ASSERT_TRUE(filter.SetTempo(1.5f));

  std::vector<OutputFrame> frames;
  Feed(filter, SAMPLE_RATE / 2, 1.5f, frames);

  // Above 2 the filter is made of two stages, so it has to be re-created
  EXPECT_FALSE(filter.ChangeTempo(2.1f));
  EXPECT_TRUE(filter.ChangeTempo(1.9f));

  ASSERT_TRUE(filter.SetTempo(2.1f));
  Feed(filter, SAMPLE_RATE / 2, 2.1f, frames);
  EXPECT_TRUE(filter.ChangeTempo(3.5f));
  EXPECT_FALSE(filter.ChangeTempo(1.9f));
}

TEST(TestActiveAEFilter, ChainedStagesPlayAtTheTempo)
{
  // Catching up at twice a 2.085x clock needs three stages
  const float tempo = 4.17f;
  CActiveAEFilter filter;
  InitStereoFloat(filter);
  ASSERT_TRUE(filter.SetTempo(tempo));

  std::vector<OutputFrame> frames;
  const int fed = Feed(filter, 4 * SAMPLE_RATE, tempo, frames);

  // Drain what the stages hold
  std::vector<float> dst(2 * MAX_OUTPUT);
  uint8_t* dstPlanes[] = {reinterpret_cast<uint8_t*>(dst.data())};
  for (int i = 0; i < 1000 && !filter.IsEof(); ++i)
  {
    const int out = filter.ProcessFilter(dstPlanes, MAX_OUTPUT, nullptr, 0, 0);
    if (out > 0)
      frames.push_back({out, tempo});
  }

  int produced = 0;
  for (const auto& frame : frames)
    produced += frame.samples;
  const double expected = fed / static_cast<double>(tempo);
  EXPECT_NEAR(produced, expected, expected * 0.01);
}

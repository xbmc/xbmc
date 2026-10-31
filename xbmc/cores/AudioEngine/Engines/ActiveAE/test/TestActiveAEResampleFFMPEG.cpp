/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/AudioEngine/Engines/ActiveAE/ActiveAEResampleFFMPEG.h"

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

extern "C"
{
#include <libavutil/channel_layout.h>
}

using namespace ActiveAE;

namespace
{
SampleConfig StereoFloat(int sampleRate)
{
  return {AV_SAMPLE_FMT_FLT, AV_CH_LAYOUT_STEREO, 2, sampleRate, 32, 0};
}

/*!
 * \brief Resample 384 kHz audio to 48 kHz in the chunk sizes given, as SameBoy
 * hands it over
 *
 * \return The samples produced, or -1 if a call failed
 */
int Resample(const std::vector<int>& chunks, double ratio)
{
  CActiveAEResampleFFMPEG resampler;
  if (!resampler.Init(StereoFloat(48000), StereoFloat(384000), false, false, 1.0, nullptr,
                      AE_QUALITY_MID, true, 1.0f))
    return -1;

  std::vector<float> src(2 * 6400, 0.0f);
  std::vector<float> dst(2 * 2048);
  uint8_t* srcPlanes[] = {reinterpret_cast<uint8_t*>(src.data())};
  uint8_t* dstPlanes[] = {reinterpret_cast<uint8_t*>(dst.data())};

  int produced = 0;
  for (const int samples : chunks)
  {
    const int out = resampler.Resample(dstPlanes, 2048, srcPlanes, samples, ratio);
    if (out < 0)
      return -1;
    produced += out;
  }
  return produced;
}
} // namespace

TEST(TestActiveAEResampleFFMPEG, CompensatesAcrossShortCalls)
{
  // Ten seconds of 60 Hz frames, each handed over as a few samples and then
  // the rest of the frame
  std::vector<int> chunks;
  for (int frame = 0; frame < 600; ++frame)
  {
    const int head = 1 + frame % 7;
    chunks.push_back(head);
    chunks.push_back(6400 - head);
  }

  for (const double ratio : {0.98, 1.02})
  {
    const int produced = Resample(chunks, ratio);
    ASSERT_GE(produced, 0) << "a call failed at ratio " << ratio;

    // Within what the resampler's filter holds back
    EXPECT_NEAR(produced, 480000 * ratio, 64) << "ratio " << ratio;
  }
}

TEST(TestActiveAEResampleFFMPEG, CompensatesCallsTooShortForOutput)
{
  // Every call a few samples, each too few for a single output sample
  std::vector<int> chunks;
  int input = 0;
  for (int call = 0; call < 100000; ++call)
  {
    chunks.push_back(1 + call % 7);
    input += chunks.back();
  }

  for (const double ratio : {0.98, 1.02})
  {
    const int produced = Resample(chunks, ratio);
    ASSERT_GE(produced, 0) << "a call failed at ratio " << ratio;

    // Within what the resampler's filter holds back
    EXPECT_NEAR(produced, input / 8.0 * ratio, 64) << "ratio " << ratio;
  }
}

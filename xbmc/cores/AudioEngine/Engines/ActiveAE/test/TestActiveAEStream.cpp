/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/AudioEngine/Engines/ActiveAE/ActiveAEStream.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

#include <gtest/gtest.h>

using namespace ActiveAE;

namespace
{
// Gives the tests the stream's static helpers, a stream itself needs a running engine
struct StreamHelpers : public CActiveAEStream
{
  using CActiveAEStream::ForwardSpeed;
  using CActiveAEStream::RatioForSpeed;
};

// Processes a tone at fromSpeed, then at toSpeed, and returns the speeds the output after the
// change is stamped with. With fillPackets, packets are filled as with several streams or
// transcoding.
std::vector<double> SpeedsAfterAChange(double fromSpeed, double toSpeed, bool fillPackets = false)
{
  constexpr unsigned int SAMPLE_RATE = 48000;
  constexpr unsigned int CHUNK = 1024;
  constexpr double ATEMPO_THRESHOLD = 0.02;

  AEAudioFormat format;
  format.m_dataFormat = AE_FMT_FLOAT;
  format.m_sampleRate = SAMPLE_RATE;
  format.m_channelLayout = AE_CH_LAYOUT_2_0;
  format.m_frames = CHUNK;
  format.m_frameSize = format.m_channelLayout.Count() * sizeof(float);

  // The engine processes planar audio, so a stream's audio always goes through its resampler
  AEAudioFormat outputFormat = format;
  outputFormat.m_dataFormat = AE_FMT_FLOATP;
  outputFormat.m_frameSize = sizeof(float);

  std::vector<double> speeds;
  CActiveAEBufferPool input(format);
  CActiveAEStreamBuffers buffers(format, outputFormat, AE_QUALITY_MID);
  // As for a stream synced by resampling
  buffers.ForceResampler(true);
  if (fillPackets)
    buffers.FillBuffer();
  if (!input.Create(400) || !buffers.Create(400, false, false))
  {
    ADD_FAILURE() << "creating the buffers failed";
    return speeds;
  }
  buffers.SetRR(1.0 / fromSpeed, ATEMPO_THRESHOLD);

  uint64_t frame = 0;
  for (int i = 0; i < 100; ++i)
  {
    if (i == 50)
      buffers.SetRR(1.0 / toSpeed, ATEMPO_THRESHOLD);

    while (CSampleBuffer* in = input.GetFreeBuffer())
    {
      auto* samples = reinterpret_cast<float*>(in->pkt->data[0]);
      for (unsigned int j = 0; j < CHUNK; ++j)
      {
        const auto value = static_cast<float>(
            0.1 * std::sin(2.0 * std::numbers::pi * 440.0 * (frame + j) / SAMPLE_RATE));
        samples[2 * j] = value;
        samples[2 * j + 1] = value;
      }
      in->pkt->nb_samples = CHUNK;
      in->timestamp = static_cast<int64_t>(frame * 1000 / SAMPLE_RATE);
      in->pkt_start_offset = 0;
      frame += CHUNK;
      buffers.m_inputSamples.push_back(in);
    }

    buffers.ProcessBuffers();

    while (!buffers.m_outputSamples.empty())
    {
      CSampleBuffer* out = buffers.m_outputSamples.front();
      buffers.m_outputSamples.pop_front();
      if (i >= 50)
        speeds.push_back(out->mediaSpeed);
      out->Return();
    }
  }
  return speeds;
}

// A filter that can't change its tempo in place drains at the old one before it changes, so the
// output after a change has to be stamped with the old speed until then
void ExpectTheOldSpeedUntilTheFilterChanges(double fromSpeed, double toSpeed)
{
  const std::vector<double> speeds = SpeedsAfterAChange(fromSpeed, toSpeed);
  const auto changed = std::find_if(speeds.begin(), speeds.end(),
                                    [&](double speed) { return std::abs(speed - toSpeed) < 1e-6; });
  EXPECT_GT(std::distance(speeds.begin(), changed), 0) << "buffers drained at the old speed";
  EXPECT_GT(std::distance(changed, speeds.end()), 0) << "buffers produced at the new speed";
  for (auto it = speeds.begin(); it != changed; ++it)
    EXPECT_NEAR(*it, fromSpeed, 1e-6);
  for (auto it = changed; it != speeds.end(); ++it)
    EXPECT_NEAR(*it, toSpeed, 1e-6);
}
} // namespace

TEST(TestActiveAEStream, StampsTheOldSpeedOnWhatIsLeftAtTheEndOfATempo)
{
  ExpectTheOldSpeedUntilTheFilterChanges(2.0, 1.0);
}

TEST(TestActiveAEStream, StampsAPacketFilledAcrossAChangeWithTheAverageSpeed)
{
  // The tempo changes in place, while a packet is being filled
  const std::vector<double> speeds = SpeedsAfterAChange(1.5, 1.8, true);
  ASSERT_FALSE(speeds.empty());
  for (const double speed : speeds)
  {
    EXPECT_GE(speed, 1.5 - 1e-6);
    EXPECT_LE(speed, 1.8 + 1e-6);
  }
  EXPECT_TRUE(std::any_of(speeds.begin(), speeds.end(),
                          [](double speed) { return speed > 1.5 + 1e-6 && speed < 1.8 - 1e-6; }))
      << "no packet holds output at both speeds";
}

TEST(TestActiveAEStream, ForwardSpeedKeepsAClockRunningForward)
{
  EXPECT_DOUBLE_EQ(StreamHelpers::ForwardSpeed(1.0), 1.0);
  EXPECT_DOUBLE_EQ(StreamHelpers::ForwardSpeed(0.8), 0.8);
  EXPECT_DOUBLE_EQ(StreamHelpers::ForwardSpeed(2.085), 2.085);
}

TEST(TestActiveAEStream, ForwardSpeedIsNormalWhileTheClockDoesNotRunForward)
{
  EXPECT_DOUBLE_EQ(StreamHelpers::ForwardSpeed(0.0), 1.0);
  EXPECT_DOUBLE_EQ(StreamHelpers::ForwardSpeed(-2.0), 1.0);
}

TEST(TestActiveAEStream, RatioFollowsTheClockWithoutCorrection)
{
  EXPECT_DOUBLE_EQ(StreamHelpers::RatioForSpeed(1.0, 0.0), 1.0);
  EXPECT_DOUBLE_EQ(StreamHelpers::RatioForSpeed(2.0, 0.0), 0.5);
  EXPECT_DOUBLE_EQ(StreamHelpers::RatioForSpeed(0.8, 0.0), 1.25);
}

TEST(TestActiveAEStream, RatioCatchesUpWithinTwiceTheClockSpeed)
{
  // 2x tempo with 23.976 fps on a 100 Hz display. Adding the correction to the ratio instead,
  // 1 / 2.085 + correction, reaches zero at a correction of about -0.48.
  const double clockSpeed = 2.085;
  for (double correction = 0.0; correction >= -5.0; correction -= 0.01)
  {
    const double ratio = StreamHelpers::RatioForSpeed(clockSpeed, correction);
    ASSERT_GT(ratio, 0.0) << "correction " << correction;
    EXPECT_GE(1.0 / ratio, clockSpeed - 1e-9) << "correction " << correction;
    EXPECT_LE(1.0 / ratio, 2.0 * clockSpeed + 1e-9) << "correction " << correction;
  }
  EXPECT_DOUBLE_EQ(StreamHelpers::RatioForSpeed(clockSpeed, -5.0), 1.0 / (2.0 * clockSpeed));
}

TEST(TestActiveAEStream, RatioSlowsDownToHalfTheClockSpeedAtMost)
{
  EXPECT_DOUBLE_EQ(StreamHelpers::RatioForSpeed(2.0, 0.5), 1.0 / 1.5);
  EXPECT_DOUBLE_EQ(StreamHelpers::RatioForSpeed(2.0, 5.0), 1.0);
}

TEST(TestActiveAEStream, RatioMatchesTheFormerCorrectionAtNormalSpeed)
{
  // Small corrections at 1x are what they were when added to the ratio
  for (const double correction : {-0.01, -0.001, 0.001, 0.01})
    EXPECT_NEAR(StreamHelpers::RatioForSpeed(1.0, correction), 1.0 + correction, 2e-4)
        << "correction " << correction;
}

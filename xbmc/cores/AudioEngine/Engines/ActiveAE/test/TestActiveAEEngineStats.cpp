/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/AudioEngine/Engines/ActiveAE/ActiveAE.h"
#include "utils/TimeUtils.h"

#include <list>

#include <gtest/gtest.h>

using namespace ActiveAE;

namespace
{
constexpr unsigned int SAMPLE_RATE = 48000;
// 200 ms at 48 kHz
constexpr int SAMPLES = 9600;

const std::list<CActiveAEStream*> NO_STREAMS;

// A delay the sink reported the given time ago, as it reads from then on
AEDelayStatus SinkDelay(double seconds, double ago = 0.0)
{
  AEDelayStatus status;
  status.SetDelay(seconds);
  status.tick -= static_cast<int64_t>(ago * CurrentHostFrequency());
  status.maxcorrection = ago;
  return status;
}
} // namespace

TEST(TestActiveAEEngineStats, EmptyQueueIsAtNormalSpeed)
{
  CEngineStats stats;
  stats.Reset(SAMPLE_RATE, true);

  EXPECT_DOUBLE_EQ(stats.GetQueuedMediaSpeed(), 1.0);
}

TEST(TestActiveAEEngineStats, QueuedMediaSpeedOfOutputAtOneSpeed)
{
  CEngineStats stats;
  stats.Reset(SAMPLE_RATE, true);
  stats.AddSamples(SAMPLES, NO_STREAMS, 2.0);
  stats.AddSamples(SAMPLES, NO_STREAMS, 2.0);

  EXPECT_DOUBLE_EQ(stats.GetQueuedMediaSpeed(), 2.0);
}

TEST(TestActiveAEEngineStats, QueuedMediaSpeedFollowsTheOutputLeft)
{
  // The tempo changed from 1x to 2x, 200 ms of each are buffered
  CEngineStats stats;
  stats.Reset(SAMPLE_RATE, true);
  stats.AddSamples(SAMPLES, NO_STREAMS, 1.0);
  stats.AddSamples(SAMPLES, NO_STREAMS, 2.0);
  EXPECT_DOUBLE_EQ(stats.GetQueuedMediaSpeed(), 1.5);

  // The sink took the 1x output and has played all but 100 ms of it
  stats.UpdateSinkDelay(SinkDelay(0.1), SAMPLES);
  EXPECT_NEAR(stats.GetQueuedMediaSpeed(), (0.1 * 1.0 + 0.2 * 2.0) / 0.3, 1e-9);

  // and then all of it
  stats.UpdateSinkDelay(SinkDelay(0.0), 0);
  EXPECT_DOUBLE_EQ(stats.GetQueuedMediaSpeed(), 2.0);
}

TEST(TestActiveAEEngineStats, QueuedMediaSpeedOfTheOutputLeftSinceTheSinkReported)
{
  // The sink took 200 ms at 1x and 200 ms at 2x, and played 100 ms since it reported its delay
  CEngineStats stats;
  stats.Reset(SAMPLE_RATE, true);
  stats.AddSamples(SAMPLES, NO_STREAMS, 1.0);
  stats.AddSamples(SAMPLES, NO_STREAMS, 2.0);
  stats.UpdateSinkDelay(SinkDelay(0.4, 0.1), 2 * SAMPLES);

  EXPECT_NEAR(stats.GetQueuedMediaSpeed(), (0.1 * 1.0 + 0.2 * 2.0) / 0.3, 1e-9);
}

TEST(TestActiveAEEngineStats, QueuedMediaSpeedWhenTranscoding)
{
  // Encoded output is counted in packets rather than samples
  AEAudioFormat format;
  format.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_AC3;
  format.m_streamInfo.m_sampleRate = SAMPLE_RATE;

  CEngineStats stats;
  stats.Reset(SAMPLE_RATE, false);
  stats.SetCurrentSinkFormat(format);
  stats.AddSamples(1, NO_STREAMS, 1.0);
  stats.AddSamples(1, NO_STREAMS, 2.0);
  stats.AddSamples(1, NO_STREAMS, 2.0);

  EXPECT_NEAR(stats.GetQueuedMediaSpeed(), 5.0 / 3.0, 1e-9);
}

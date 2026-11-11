/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/AudioEngine/Engines/ActiveAE/ActiveAEStream.h"

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
} // namespace

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

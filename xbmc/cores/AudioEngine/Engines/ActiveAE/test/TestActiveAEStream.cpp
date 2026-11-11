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

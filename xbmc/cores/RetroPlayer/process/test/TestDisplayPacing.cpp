/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/process/DisplayPacing.h"

#include <chrono>

#include <gtest/gtest.h>

using namespace KODI::RETRO;
using namespace std::chrono_literals;

namespace
{
using Clock = CDisplayPacing::Clock;

constexpr auto REFRESH = std::chrono::nanoseconds(16'666'667);
const Clock::time_point START{1000s};

double Micros(Clock::duration duration)
{
  return std::chrono::duration<double, std::micro>(duration).count();
}
} // namespace

TEST(TestDisplayPacing, TakesTheIntervalFromTheRefreshRate)
{
  CDisplayPacing pacing;
  pacing.SetRefreshRateSource([] { return 60.0; });
  pacing.OnFrameTaken(START);

  EXPECT_NEAR(Micros(pacing.Interval(START)), Micros(REFRESH), 1.0);
}

TEST(TestDisplayPacing, HasNoIntervalWithoutARefreshRate)
{
  CDisplayPacing pacing;
  pacing.OnFrameTaken(START);
  EXPECT_EQ(pacing.Interval(START), Clock::duration::zero());

  // As the display's reference clock reports while the display changes mode
  pacing.SetRefreshRateSource([] { return -1.0; });
  EXPECT_EQ(pacing.Interval(START), Clock::duration::zero());
}

TEST(TestDisplayPacing, HasNoIntervalBeforeAFrameIsTaken)
{
  CDisplayPacing pacing;
  pacing.SetRefreshRateSource([] { return 60.0; });

  EXPECT_EQ(pacing.Interval(START), Clock::duration::zero());
}

TEST(TestDisplayPacing, FollowsAChangeOfRefreshRateAtOnce)
{
  CDisplayPacing pacing;
  double refreshRate = 60.0;
  pacing.SetRefreshRateSource([&refreshRate] { return refreshRate; });
  pacing.OnFrameTaken(START);

  refreshRate = 50.0;
  EXPECT_NEAR(Micros(pacing.Interval(START)), 20'000.0, 1.0);
}

TEST(TestDisplayPacing, PredictsTheNextFrameAfterNow)
{
  CDisplayPacing pacing;
  pacing.SetRefreshRateSource([] { return 60.0; });
  pacing.OnFrameTaken(START);

  EXPECT_NEAR(Micros(pacing.NextTake(START + 5ms) - START), Micros(REFRESH), 1.0);
  EXPECT_NEAR(Micros(pacing.NextTake(START + REFRESH + 1ms) - START), Micros(2 * REFRESH), 1.0);
}

TEST(TestDisplayPacing, StopsWhenFramesStopBeingTaken)
{
  CDisplayPacing pacing;
  pacing.SetRefreshRateSource([] { return 60.0; });
  pacing.OnFrameTaken(START);

  EXPECT_NE(pacing.Interval(START + 2 * REFRESH), Clock::duration::zero());
  EXPECT_EQ(pacing.Interval(START + 4 * REFRESH), Clock::duration::zero());
}

TEST(TestDisplayPacing, IgnoresASecondFrameInTheSameRefresh)
{
  CDisplayPacing pacing;
  pacing.SetRefreshRateSource([] { return 60.0; });
  pacing.OnFrameTaken(START);
  pacing.OnFrameTaken(START + 1ms);

  EXPECT_NEAR(Micros(pacing.NextTake(START + 2ms) - START), Micros(REFRESH), 1.0);
}

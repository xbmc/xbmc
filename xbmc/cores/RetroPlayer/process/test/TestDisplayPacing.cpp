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

// Takes a frame at each of the first count refreshes, returning the last
Clock::time_point TakeFrames(CDisplayPacing& pacing, int count)
{
  Clock::time_point when;
  for (int i = 0; i < count; ++i)
  {
    when = START + i * REFRESH;
    pacing.OnFrameTaken(when);
  }
  return when;
}
} // namespace

TEST(TestDisplayPacing, MeasuresTheRefreshThroughJitter)
{
  CDisplayPacing pacing;

  Clock::time_point when;
  for (int i = 0; i < 120; ++i)
  {
    // Up to 1 ms late, as a busy rendering thread takes them
    when = START + i * REFRESH + std::chrono::microseconds((i * 7919) % 1000);
    pacing.OnFrameTaken(when);
  }

  EXPECT_NEAR(Micros(pacing.Interval(when)), Micros(REFRESH), 10.0);
}

TEST(TestDisplayPacing, CountsARefreshThatTookNoFrame)
{
  CDisplayPacing pacing;

  Clock::time_point when;
  for (int i = 0; i < 60; ++i)
  {
    if (i == 40)
      continue;
    when = START + i * REFRESH;
    pacing.OnFrameTaken(when);
  }

  EXPECT_NEAR(Micros(pacing.Interval(when)), Micros(REFRESH), 1.0);
}

TEST(TestDisplayPacing, IgnoresASecondFrameInTheSameRefresh)
{
  CDisplayPacing pacing;

  Clock::time_point when = TakeFrames(pacing, 40);
  pacing.OnFrameTaken(when + 1ms);

  EXPECT_NEAR(Micros(pacing.Interval(when + 1ms)), Micros(REFRESH), 1.0);
}

TEST(TestDisplayPacing, PredictsTheNextFrameAfterNow)
{
  CDisplayPacing pacing;
  const Clock::time_point last = TakeFrames(pacing, 40);

  EXPECT_NEAR(Micros(pacing.NextTake(last + REFRESH / 5) - last), Micros(REFRESH), 1.0);
  EXPECT_NEAR(Micros(pacing.NextTake(last + REFRESH * 5 / 2) - last), Micros(REFRESH * 3), 1.0);
}

TEST(TestDisplayPacing, StopsWhenFramesStopBeingTaken)
{
  CDisplayPacing pacing;
  const Clock::time_point last = TakeFrames(pacing, 40);

  EXPECT_NE(pacing.Interval(last + REFRESH), Clock::duration::zero());
  EXPECT_EQ(pacing.Interval(last + REFRESH * 4), Clock::duration::zero());
}

TEST(TestDisplayPacing, NeedsARunOfFramesBeforeAnInterval)
{
  CDisplayPacing pacing;

  const Clock::time_point last = TakeFrames(pacing, 20);
  EXPECT_EQ(pacing.Interval(last), Clock::duration::zero());
}

TEST(TestDisplayPacing, StartsAgainAfterALongGap)
{
  CDisplayPacing pacing;
  const Clock::time_point last = TakeFrames(pacing, 40);

  const Clock::time_point resumed = last + 500ms;
  pacing.OnFrameTaken(resumed);
  EXPECT_EQ(pacing.Interval(resumed), Clock::duration::zero());

  Clock::time_point when;
  for (int i = 1; i <= 31; ++i)
  {
    when = resumed + i * REFRESH;
    pacing.OnFrameTaken(when);
  }
  EXPECT_NEAR(Micros(pacing.Interval(when)), Micros(REFRESH), 1.0);
}

TEST(TestDisplayPacing, KeepsTheIntervalThroughAnIrregularGap)
{
  CDisplayPacing pacing;
  Clock::time_point when = TakeFrames(pacing, 120);

  // A stall that isn't a whole number of refreshes, then a few regular frames
  when += REFRESH * 5 / 2;
  pacing.OnFrameTaken(when);
  for (int i = 0; i < 10; ++i)
  {
    when += REFRESH;
    pacing.OnFrameTaken(when);
  }

  EXPECT_NEAR(Micros(pacing.Interval(when)), Micros(REFRESH), 1.0);
}

TEST(TestDisplayPacing, IsNotSkewedByUnevenFramesAtTheStart)
{
  CDisplayPacing pacing;

  // Opening a game takes frames unevenly before it settles
  Clock::time_point when = START;
  for (const auto gap : {90ms, 40ms, 70ms, 25ms})
  {
    when += gap;
    pacing.OnFrameTaken(when);
  }
  for (int i = 0; i < 60; ++i)
  {
    when += REFRESH;
    pacing.OnFrameTaken(when);
  }

  EXPECT_NEAR(Micros(pacing.Interval(when)), Micros(REFRESH), 1.0);
}

namespace
{
// Takes count frames from when at the given refresh, returning the last
Clock::time_point TakeFramesAt(CDisplayPacing& pacing,
                               Clock::time_point when,
                               std::chrono::nanoseconds refresh,
                               int count)
{
  for (int i = 0; i < count; ++i)
  {
    when += refresh;
    pacing.OnFrameTaken(when);
  }
  return when;
}
} // namespace

TEST(TestDisplayPacing, FollowsTheScreenToAHigherRefreshRate)
{
  CDisplayPacing pacing;
  Clock::time_point when = TakeFrames(pacing, 1000);

  when = TakeFramesAt(pacing, when, REFRESH / 2, 400);

  EXPECT_NEAR(Micros(pacing.Interval(when)), Micros(REFRESH / 2), 1.0);
}

TEST(TestDisplayPacing, FollowsTheScreenToALowerRefreshRate)
{
  CDisplayPacing pacing;
  Clock::time_point when = TakeFramesAt(pacing, START, REFRESH / 2, 1000);

  // Every other refresh of the old rate, which a run at that rate takes for a
  // refresh that passed without a frame
  when = TakeFramesAt(pacing, when, REFRESH, 700);

  EXPECT_NEAR(Micros(pacing.Interval(when)), Micros(REFRESH), 1.0);
}

TEST(TestDisplayPacing, FollowsASmallChangeInTheRefreshRate)
{
  CDisplayPacing pacing;
  Clock::time_point when = TakeFrames(pacing, 1000);

  // Close enough to the old rate for each gap to pass as a refresh
  when = TakeFramesAt(pacing, when, 20ms, 700);

  EXPECT_NEAR(Micros(pacing.Interval(when)), 20'000.0, 1.0);
}

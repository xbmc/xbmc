/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/process/DisplayPacing.h"

#include <chrono>
#include <thread>

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

TEST(TestDisplayPacing, FollowsAGameTooFarFromTheModeToPace)
{
  // PAL, and Mortal Kombat's 54.7 fps, on a display with 40 to 60 Hz VRR
  EXPECT_TRUE(CDisplayPacing::FollowsGame(50.0, 40.0, 60.0, 0.02));
  EXPECT_TRUE(CDisplayPacing::FollowsGame(54.7, 40.0, 60.0, 0.02));
}

TEST(TestDisplayPacing, FollowsAGamePacingWouldSpeedUpAudibly)
{
  // Neo Geo would run 1.4% fast, CPS2 0.6%
  EXPECT_TRUE(CDisplayPacing::FollowsGame(59.19, 40.0, 60.0, 0.02));
  EXPECT_TRUE(CDisplayPacing::FollowsGame(59.637, 40.0, 60.0, 0.02));
}

TEST(TestDisplayPacing, PacesAGameCloseToTheModesRate)
{
  // The Mega Drive would run 0.13% fast, the GBA 0.45%
  EXPECT_FALSE(CDisplayPacing::FollowsGame(59.92, 40.0, 60.0, 0.02));
  EXPECT_FALSE(CDisplayPacing::FollowsGame(59.73, 40.0, 60.0, 0.02));
}

TEST(TestDisplayPacing, DoesNotFollowAGameOutsideTheRange)
{
  // The NES is faster than the mode, the Lynx more so
  EXPECT_FALSE(CDisplayPacing::FollowsGame(60.10, 40.0, 60.0, 0.02));
  EXPECT_FALSE(CDisplayPacing::FollowsGame(75.0, 40.0, 60.0, 0.02));
  EXPECT_FALSE(CDisplayPacing::FollowsGame(30.0, 40.0, 60.0, 0.02));
  EXPECT_FALSE(CDisplayPacing::FollowsGame(50.0, 0.0, 0.0, 0.02));
}

TEST(TestDisplayPacing, FollowsAGameInRangeThatIsNotPaced)
{
  EXPECT_TRUE(CDisplayPacing::FollowsGame(59.92, 40.0, 60.0, 0.0));
}

TEST(TestDisplayPacing, WaitsForANewFrameOnlyWhileFollowed)
{
  CDisplayPacing pacing;
  pacing.OnFrameRun();
  const Clock::time_point start = Clock::now();
  pacing.WaitForNextFrame();
  EXPECT_LT(Clock::now() - start, 1s);

  // The frame run while the display kept to its own rate isn't waited for
  pacing.SetFollowedFrameTime(20ms);
  const Clock::time_point timed = Clock::now();
  pacing.WaitForNextFrame();
  EXPECT_GE(Clock::now() - timed, 20ms);
}

TEST(TestDisplayPacing, StopsWaitingWhenTheGameRunsAFrame)
{
  CDisplayPacing pacing;
  pacing.SetFollowedFrameTime(10s);
  std::thread game(
      [&pacing]
      {
        std::this_thread::sleep_for(10ms);
        pacing.OnFrameRun();
      });

  const Clock::time_point start = Clock::now();
  pacing.WaitForNextFrame();
  EXPECT_LT(Clock::now() - start, 5s);
  game.join();

  // One run before the wait is drawn without waiting
  pacing.OnFrameRun();
  const Clock::time_point next = Clock::now();
  pacing.WaitForNextFrame();
  EXPECT_LT(Clock::now() - next, 5s);
}

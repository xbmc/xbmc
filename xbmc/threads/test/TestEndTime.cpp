/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "threads/SystemClock.h"

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{

template<typename T = std::chrono::milliseconds>
void CommonTests(XbmcThreads::EndTime<T>& endTime)
{
  EXPECT_EQ(100ms, endTime.GetInitialTimeoutValue());
  EXPECT_LT(T::zero(), endTime.GetStartTime().time_since_epoch());

  EXPECT_FALSE(endTime.IsTimePast());
  EXPECT_LT(T::zero(), endTime.GetTimeLeft());

  std::this_thread::sleep_for(100ms);

  EXPECT_TRUE(endTime.IsTimePast());
  EXPECT_EQ(T::zero(), endTime.GetTimeLeft());

  endTime.SetInfinite();
  EXPECT_GE(T::max(), endTime.GetInitialTimeoutValue());
  EXPECT_EQ(XbmcThreads::EndTime<T>::Max(), endTime.GetInitialTimeoutValue());
  endTime.SetExpired();
  EXPECT_EQ(T::zero(), endTime.GetInitialTimeoutValue());
}

} // namespace

TEST(TestEndTime, DefaultConstructor)
{
  XbmcThreads::EndTime<> endTime;
  endTime.Set(100ms);

  CommonTests(endTime);
}

TEST(TestEndTime, ExplicitConstructor)
{
  XbmcThreads::EndTime<> endTime(100ms);

  CommonTests(endTime);
}

TEST(TestEndTime, DoubleMicroSeconds)
{
  XbmcThreads::EndTime<std::chrono::duration<double, std::micro>> endTime(100ms);

  CommonTests(endTime);
}

TEST(TestEndTime, SteadyClockDuration)
{
  XbmcThreads::EndTime<std::chrono::steady_clock::duration> endTime(100ms);

  CommonTests(endTime);

  endTime.SetInfinite();
  EXPECT_EQ(std::chrono::steady_clock::duration::max(), endTime.GetInitialTimeoutValue());

  endTime.SetExpired();
  EXPECT_EQ(std::chrono::steady_clock::duration::zero(), endTime.GetInitialTimeoutValue());

  endTime.Set(endTime.Max());
  EXPECT_EQ(std::chrono::steady_clock::duration::max(), endTime.GetInitialTimeoutValue());
}

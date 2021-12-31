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

void CommonTests(XbmcThreads::EndTime<>& endTime)
{
  EXPECT_EQ(100ms, endTime.GetInitialTimeoutValue());
  EXPECT_LT(0ms, endTime.GetStartTime().time_since_epoch());

  EXPECT_FALSE(endTime.IsTimePast());
  EXPECT_LT(0ms, endTime.GetTimeLeft());

  std::this_thread::sleep_for(100ms);

  EXPECT_TRUE(endTime.IsTimePast());
  EXPECT_EQ(0ms, endTime.GetTimeLeft());

  endTime.SetInfinite();
  EXPECT_EQ(std::chrono::milliseconds::max(), endTime.GetInitialTimeoutValue());
  endTime.SetExpired();
  EXPECT_EQ(0ms, endTime.GetInitialTimeoutValue());
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

/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "threads/SystemClock.h"

#include <gtest/gtest.h>

namespace
{

void CommonTests(XbmcThreads::EndTime& endTime)
{
  EXPECT_EQ(static_cast<unsigned int>(100), endTime.GetInitialTimeoutValue());
  EXPECT_LT(static_cast<unsigned int>(0), endTime.GetStartTime());

  EXPECT_FALSE(endTime.IsTimePast());
  EXPECT_LT(static_cast<unsigned int>(0), endTime.MillisLeft());

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(endTime.IsTimePast());
  EXPECT_EQ(static_cast<unsigned int>(0), endTime.MillisLeft());

  endTime.SetInfinite();
  EXPECT_EQ(std::numeric_limits<unsigned int>::max(), endTime.GetInitialTimeoutValue());
  endTime.SetExpired();
  EXPECT_EQ(static_cast<unsigned int>(0), endTime.GetInitialTimeoutValue());
}

} // namespace

TEST(TestEndTime, DefaultConstructor)
{
  XbmcThreads::EndTime endTime;
  endTime.Set(static_cast<unsigned int>(100));

  CommonTests(endTime);
}

TEST(TestEndTime, ExplicitConstructor)
{
  XbmcThreads::EndTime endTime(static_cast<unsigned int>(100));

  CommonTests(endTime);
}

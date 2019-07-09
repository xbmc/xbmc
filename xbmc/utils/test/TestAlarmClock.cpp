/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/AlarmClock.h"

#include <gtest/gtest.h>

TEST(TestAlarmClock, General)
{
  CAlarmClock a;
  EXPECT_FALSE(a.IsRunning());
  EXPECT_FALSE(a.HasAlarm("test"));
  a.Start("test", 100.f, "test");
  EXPECT_TRUE(a.IsRunning());
  EXPECT_TRUE(a.HasAlarm("test"));
  EXPECT_FALSE(a.HasAlarm("test2"));
  EXPECT_NE(0.f, a.GetRemaining("test"));
  EXPECT_EQ(0.f, a.GetRemaining("test2"));
  a.Stop("test");
}

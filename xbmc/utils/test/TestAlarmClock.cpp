/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/AlarmClock.h"

#include "gtest/gtest.h"

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

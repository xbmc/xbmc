/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "XBDateTime.h"

#include <gtest/gtest.h>

class TestDateTimeSpan : public testing::Test
{
protected:
  TestDateTimeSpan() = default;
  ~TestDateTimeSpan() override = default;
};

TEST_F(TestDateTimeSpan, Operators)
{
  CDateTimeSpan timeSpan1(1, 1, 1, 1);
  CDateTimeSpan timeSpan2(2, 2, 2, 2);

  EXPECT_FALSE(timeSpan1 > timeSpan2);
  EXPECT_TRUE(timeSpan1 < timeSpan2);

  CDateTimeSpan timeSpan3(timeSpan1);
  EXPECT_TRUE(timeSpan1 == timeSpan3);

  EXPECT_TRUE((timeSpan1 + timeSpan3) == timeSpan2);
  EXPECT_TRUE((timeSpan2 - timeSpan3) == timeSpan1);

  timeSpan1 += timeSpan3;
  EXPECT_TRUE(timeSpan1 == timeSpan2);

  timeSpan1 -= timeSpan3;
  EXPECT_TRUE(timeSpan1 == timeSpan3);
}

TEST_F(TestDateTimeSpan, SetDateTimeSpan)
{
  CDateTimeSpan timeSpan;
  int days = 1;
  int hours = 2;
  int minutes = 3;
  int seconds = 4;

  int secondsTotal = (days * 24 * 60 * 60) + (hours * 60 * 60) + (minutes * 60) + seconds;

  timeSpan.SetDateTimeSpan(days, hours, minutes, seconds);
  EXPECT_EQ(timeSpan.GetDays(), days);
  EXPECT_EQ(timeSpan.GetHours(), hours);
  EXPECT_EQ(timeSpan.GetMinutes(), minutes);
  EXPECT_EQ(timeSpan.GetSeconds(), seconds);
  EXPECT_EQ(timeSpan.GetSecondsTotal(), secondsTotal);
}

TEST_F(TestDateTimeSpan, SetFromPeriod)
{
  CDateTimeSpan timeSpan;

  timeSpan.SetFromPeriod("3");
  EXPECT_EQ(timeSpan.GetDays(), 3);

  timeSpan.SetFromPeriod("3weeks");
  EXPECT_EQ(timeSpan.GetDays(), 21);

  timeSpan.SetFromPeriod("3months");
  EXPECT_EQ(timeSpan.GetDays(), 93);
}

TEST_F(TestDateTimeSpan, SetFromTimeString)
{
  CDateTimeSpan timeSpan;

  timeSpan.SetFromTimeString("12:34");
  EXPECT_EQ(timeSpan.GetHours(), 12);
  EXPECT_EQ(timeSpan.GetMinutes(), 34);
}

/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LangInfo.h"
#include "XBDateTime.h"
#include "guilib/LocalizeStrings.h"

#include <array>
#include <chrono>
#include <iostream>

#define USE_OS_TZDB 0
#define HAS_REMOTE_API 0
#include <date/date.h>
#include <date/tz.h>
#include <gtest/gtest.h>

class TestDateTime : public testing::Test
{
protected:
  TestDateTime() = default;
  ~TestDateTime() override = default;
};

TEST_F(TestDateTime, DateTimeOperators)
{
  CDateTime dateTime1(1991, 5, 14, 12, 34, 56);
  CDateTime dateTime2(1991, 5, 14, 12, 34, 57);

  EXPECT_TRUE(dateTime1 < dateTime2);
  EXPECT_FALSE(dateTime1 > dateTime2);
  EXPECT_FALSE(dateTime1 == dateTime2);
}

TEST_F(TestDateTime, TimePointOperators)
{
  CDateTime dateTime1(1991, 5, 14, 12, 34, 56);
  CDateTime dateTime2(1991, 5, 14, 12, 34, 57);

  auto tp = dateTime2.GetAsTimePoint();

  EXPECT_TRUE(dateTime1 < tp);
  EXPECT_FALSE(dateTime1 > tp);
  EXPECT_FALSE(dateTime1 == tp);
}

TEST_F(TestDateTime, TimeTOperators)
{
  CDateTime dateTime1(1991, 5, 14, 12, 34, 56);
  CDateTime dateTime2(1991, 5, 14, 12, 34, 57);

  time_t time;
  dateTime2.GetAsTime(time);

  EXPECT_TRUE(dateTime1 < time);
  EXPECT_FALSE(dateTime1 > time);
  EXPECT_FALSE(dateTime1 == time);
}

TEST_F(TestDateTime, TmOperators)
{
  CDateTime dateTime1(1991, 5, 14, 12, 34, 56);
  CDateTime dateTime2(1991, 5, 14, 12, 34, 57);

  tm t;
  dateTime2.GetAsTm(t);

  EXPECT_TRUE(dateTime1 < t);
  EXPECT_FALSE(dateTime1 > t);
  EXPECT_FALSE(dateTime1 == t);
}

// no way to test this platform agnostically (for now) so just log it.
TEST_F(TestDateTime, GetCurrentDateTime)
{
  auto date = CDateTime::GetCurrentDateTime();
  std::cout << "Current Date: " << date.GetAsDBDateTime() << std::endl;
}

// no way to test this platform agnostically (for now) so just log it.
TEST_F(TestDateTime, GetUTCDateTime)
{
  auto date = CDateTime::GetUTCDateTime();
  std::cout << "Current Date UTC: " << date.GetAsDBDateTime() << std::endl;
}

TEST_F(TestDateTime, MonthStringToMonthNum)
{
  std::array<std::pair<std::string, std::string>, 12> months = {{
      {"Jan", "January"},
      {"Feb", "February"},
      {"Mar", "March"},
      {"Apr", "April"},
      {"May", "May"},
      {"Jun", "June"},
      {"Jul", "July"},
      {"Aug", "August"},
      {"Sep", "September"},
      {"Oct", "October"},
      {"Nov", "November"},
      {"Dec", "December"},
  }};

  int i = 1;
  for (const auto& month : months)
  {
    EXPECT_EQ(CDateTime::MonthStringToMonthNum(month.first), i);
    EXPECT_EQ(CDateTime::MonthStringToMonthNum(month.second), i);
    i++;
  }
}

// this method is broken as SetFromDBDate() will return true
TEST_F(TestDateTime, SetFromDateString)
{
  CDateTime dateTime;
  EXPECT_TRUE(dateTime.SetFromDateString("tuesday may 14, 1991"));

  std::cout << "year: " << dateTime.GetYear() << std::endl;
  std::cout << "month: " << dateTime.GetMonth() << std::endl;
  std::cout << "day: " << dateTime.GetDay() << std::endl;

  EXPECT_EQ(dateTime.GetYear(), 1991);
  EXPECT_EQ(dateTime.GetMonth(), 5);
  EXPECT_EQ(dateTime.GetDay(), 14);
}

TEST_F(TestDateTime, SetFromDBDate)
{
  CDateTime dateTime;
  EXPECT_TRUE(dateTime.SetFromDBDate("1991-05-14"));
  EXPECT_EQ(dateTime.GetYear(), 1991);
  EXPECT_EQ(dateTime.GetMonth(), 5);
  EXPECT_EQ(dateTime.GetDay(), 14);

  dateTime.Reset();
  EXPECT_TRUE(dateTime.SetFromDBDate("02-01-1993"));
  EXPECT_EQ(dateTime.GetYear(), 1993);
  EXPECT_EQ(dateTime.GetMonth(), 1);
  EXPECT_EQ(dateTime.GetDay(), 2);
}

TEST_F(TestDateTime, SetFromDBTime)
{
  CDateTime dateTime1;
  EXPECT_TRUE(dateTime1.SetFromDBTime("12:34"));
  EXPECT_EQ(dateTime1.GetHour(), 12);
  EXPECT_EQ(dateTime1.GetMinute(), 34);
  EXPECT_EQ(dateTime1.GetSecond(), 0);

  CDateTime dateTime2;
  EXPECT_TRUE(dateTime2.SetFromDBTime("12:34:56"));
  EXPECT_EQ(dateTime2.GetHour(), 12);
  EXPECT_EQ(dateTime2.GetMinute(), 34);
  EXPECT_EQ(dateTime2.GetSecond(), 56);
}

TEST_F(TestDateTime, SetFromDBDateTime)
{
  CDateTime dateTime;
  EXPECT_TRUE(dateTime.SetFromDBDateTime("1991-05-14 12:34:56"));
  EXPECT_EQ(dateTime.GetYear(), 1991);
  EXPECT_EQ(dateTime.GetMonth(), 5);
  EXPECT_EQ(dateTime.GetDay(), 14);
  EXPECT_EQ(dateTime.GetHour(), 12);
  EXPECT_EQ(dateTime.GetMinute(), 34);
  EXPECT_EQ(dateTime.GetSecond(), 56);
}

TEST_F(TestDateTime, SetFromW3CDate)
{
  CDateTime dateTime;
  EXPECT_TRUE(dateTime.SetFromW3CDate("1994-11-05T13:15:30Z"));
  EXPECT_EQ(dateTime.GetYear(), 1994);
  EXPECT_EQ(dateTime.GetMonth(), 11);
  EXPECT_EQ(dateTime.GetDay(), 5);
  EXPECT_EQ(dateTime.GetHour(), 0);
  EXPECT_EQ(dateTime.GetMinute(), 0);
  EXPECT_EQ(dateTime.GetSecond(), 0);
}

TEST_F(TestDateTime, SetFromW3CDateTime)
{
  CDateTime dateTime;
  dateTime.SetFromDBDateTime("1994-11-05 13:15:30");
  std::string dateTimeStr = dateTime.GetAsDBDate() + "T" + dateTime.GetAsDBTime() + "Z";

  CDateTime dateTime1;
  EXPECT_TRUE(dateTime1.SetFromW3CDateTime(dateTimeStr));
  EXPECT_EQ(dateTime1.GetYear(), 1994);
  EXPECT_EQ(dateTime1.GetMonth(), 11);
  EXPECT_EQ(dateTime1.GetDay(), 5);
  EXPECT_EQ(dateTime1.GetHour(), 13);
  EXPECT_EQ(dateTime1.GetMinute(), 15);
  EXPECT_EQ(dateTime1.GetSecond(), 30);

  CDateTime dateTime2;
  EXPECT_TRUE(dateTime2.SetFromW3CDateTime("1994-11-05T08:15:30-05:00"));
  EXPECT_EQ(dateTime2.GetYear(), 1994);
  EXPECT_EQ(dateTime2.GetMonth(), 11);
  EXPECT_EQ(dateTime2.GetDay(), 5);
  EXPECT_EQ(dateTime2.GetHour(), 13);
  EXPECT_EQ(dateTime2.GetMinute(), 15);
  EXPECT_EQ(dateTime2.GetSecond(), 30);
}

TEST_F(TestDateTime, SetFromUTCDateTime)
{
  CDateTime dateTime1;
  dateTime1.SetFromDBDateTime("1991-05-14 12:34:56");

  CDateTime dateTime2;
  EXPECT_TRUE(dateTime2.SetFromUTCDateTime(dateTime1));
  EXPECT_EQ(dateTime2.GetYear(), 1991);
  EXPECT_EQ(dateTime2.GetMonth(), 5);
  EXPECT_EQ(dateTime2.GetDay(), 14);
  EXPECT_EQ(dateTime2.GetHour(), 12);
  EXPECT_EQ(dateTime2.GetMinute(), 34);
  EXPECT_EQ(dateTime2.GetSecond(), 56);

  const time_t time = 674224496;

  CDateTime dateTime3;
  EXPECT_TRUE(dateTime3.SetFromUTCDateTime(time));
  EXPECT_EQ(dateTime3.GetYear(), 1991);
  EXPECT_EQ(dateTime3.GetMonth(), 5);
  EXPECT_EQ(dateTime3.GetDay(), 14);
  EXPECT_EQ(dateTime3.GetHour(), 12);
  EXPECT_EQ(dateTime3.GetMinute(), 34);
  EXPECT_EQ(dateTime3.GetSecond(), 56);
}

TEST_F(TestDateTime, SetFromRFC1123DateTime)
{
  std::string dateTime1("Mon, 21 Oct 2018 12:16:24 GMT");

  CDateTime dateTime2;
  EXPECT_TRUE(dateTime2.SetFromRFC1123DateTime(dateTime1));
  EXPECT_EQ(dateTime2.GetYear(), 2018);
  EXPECT_EQ(dateTime2.GetMonth(), 10);
  EXPECT_EQ(dateTime2.GetDay(), 21);
  EXPECT_EQ(dateTime2.GetHour(), 12);
  EXPECT_EQ(dateTime2.GetMinute(), 16);
  EXPECT_EQ(dateTime2.GetSecond(), 24);
}

TEST_F(TestDateTime, SetDateTime)
{
  CDateTime dateTime1;
  EXPECT_TRUE(dateTime1.SetDateTime(1991, 05, 14, 12, 34, 56));
  EXPECT_EQ(dateTime1.GetYear(), 1991);
  EXPECT_EQ(dateTime1.GetMonth(), 5);
  EXPECT_EQ(dateTime1.GetDay(), 14);
  EXPECT_EQ(dateTime1.GetHour(), 12);
  EXPECT_EQ(dateTime1.GetMinute(), 34);
  EXPECT_EQ(dateTime1.GetSecond(), 56);

  CDateTime dateTime2;
  EXPECT_TRUE(dateTime2.SetDate(1991, 05, 14));
  EXPECT_EQ(dateTime2.GetYear(), 1991);
  EXPECT_EQ(dateTime2.GetMonth(), 5);
  EXPECT_EQ(dateTime2.GetDay(), 14);
  EXPECT_EQ(dateTime2.GetHour(), 0);
  EXPECT_EQ(dateTime2.GetMinute(), 0);
  EXPECT_EQ(dateTime2.GetSecond(), 0);

  CDateTime dateTime3;
  EXPECT_TRUE(dateTime3.SetTime(12, 34, 56));
  EXPECT_EQ(dateTime3.GetYear(), 1970);
  EXPECT_EQ(dateTime3.GetMonth(), 1);
  EXPECT_EQ(dateTime3.GetDay(), 1);
  EXPECT_EQ(dateTime3.GetHour(), 12);
  EXPECT_EQ(dateTime3.GetMinute(), 34);
  EXPECT_EQ(dateTime3.GetSecond(), 56);
}

TEST_F(TestDateTime, GetAsStrings)
{
  CDateTime dateTime;
  dateTime.SetDateTime(1991, 05, 14, 12, 34, 56);

  EXPECT_EQ(dateTime.GetAsSaveString(), "19910514_123456");
  EXPECT_EQ(dateTime.GetAsDBDateTime(), "1991-05-14 12:34:56");
  EXPECT_EQ(dateTime.GetAsDBDate(), "1991-05-14");
  EXPECT_EQ(dateTime.GetAsDBTime(), "12:34:56");
  EXPECT_EQ(dateTime.GetAsW3CDate(), "1991-05-14");
}

TEST_F(TestDateTime, GetAsStringsWithBias)
{
  CDateTime dateTime;
  dateTime.SetDateTime(1991, 05, 14, 12, 34, 56);

  std::cout << dateTime.GetAsRFC1123DateTime() << std::endl;
  std::cout << dateTime.GetAsW3CDateTime(false) << std::endl;
  std::cout << dateTime.GetAsW3CDateTime(true) << std::endl;

  auto tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  auto zone = date::make_zoned(date::current_zone(), tps);

  EXPECT_EQ(dateTime.GetAsRFC1123DateTime(), "Tue, 14 May 1991 12:34:56 GMT");
  EXPECT_EQ(dateTime.GetAsW3CDateTime(false), date::format("%FT%T%Ez", zone));
  EXPECT_EQ(dateTime.GetAsW3CDateTime(true), "1991-05-14T12:34:56Z");
}

TEST_F(TestDateTime, GetAsLocalized)
{
  // short date formats using "/"
  // "DD/MM/YYYY",
  // "MM/DD/YYYY",
  // "YYYY/MM/DD",
  // "D/M/YYYY",
  // short date formats using "-"
  // "DD-MM-YYYY",
  // "MM-DD-YYYY",
  // "YYYY-MM-DD",
  // "YYYY-M-D",
  // short date formats using "."
  // "DD.MM.YYYY",
  // "DD.M.YYYY",
  // "D.M.YYYY",
  // "D. M. YYYY",
  // "YYYY.MM.DD"

  // "DDDD, D MMMM YYYY",
  // "DDDD, DD MMMM YYYY",
  // "DDDD, D. MMMM YYYY",
  // "DDDD, DD. MMMM YYYY",
  // "DDDD, MMMM D, YYYY",
  // "DDDD, MMMM DD, YYYY",
  // "DDDD D MMMM YYYY",
  // "DDDD DD MMMM YYYY",
  // "DDDD D. MMMM YYYY",
  // "DDDD DD. MMMM YYYY",
  // "D. MMMM YYYY",
  // "DD. MMMM YYYY",
  // "D. MMMM. YYYY",
  // "DD. MMMM. YYYY",
  // "YYYY. MMMM. D"

  ASSERT_TRUE(g_localizeStrings.Load(g_langInfo.GetLanguagePath(), "resource.language.en_gb"));

  // 24 hour clock must be set before time format
  g_langInfo.Set24HourClock(false);
  g_langInfo.SetTimeFormat("hh:mm:ss");

  g_langInfo.SetShortDateFormat("MM/DD/YYYY");
  g_langInfo.SetLongDateFormat("DDDD, DD MMMM YYYY");

  CDateTime dateTime1;
  dateTime1.SetDateTime(1991, 05, 14, 12, 34, 56);

  // std::cout << "GetAsLocalizedDate: " << dateTime1.GetAsLocalizedDate(false) << std::endl;
  // std::cout << "GetAsLocalizedDate: " << dateTime1.GetAsLocalizedDate(true) << std::endl;
  // std::cout << "GetAsLocalizedDate: " << dateTime1.GetAsLocalizedDate(std::string("dd-mm-yyyy")) << std::endl;
  // std::cout << "GetAsLocalizedTime: " << dateTime1.GetAsLocalizedTime("hh-mm-ss", true) << std::endl;
  // std::cout << "GetAsLocalizedTime: " << dateTime1.GetAsLocalizedTime("hh-mm-ss", false)
  //           << std::endl;
  // std::cout << "GetAsLocalizedDateTime: " << dateTime1.GetAsLocalizedDateTime(false, false)
  //           << std::endl;
  // std::cout << "GetAsLocalizedDateTime: " << dateTime1.GetAsLocalizedDateTime(true, true)
  //           << std::endl;
  // std::cout << "GetAsLocalizedTime: " << dateTime1.GetAsLocalizedTime(TIME_FORMAT(0), false)
  //           << std::endl;
  // std::cout << "GetAsLocalizedTime: " << dateTime1.GetAsLocalizedTime(TIME_FORMAT(0), true)
  //           << std::endl;

  // std::cout << "1: " << dateTime1.GetAsLocalizedTime(TIME_FORMAT(1)) << std::endl;
  // std::cout << "2: " << dateTime1.GetAsLocalizedTime(TIME_FORMAT(2)) << std::endl;
  // std::cout << "3: " << dateTime1.GetAsLocalizedTime(TIME_FORMAT(3)) << std::endl;
  // std::cout << "4: " << dateTime1.GetAsLocalizedTime(TIME_FORMAT(4)) << std::endl;
  // std::cout << "5: " << dateTime1.GetAsLocalizedTime(TIME_FORMAT(5)) << std::endl;
  // std::cout << "6: " << dateTime1.GetAsLocalizedTime(TIME_FORMAT(6)) << std::endl;
  // std::cout << "7: " << dateTime1.GetAsLocalizedTime(TIME_FORMAT(7)) << std::endl;
  // std::cout << "8: " << dateTime1.GetAsLocalizedTime(TIME_FORMAT(8)) << std::endl;
  // std::cout << "14: " << dateTime1.GetAsLocalizedTime(TIME_FORMAT(14)) << std::endl;
  // std::cout << "15: " << dateTime1.GetAsLocalizedTime(TIME_FORMAT(15)) << std::endl;
  // std::cout << "16: " << dateTime1.GetAsLocalizedTime(TIME_FORMAT(16)) << std::endl;
  // std::cout << "19: " << dateTime1.GetAsLocalizedTime(TIME_FORMAT(19)) << std::endl;
  // std::cout << "27: " << dateTime1.GetAsLocalizedTime(TIME_FORMAT(27)) << std::endl;
  // std::cout << "32: " << dateTime1.GetAsLocalizedTime(TIME_FORMAT(32)) << std::endl;
  // std::cout << "64: " << dateTime1.GetAsLocalizedTime(TIME_FORMAT(64)) << std::endl;
  // std::cout << "128: " << dateTime1.GetAsLocalizedTime(TIME_FORMAT(128)) << std::endl;
  // std::cout << "256: " << dateTime1.GetAsLocalizedTime(TIME_FORMAT(256)) << std::endl;

  EXPECT_EQ(dateTime1.GetAsLocalizedDate(false), "05/14/1991");
  EXPECT_EQ(dateTime1.GetAsLocalizedDate(true), "Tuesday, 14 May 1991");
  EXPECT_EQ(dateTime1.GetAsLocalizedDate(std::string("dd-mm-yyyy")),
            "14-05-1991"); // need to force overload function
  EXPECT_EQ(dateTime1.GetAsLocalizedTime("hh-mm-ss", true), "12-34-56");
  EXPECT_EQ(dateTime1.GetAsLocalizedTime("hh-mm-ss", false), "12-34");
  EXPECT_EQ(dateTime1.GetAsLocalizedDateTime(false, false), "05/14/1991 12:34");
  EXPECT_EQ(dateTime1.GetAsLocalizedDateTime(true, true), "Tuesday, 14 May 1991 12:34:56");

  EXPECT_EQ(dateTime1.GetAsLocalizedTime(TIME_FORMAT(0), false), "12:34");
  EXPECT_EQ(dateTime1.GetAsLocalizedTime(TIME_FORMAT(0), true), "12:34:56");

  EXPECT_EQ(dateTime1.GetAsLocalizedTime(TIME_FORMAT(1)), "56");
  EXPECT_EQ(dateTime1.GetAsLocalizedTime(TIME_FORMAT(2)), "34");
  EXPECT_EQ(dateTime1.GetAsLocalizedTime(TIME_FORMAT(3)), "34:56");
  EXPECT_EQ(dateTime1.GetAsLocalizedTime(TIME_FORMAT(4)), "12");
  EXPECT_EQ(dateTime1.GetAsLocalizedTime(TIME_FORMAT(5)), "12:56");
  EXPECT_EQ(dateTime1.GetAsLocalizedTime(TIME_FORMAT(6)), "12:34");
  EXPECT_EQ(dateTime1.GetAsLocalizedTime(TIME_FORMAT(7)), "12:34:56");
  EXPECT_EQ(dateTime1.GetAsLocalizedTime(TIME_FORMAT(8)), "PM");
  EXPECT_EQ(dateTime1.GetAsLocalizedTime(TIME_FORMAT(14)), "12:34 PM");
  EXPECT_EQ(dateTime1.GetAsLocalizedTime(TIME_FORMAT(15)), "12:34:56 PM");
  EXPECT_EQ(dateTime1.GetAsLocalizedTime(TIME_FORMAT(16)), "12");
  EXPECT_EQ(dateTime1.GetAsLocalizedTime(TIME_FORMAT(19)), "12:34:56");
  EXPECT_EQ(dateTime1.GetAsLocalizedTime(TIME_FORMAT(27)), "12:34:56 PM");

  // not possible to use these three
  // EXPECT_EQ(dateTime1.GetAsLocalizedTime(TIME_FORMAT(32)), "");
  // EXPECT_EQ(dateTime1.GetAsLocalizedTime(TIME_FORMAT(64)), "");
  // EXPECT_EQ(dateTime1.GetAsLocalizedTime(TIME_FORMAT(128)), "");

  EXPECT_EQ(dateTime1.GetAsLocalizedTime(TIME_FORMAT(256)), "34");


  // 24 hour clock must be set before time format
  g_langInfo.Set24HourClock(true);
  g_langInfo.SetTimeFormat("h:m:s");

  g_langInfo.SetShortDateFormat("YYYY-M-D");
  g_langInfo.SetLongDateFormat("DDDD, MMMM D, YYYY");

  CDateTime dateTime2;
  dateTime2.SetDateTime(2020, 2, 3, 4, 5, 6);

  // std::cout << "GetAsLocalizedDate: " << dateTime2.GetAsLocalizedDate(false) << std::endl;
  // std::cout << "GetAsLocalizedDate: " << dateTime2.GetAsLocalizedDate(true) << std::endl;
  // std::cout << "GetAsLocalizedDate: " << dateTime2.GetAsLocalizedDate(std::string("dd-mm-yyyy")) << std::endl;
  // std::cout << "GetAsLocalizedTime: " << dateTime2.GetAsLocalizedTime("hh-mm-ss", true) << std::endl;
  // std::cout << "GetAsLocalizedTime: " << dateTime2.GetAsLocalizedTime("hh-mm-ss", false)
  //           << std::endl;
  // std::cout << "GetAsLocalizedDateTime: " << dateTime2.GetAsLocalizedDateTime(false, false)
  //           << std::endl;
  // std::cout << "GetAsLocalizedDateTime: " << dateTime2.GetAsLocalizedDateTime(true, true)
  //           << std::endl;
  // std::cout << "GetAsLocalizedTime: " << dateTime2.GetAsLocalizedTime(TIME_FORMAT(0), false)
  //           << std::endl;
  // std::cout << "GetAsLocalizedTime: " << dateTime2.GetAsLocalizedTime(TIME_FORMAT(0), true)
  //           << std::endl;

  // std::cout << "1: " << dateTime2.GetAsLocalizedTime(TIME_FORMAT(1)) << std::endl;
  // std::cout << "2: " << dateTime2.GetAsLocalizedTime(TIME_FORMAT(2)) << std::endl;
  // std::cout << "3: " << dateTime2.GetAsLocalizedTime(TIME_FORMAT(3)) << std::endl;
  // std::cout << "4: " << dateTime2.GetAsLocalizedTime(TIME_FORMAT(4)) << std::endl;
  // std::cout << "5: " << dateTime2.GetAsLocalizedTime(TIME_FORMAT(5)) << std::endl;
  // std::cout << "6: " << dateTime2.GetAsLocalizedTime(TIME_FORMAT(6)) << std::endl;
  // std::cout << "7: " << dateTime2.GetAsLocalizedTime(TIME_FORMAT(7)) << std::endl;
  // std::cout << "8: " << dateTime2.GetAsLocalizedTime(TIME_FORMAT(8)) << std::endl;
  // std::cout << "14: " << dateTime2.GetAsLocalizedTime(TIME_FORMAT(14)) << std::endl;
  // std::cout << "15: " << dateTime2.GetAsLocalizedTime(TIME_FORMAT(15)) << std::endl;
  // std::cout << "16: " << dateTime2.GetAsLocalizedTime(TIME_FORMAT(16)) << std::endl;
  // std::cout << "19: " << dateTime2.GetAsLocalizedTime(TIME_FORMAT(19)) << std::endl;
  // std::cout << "27: " << dateTime2.GetAsLocalizedTime(TIME_FORMAT(27)) << std::endl;
  // std::cout << "32: " << dateTime2.GetAsLocalizedTime(TIME_FORMAT(32)) << std::endl;
  // std::cout << "64: " << dateTime2.GetAsLocalizedTime(TIME_FORMAT(64)) << std::endl;
  // std::cout << "128: " << dateTime2.GetAsLocalizedTime(TIME_FORMAT(128)) << std::endl;
  // std::cout << "256: " << dateTime2.GetAsLocalizedTime(TIME_FORMAT(256)) << std::endl;

  EXPECT_EQ(dateTime2.GetAsLocalizedDate(false), "2020-2-3");
  EXPECT_EQ(dateTime2.GetAsLocalizedDate(true), "Monday, February 3, 2020");
  EXPECT_EQ(dateTime2.GetAsLocalizedDate(std::string("dd-mm-yyyy")),
            "03-02-2020"); // need to force overload function
  EXPECT_EQ(dateTime2.GetAsLocalizedTime("hh-mm-ss", true), "04-05-06");
  EXPECT_EQ(dateTime2.GetAsLocalizedTime("hh-mm-ss", false), "04-05");
  EXPECT_EQ(dateTime2.GetAsLocalizedDateTime(false, false), "2020-2-3 4:5");
  EXPECT_EQ(dateTime2.GetAsLocalizedDateTime(true, true), "Monday, February 3, 2020 4:5:6");

  EXPECT_EQ(dateTime2.GetAsLocalizedTime(TIME_FORMAT(0), false), "4:5");
  EXPECT_EQ(dateTime2.GetAsLocalizedTime(TIME_FORMAT(0), true), "4:5:6");

  EXPECT_EQ(dateTime2.GetAsLocalizedTime(TIME_FORMAT(1)), "06");
  EXPECT_EQ(dateTime2.GetAsLocalizedTime(TIME_FORMAT(2)), "05");
  EXPECT_EQ(dateTime2.GetAsLocalizedTime(TIME_FORMAT(3)), "05:06");
  EXPECT_EQ(dateTime2.GetAsLocalizedTime(TIME_FORMAT(4)), "04");
  EXPECT_EQ(dateTime2.GetAsLocalizedTime(TIME_FORMAT(5)), "04:06");
  EXPECT_EQ(dateTime2.GetAsLocalizedTime(TIME_FORMAT(6)), "04:05");
  EXPECT_EQ(dateTime2.GetAsLocalizedTime(TIME_FORMAT(7)), "04:05:06");
  EXPECT_EQ(dateTime2.GetAsLocalizedTime(TIME_FORMAT(8)), "");
  EXPECT_EQ(dateTime2.GetAsLocalizedTime(TIME_FORMAT(14)), "04:05");
  EXPECT_EQ(dateTime2.GetAsLocalizedTime(TIME_FORMAT(15)), "04:05:06");
  EXPECT_EQ(dateTime2.GetAsLocalizedTime(TIME_FORMAT(16)), "4");
  EXPECT_EQ(dateTime2.GetAsLocalizedTime(TIME_FORMAT(19)), "4:05:06");
  EXPECT_EQ(dateTime2.GetAsLocalizedTime(TIME_FORMAT(27)), "4:05:06 AM");

  // not possible to use these three
  // EXPECT_EQ(dateTime2.GetAsLocalizedTime(TIME_FORMAT(32)), "");
  // EXPECT_EQ(dateTime2.GetAsLocalizedTime(TIME_FORMAT(64)), "");
  // EXPECT_EQ(dateTime2.GetAsLocalizedTime(TIME_FORMAT(128)), "");

  EXPECT_EQ(dateTime2.GetAsLocalizedTime(TIME_FORMAT(256)), "5");
}

TEST_F(TestDateTime, GetAsTimePoint)
{
  CDateTime dateTime;
  dateTime.SetDateTime(1991, 05, 14, 12, 34, 56);

  auto tp = dateTime.GetAsTimePoint();

  EXPECT_TRUE(dateTime == tp);
}

TEST_F(TestDateTime, GetAsTime)
{
  CDateTime dateTime;
  dateTime.SetDateTime(1991, 05, 14, 12, 34, 56);

  time_t time;
  dateTime.GetAsTime(time);

  EXPECT_TRUE(dateTime == time);
}

TEST_F(TestDateTime, GetAsTm)
{
  CDateTime dateTime;
  dateTime.SetDateTime(1991, 05, 14, 12, 34, 56);

  tm time;
  dateTime.GetAsTm(time);

  EXPECT_TRUE(dateTime == time);
}

TEST_F(TestDateTime, GetAsLocalDateTime)
{
  CDateTime dateTime1;
  dateTime1.SetDateTime(1991, 05, 14, 12, 34, 56);

  CDateTime dateTime2;
  dateTime2 = dateTime1.GetAsLocalDateTime();

  auto zoned_time = date::make_zoned(date::current_zone(), dateTime1.GetAsTimePoint());
  auto time = zoned_time.get_local_time().time_since_epoch();

  CDateTime cmpTime(std::chrono::duration_cast<std::chrono::seconds>(time).count());

  EXPECT_TRUE(dateTime1 == cmpTime);
}

TEST_F(TestDateTime, Reset)
{
  CDateTime dateTime;
  dateTime.SetDateTime(1991, 05, 14, 12, 34, 56);

  dateTime.Reset();

  EXPECT_EQ(dateTime.GetYear(), 1970);
  EXPECT_EQ(dateTime.GetMonth(), 1);
  EXPECT_EQ(dateTime.GetDay(), 1);
  EXPECT_EQ(dateTime.GetHour(), 0);
  EXPECT_EQ(dateTime.GetMinute(), 0);
  EXPECT_EQ(dateTime.GetSecond(), 0);
}

TEST_F(TestDateTime, Tzdata)
{
  CDateTime dateTime;
  dateTime.SetDateTime(1991, 05, 14, 12, 34, 56);

  // LANG=C TZ="Etc/GMT+1" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  auto tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  auto zone = date::make_zoned("Etc/GMT+1", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T11:34:56-01:00") << "tzdata information not valid for 'Etc/GMT+1'";

  // LANG=C TZ="Etc/GMT+2" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT+2", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T10:34:56-02:00") << "tzdata information not valid for 'Etc/GMT+2'";

  // LANG=C TZ="Etc/GMT+3" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT+3", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T09:34:56-03:00") << "tzdata information not valid for 'Etc/GMT+3'";

  // LANG=C TZ="Etc/GMT+4" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT+4", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T08:34:56-04:00") << "tzdata information not valid for 'Etc/GMT+4'";

  // LANG=C TZ="Etc/GMT+5" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT+5", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T07:34:56-05:00") << "tzdata information not valid for 'Etc/GMT+5'";

  // LANG=C TZ="Etc/GMT+6" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT+6", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T06:34:56-06:00") << "tzdata information not valid for 'Etc/GMT+6'";

  // LANG=C TZ="Etc/GMT+7" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT+7", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T05:34:56-07:00") << "tzdata information not valid for 'Etc/GMT+7'";

  // LANG=C TZ="Etc/GMT+8" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT+8", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T04:34:56-08:00") << "tzdata information not valid for 'Etc/GMT+8'";

  // LANG=C TZ="Etc/GMT+9" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT+9", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T03:34:56-09:00") << "tzdata information not valid for 'Etc/GMT+9'";

  // LANG=C TZ="Etc/GMT+10" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT+10", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T02:34:56-10:00") << "tzdata information not valid for 'Etc/GMT+10'";

  // LANG=C TZ="Etc/GMT+11" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT+11", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T01:34:56-11:00") << "tzdata information not valid for 'Etc/GMT+11'";

  // LANG=C TZ="Etc/GMT+12" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT+12", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T00:34:56-12:00") << "tzdata information not valid for 'Etc/GMT+12'";

  // LANG=C TZ="Etc/GMT-1" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT-1", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T13:34:56+01:00") << "tzdata information not valid for 'Etc/GMT-1'";

  // LANG=C TZ="Etc/GMT-2" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT-2", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T14:34:56+02:00") << "tzdata information not valid for 'Etc/GMT-2'";

  // LANG=C TZ="Etc/GMT-3" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT-3", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T15:34:56+03:00") << "tzdata information not valid for 'Etc/GMT-3'";

  // LANG=C TZ="Etc/GMT-4" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT-4", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T16:34:56+04:00") << "tzdata information not valid for 'Etc/GMT-4'";

  // LANG=C TZ="Etc/GMT-5" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT-5", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T17:34:56+05:00") << "tzdata information not valid for 'Etc/GMT-5'";

  // LANG=C TZ="Etc/GMT-6" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT-6", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T18:34:56+06:00") << "tzdata information not valid for 'Etc/GMT-6'";

  // LANG=C TZ="Etc/GMT-7" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT-7", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T19:34:56+07:00") << "tzdata information not valid for 'Etc/GMT-7'";

  // LANG=C TZ="Etc/GMT-8" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT-8", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T20:34:56+08:00") << "tzdata information not valid for 'Etc/GMT-8'";

  // LANG=C TZ="Etc/GMT-9" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT-9", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T21:34:56+09:00") << "tzdata information not valid for 'Etc/GMT-9'";

  // LANG=C TZ="Etc/GMT-10" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT-10", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T22:34:56+10:00") << "tzdata information not valid for 'Etc/GMT-10'";

  // LANG=C TZ="Etc/GMT-11" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT-11", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-14T23:34:56+11:00") << "tzdata information not valid for 'Etc/GMT-11'";

  // LANG=C TZ="Etc/GMT-12" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT-12", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-15T00:34:56+12:00") << "tzdata information not valid for 'Etc/GMT-12'";

  // LANG=C TZ="Etc/GMT-13" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT-13", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-15T01:34:56+13:00") << "tzdata information not valid for 'Etc/GMT-13'";

  // LANG=C TZ="Etc/GMT-14" date '+%Y-%m-%dT%H:%M:%S%Ez' -d "1991-05-14 12:34:56 UTC" | sed 's/[0-9][0-9]$/:&/'
  tps = date::floor<std::chrono::seconds>(dateTime.GetAsTimePoint());
  zone = date::make_zoned("Etc/GMT-14", tps);
  EXPECT_EQ(date::format("%FT%T%Ez", zone), "1991-05-15T02:34:56+14:00") << "tzdata information not valid for 'Etc/GMT-14'";
}

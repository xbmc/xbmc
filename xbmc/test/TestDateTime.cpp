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
#include "interfaces/legacy/ModuleXbmc.h" //Needed to test getRegion()

#include <array>
#include <iostream>

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

TEST_F(TestDateTime, FileTimeOperators)
{
  CDateTime dateTime1(1991, 5, 14, 12, 34, 56);
  CDateTime dateTime2(1991, 5, 14, 12, 34, 57);

  KODI::TIME::FileTime fileTime1;
  KODI::TIME::FileTime fileTime2;

  dateTime1.GetAsTimeStamp(fileTime1);
  dateTime2.GetAsTimeStamp(fileTime2);

  CDateTime dateTime3(fileTime1);

  EXPECT_TRUE(dateTime3 < fileTime2);
  EXPECT_FALSE(dateTime3 > fileTime2);
  EXPECT_FALSE(dateTime3 == fileTime2);
}

TEST_F(TestDateTime, SystemTimeOperators)
{
  CDateTime dateTime1(1991, 5, 14, 12, 34, 56);
  CDateTime dateTime2(1991, 5, 14, 12, 34, 57);

  KODI::TIME::SystemTime systemTime;
  dateTime2.GetAsSystemTime(systemTime);

  EXPECT_TRUE(dateTime1 < systemTime);
  EXPECT_FALSE(dateTime1 > systemTime);
  EXPECT_FALSE(dateTime1 == systemTime);
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
  {
    CDateTime dateTime1(1991, 5, 14, 12, 34, 56);

    tm t1;
    dateTime1.GetAsTm(t1);

    EXPECT_FALSE(dateTime1 < t1);
    EXPECT_FALSE(dateTime1 > t1);
    EXPECT_TRUE(dateTime1 == t1);

    CDateTime dateTime2(1991, 5, 14, 12, 34, 57);

    tm t2;
    dateTime2.GetAsTm(t2);

    EXPECT_TRUE(dateTime1 < t2);
    EXPECT_FALSE(dateTime1 > t2);
    EXPECT_FALSE(dateTime1 == t2);
  }

  // same test but opposite daylight saving
  {
    CDateTime dateTime1(1991, 1, 14, 12, 34, 56);

    tm t1;
    dateTime1.GetAsTm(t1);

    EXPECT_FALSE(dateTime1 < t1);
    EXPECT_FALSE(dateTime1 > t1);
    EXPECT_TRUE(dateTime1 == t1);

    CDateTime dateTime2(1991, 1, 14, 12, 34, 57);

    tm t2;
    dateTime2.GetAsTm(t2);

    EXPECT_TRUE(dateTime1 < t2);
    EXPECT_FALSE(dateTime1 > t2);
    EXPECT_FALSE(dateTime1 == t2);
  }
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
TEST_F(TestDateTime, DISABLED_SetFromDateString)
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

// disabled on osx and freebsd as their mktime functions
// don't work for dates before 1900
#if defined(TARGET_DARWIN_OSX) || defined(TARGET_FREEBSD)
TEST_F(TestDateTime, DISABLED_SetFromDBTime)
#else
TEST_F(TestDateTime, SetFromDBTime)
#endif
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
  CDateTimeSpan bias = CDateTime::GetTimezoneBias();
  CDateTime dateTime;
  dateTime.SetFromDBDateTime("1994-11-05 13:15:30");
  dateTime += bias;
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
  CDateTimeSpan bias = CDateTime::GetTimezoneBias();

  CDateTime dateTime1;
  dateTime1.SetFromDBDateTime("1991-05-14 12:34:56");
  dateTime1 += bias;

  CDateTime dateTime2;
  EXPECT_TRUE(dateTime2.SetFromUTCDateTime(dateTime1));
  EXPECT_EQ(dateTime2.GetYear(), 1991);
  EXPECT_EQ(dateTime2.GetMonth(), 5);
  EXPECT_EQ(dateTime2.GetDay(), 14);
  EXPECT_EQ(dateTime2.GetHour(), 12);
  EXPECT_EQ(dateTime2.GetMinute(), 34);
  EXPECT_EQ(dateTime2.GetSecond(), 56);

  const time_t time = 674224496 + bias.GetSecondsTotal();

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

// disabled on osx and freebsd as their mktime functions
// don't work for dates before 1900
#if !defined(TARGET_DARWIN_OSX) && !defined(TARGET_FREEBSD)
  CDateTime dateTime3;
  EXPECT_TRUE(dateTime3.SetTime(12, 34, 56));
  EXPECT_EQ(dateTime3.GetYear(), 1601);
  EXPECT_EQ(dateTime3.GetMonth(), 1);
  EXPECT_EQ(dateTime3.GetDay(), 1);
  EXPECT_EQ(dateTime3.GetHour(), 12);
  EXPECT_EQ(dateTime3.GetMinute(), 34);
  EXPECT_EQ(dateTime3.GetSecond(), 56);
#endif
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

// disabled because we have no way to validate these values
// GetTimezoneBias() always returns a positive value so
// there is no way to detect the direction of the offset
TEST_F(TestDateTime, DISABLED_GetAsStringsWithBias)
{
  CDateTimeSpan bias = CDateTime::GetTimezoneBias();

  CDateTime dateTime;
  dateTime.SetDateTime(1991, 05, 14, 12, 34, 56);

  CDateTime dateTimeWithBias(dateTime);
  dateTimeWithBias += bias;

  EXPECT_EQ(dateTime.GetAsRFC1123DateTime(), "Tue, 14 May 1991 20:34:56 GMT");
  EXPECT_EQ(dateTime.GetAsW3CDateTime(false), "1991-05-14T12:34:56+08:00");
  EXPECT_EQ(dateTime.GetAsW3CDateTime(true), "1991-05-14T20:34:56Z");
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

  //Test abbreviated month and short year formats
  CDateTime dateTime3;
  dateTime3.SetDateTime(1991, 05, 9, 12, 34, 56); //Need a single digit date

  g_langInfo.SetShortDateFormat("DD-mmm-YY");
  g_langInfo.SetLongDateFormat("ddd, D MMMM YYYY");

  //Actual formatted date
  //Test short month name and 2 digit year.
  EXPECT_EQ(dateTime3.GetAsLocalizedDate(false), "09-May-91");
  //Test short day name and single digit day number.
  EXPECT_EQ(dateTime3.GetAsLocalizedDate(true), "Thu, 9 May 1991");

  //Test that the Python date formatting string is returned instead
  //of the actual formatted date string.

  CDateTime dateTime4;
  dateTime4.SetDateTime(1991, 05, 9, 12, 34, 56); //Need a single digit date

  //Test non zero-padded day and short month name.
  EXPECT_EQ(
      dateTime4.GetAsLocalizedDate(std::string("D-mmm-YY"), CDateTime::ReturnFormat::CHOICE_YES),
      "%-d-%b-%y");
  //Test NZP day and NZP month.
  EXPECT_EQ(
      dateTime4.GetAsLocalizedDate(std::string("D-M-YY"), CDateTime::ReturnFormat::CHOICE_YES),
      "%-d-%-m-%y");

  g_langInfo.SetShortDateFormat("D/M/YY");
  g_langInfo.SetLongDateFormat("ddd, D MMMM YYYY");

  //Test getRegion() here because it is directly reliant on GetAsLocalizedDate()
  //and the windows-specific formatting happens in getRegion().
#ifdef TARGET_WINDOWS
  //Windows is handled differently because that's what ModuleXbmc.cpp does.
  EXPECT_EQ(XBMCAddon::xbmc::getRegion("dateshort"), "%#d/%#m/%y");
  EXPECT_EQ(XBMCAddon::xbmc::getRegion("datelong"), "%a, %#d %B %Y");
#else
  EXPECT_EQ(XBMCAddon::xbmc::getRegion("dateshort"), "%-d/%-m/%y");
  EXPECT_EQ(XBMCAddon::xbmc::getRegion("datelong"), "%a, %-d %B %Y");
#endif

  //Test short day name, short month name and 2 digit year.
  EXPECT_EQ(dateTime4.GetAsLocalizedDate(std::string("ddd, DD-mmm-YY"),
                                         CDateTime::ReturnFormat::CHOICE_YES),
            "%a, %d-%b-%y");
  //Test as above but with 4 digit year.
  EXPECT_EQ(dateTime4.GetAsLocalizedDate(std::string("ddd, DD-mmm-YYYY"),
                                         CDateTime::ReturnFormat::CHOICE_YES),
            "%a, %d-%b-%Y");
  //Test some 'normal' DMY.
  EXPECT_EQ(
      dateTime4.GetAsLocalizedDate(std::string("DD/MM/YYYY"), CDateTime::ReturnFormat::CHOICE_YES),
      "%d/%m/%Y");
  EXPECT_EQ(
      dateTime4.GetAsLocalizedDate(std::string("DD/MM/YY"), CDateTime::ReturnFormat::CHOICE_YES),
      "%d/%m/%y");

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

TEST_F(TestDateTime, GetAsSystemTime)
{
  CDateTime dateTime;
  dateTime.SetDateTime(1991, 05, 14, 12, 34, 56);

  KODI::TIME::SystemTime systemTime;
  dateTime.GetAsSystemTime(systemTime);

  EXPECT_TRUE(dateTime == systemTime);
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
  {
    CDateTime dateTime;
    dateTime.SetDateTime(1991, 05, 14, 12, 34, 56);

    tm time;
    dateTime.GetAsTm(time);
    EXPECT_TRUE(dateTime == time);
  }

  // same test but opposite daylight saving
  {
    CDateTime dateTime;
    dateTime.SetDateTime(1991, 01, 14, 12, 34, 56);

    tm time;
    dateTime.GetAsTm(time);
    EXPECT_TRUE(dateTime == time);
  }
}

// Disabled pending std::chrono and std::date changes.
TEST_F(TestDateTime, DISABLED_GetAsTimeStamp)
{
  CDateTimeSpan bias = CDateTime::GetTimezoneBias();

  CDateTime dateTime;
  dateTime.SetDateTime(1991, 05, 14, 12, 34, 56);

  KODI::TIME::FileTime fileTime;
  dateTime.GetAsTimeStamp(fileTime);
  dateTime += bias;

  EXPECT_TRUE(dateTime == fileTime);
}

TEST_F(TestDateTime, GetAsUTCDateTime)
{
  CDateTimeSpan bias = CDateTime::GetTimezoneBias();

  CDateTime dateTime1;
  dateTime1.SetDateTime(1991, 05, 14, 12, 34, 56);

  CDateTime dateTime2;
  dateTime2 = dateTime1.GetAsUTCDateTime();
  dateTime2 -= bias;

  EXPECT_EQ(dateTime2.GetYear(), 1991);
  EXPECT_EQ(dateTime2.GetMonth(), 5);
  EXPECT_EQ(dateTime2.GetDay(), 14);
  EXPECT_EQ(dateTime2.GetHour(), 12);
  EXPECT_EQ(dateTime2.GetMinute(), 34);
  EXPECT_EQ(dateTime2.GetSecond(), 56);
}

// disabled on osx and freebsd as their mktime functions
// don't work for dates before 1900
#if defined(TARGET_DARWIN_OSX) || defined(TARGET_FREEBSD)
TEST_F(TestDateTime, DISABLED_Reset)
#else
TEST_F(TestDateTime, Reset)
#endif
{
  CDateTime dateTime;
  dateTime.SetDateTime(1991, 05, 14, 12, 34, 56);

  dateTime.Reset();

  EXPECT_EQ(dateTime.GetYear(), 1601);
  EXPECT_EQ(dateTime.GetMonth(), 1);
  EXPECT_EQ(dateTime.GetDay(), 1);
  EXPECT_EQ(dateTime.GetHour(), 0);
  EXPECT_EQ(dateTime.GetMinute(), 0);
  EXPECT_EQ(dateTime.GetSecond(), 0);
}

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#ifndef TEST_UTILS_TIMEUTILS_H_INCLUDED
#define TEST_UTILS_TIMEUTILS_H_INCLUDED
#include "utils/TimeUtils.h"
#endif

#ifndef TEST_XBDATETIME_H_INCLUDED
#define TEST_XBDATETIME_H_INCLUDED
#include "XBDateTime.h"
#endif


#ifndef TEST_GTEST_GTEST_H_INCLUDED
#define TEST_GTEST_GTEST_H_INCLUDED
#include "gtest/gtest.h"
#endif


TEST(TestTimeUtils, CurrentHostCounter)
{
  std::cout << "CurrentHostCounter(): " <<
    testing::PrintToString(CurrentHostCounter()) << std::endl;
}

TEST(TestTimeUtils, CurrentHostFrequency)
{
  std::cout << "CurrentHostFrequency(): " <<
    testing::PrintToString(CurrentHostFrequency()) << std::endl;
}

TEST(TestTimeUtils, GetFrameTime)
{
  std::cout << "GetFrameTime(): " <<
    testing::PrintToString(CTimeUtils::GetFrameTime()) << std::endl;

  std::cout << "Calling UpdateFrameTime()" << std::endl;
  CTimeUtils::UpdateFrameTime(true);
  std::cout << "GetFrameTime(): " <<
    testing::PrintToString(CTimeUtils::GetFrameTime()) << std::endl;
}

TEST(TestTimeUtils, GetLocalTime)
{
  CDateTime cdatetime, cdatetime2;
  time_t timer;

  time(&timer);

  cdatetime = CTimeUtils::GetLocalTime(timer);
  std::cout << "cdatetime.GetAsLocalizedDateTime(): " <<
    cdatetime.GetAsLocalizedDateTime() << std::endl;

  cdatetime2 = timer;
  std::cout << "time: " <<
    cdatetime2.GetAsLocalizedDateTime() << std::endl;
}

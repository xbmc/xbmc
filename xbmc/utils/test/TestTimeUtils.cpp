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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "utils/TimeUtils.h"
#include "XBDateTime.h"

#include "gtest/gtest.h"

TEST(TestTimeUtils, CurrentHostCounter)
{
  std::cout << "CurrentHostCounter(): " <<
    testing::PrintToString(CurrentHostCounter()) << "\n";
}

TEST(TestTimeUtils, CurrentHostFrequency)
{
  std::cout << "CurrentHostFrequency(): " <<
    testing::PrintToString(CurrentHostFrequency()) << "\n";
}

TEST(TestTimeUtils, GetFrameTime)
{
  std::cout << "GetFrameTime(): " <<
    testing::PrintToString(CTimeUtils::GetFrameTime()) << "\n";

  std::cout << "Calling UpdateFrameTime()\n";
  CTimeUtils::UpdateFrameTime(true);
  std::cout << "GetFrameTime(): " <<
    testing::PrintToString(CTimeUtils::GetFrameTime()) << "\n";
}

TEST(TestTimeUtils, GetLocalTime)
{
  CDateTime cdatetime, cdatetime2;
  time_t time;

  cdatetime = CTimeUtils::GetLocalTime(time);
  std::cout << "cdatetime.GetAsLocalizedDateTime(): " <<
    cdatetime.GetAsLocalizedDateTime() << "\n";

  cdatetime2 = time;
  std::cout << "time: " <<
    cdatetime2.GetAsLocalizedDateTime() << "\n";
}

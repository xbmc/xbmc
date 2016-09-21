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

#include "utils/Stopwatch.h"
#include "threads/Thread.h"

#include "gtest/gtest.h"

class CTestStopWatchThread : public CThread
{
public:
  CTestStopWatchThread() :
    CThread("TestStopWatch"){}
};

TEST(TestStopWatch, Initialization)
{
  CStopWatch a;
  EXPECT_FALSE(a.IsRunning());
  EXPECT_EQ(0.0f, a.GetElapsedSeconds());
  EXPECT_EQ(0.0f, a.GetElapsedMilliseconds());
}

TEST(TestStopWatch, Start)
{
  CStopWatch a;
  a.Start();
  EXPECT_TRUE(a.IsRunning());
}

TEST(TestStopWatch, Stop)
{
  CStopWatch a;
  a.Start();
  a.Stop();
  EXPECT_FALSE(a.IsRunning());
}

TEST(TestStopWatch, ElapsedTime)
{
  CStopWatch a;
  CTestStopWatchThread thread;
  a.Start();
  thread.Sleep(1);
  EXPECT_GT(a.GetElapsedSeconds(), 0.0f);
  EXPECT_GT(a.GetElapsedMilliseconds(), 0.0f);
}

TEST(TestStopWatch, Reset)
{
  CStopWatch a;
  CTestStopWatchThread thread;
  a.StartZero();
  thread.Sleep(2);
  EXPECT_GT(a.GetElapsedMilliseconds(), 1);
  thread.Sleep(3);
  a.Reset();
  EXPECT_LT(a.GetElapsedMilliseconds(), 5);
}

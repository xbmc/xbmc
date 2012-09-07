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

#include "utils/Stopwatch.h"
#include "threads/Thread.h"

#include "gtest/gtest.h"

class CTestStopWatchThread : public CThread
{
public:
  CTestStopWatchThread() :
    CThread("CTestStopWatchThread"){}
};

TEST(TestStopWatch, Start)
{
  CStopWatch a;
  CTestStopWatchThread thread;

  EXPECT_FALSE(a.IsRunning());
  EXPECT_EQ(0.0f, a.GetElapsedSeconds());
  EXPECT_EQ(0.0f, a.GetElapsedMilliseconds());

  std::cout << "Calling Start()" << std::endl;
  a.Start();
  thread.Sleep(1000);
  EXPECT_TRUE(a.IsRunning());
  std::cout << "Elapsed Seconds: " << a.GetElapsedSeconds() << std::endl;
  std::cout << "Elapsed Milliseconds: " << a.GetElapsedMilliseconds() << std::endl;
}

TEST(TestStopWatch, Stop)
{
  CStopWatch a;
  CTestStopWatchThread thread;

  EXPECT_FALSE(a.IsRunning());
  EXPECT_EQ(0.0f, a.GetElapsedSeconds());
  EXPECT_EQ(0.0f, a.GetElapsedMilliseconds());

  std::cout << "Calling Start()" << std::endl;
  a.Start();
  thread.Sleep(1000);
  EXPECT_TRUE(a.IsRunning());
  std::cout << "Elapsed Seconds: " << a.GetElapsedSeconds() << std::endl;
  std::cout << "Elapsed Milliseconds: " << a.GetElapsedMilliseconds() << std::endl;

  a.Stop();
  EXPECT_FALSE(a.IsRunning());
  EXPECT_EQ(0.0f, a.GetElapsedSeconds());
  EXPECT_EQ(0.0f, a.GetElapsedMilliseconds());
}

TEST(TestStopWatch, StartZero)
{
  CStopWatch a;
  CTestStopWatchThread thread;

  EXPECT_FALSE(a.IsRunning());
  EXPECT_EQ(0.0f, a.GetElapsedSeconds());
  EXPECT_EQ(0.0f, a.GetElapsedMilliseconds());

  std::cout << "Calling StartZero()" << std::endl;
  a.StartZero();
  thread.Sleep(1000);
  EXPECT_TRUE(a.IsRunning());
  std::cout << "Elapsed Seconds: " << a.GetElapsedSeconds() << std::endl;
  std::cout << "Elapsed Milliseconds: " << a.GetElapsedMilliseconds() << std::endl;

  std::cout << "Calling StartZero()" << std::endl;
  a.StartZero();
  thread.Sleep(1000);
  EXPECT_TRUE(a.IsRunning());
  std::cout << "Elapsed Seconds: " << a.GetElapsedSeconds() << std::endl;
  std::cout << "Elapsed Milliseconds: " << a.GetElapsedMilliseconds() << std::endl;
}

TEST(TestStopWatch, Reset)
{
  CStopWatch a;
  CTestStopWatchThread thread;

  EXPECT_FALSE(a.IsRunning());
  EXPECT_EQ(0.0f, a.GetElapsedSeconds());
  EXPECT_EQ(0.0f, a.GetElapsedMilliseconds());

  std::cout << "Calling StartZero()" << std::endl;
  a.StartZero();
  thread.Sleep(1000);
  EXPECT_TRUE(a.IsRunning());
  std::cout << "Elapsed Seconds: " << a.GetElapsedSeconds() << std::endl;
  std::cout << "Elapsed Milliseconds: " << a.GetElapsedMilliseconds() << std::endl;

  std::cout << "Calling Reset()" << std::endl;
  a.Reset();
  thread.Sleep(1000);
  EXPECT_TRUE(a.IsRunning());
  std::cout << "Elapsed Seconds: " << a.GetElapsedSeconds() << std::endl;
  std::cout << "Elapsed Milliseconds: " << a.GetElapsedMilliseconds() << std::endl;
}

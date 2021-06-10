/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "threads/Thread.h"
#include "utils/Stopwatch.h"

#include <gtest/gtest.h>

using namespace std::chrono_literals;

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
  thread.Sleep(1ms);
  EXPECT_GT(a.GetElapsedSeconds(), 0.0f);
  EXPECT_GT(a.GetElapsedMilliseconds(), 0.0f);
}

TEST(TestStopWatch, Reset)
{
  CStopWatch a;
  CTestStopWatchThread thread;
  a.StartZero();
  thread.Sleep(2ms);
  EXPECT_GT(a.GetElapsedMilliseconds(), 1);
  thread.Sleep(3ms);
  a.Reset();
  EXPECT_LT(a.GetElapsedMilliseconds(), 5);
}

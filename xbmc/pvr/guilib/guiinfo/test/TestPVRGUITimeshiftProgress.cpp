/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "pvr/guilib/guiinfo/PVRGUITimeshiftProgress.h"

#include <limits>

#include <gtest/gtest.h>

using PVR::TimeshiftProgress;

TEST(TestPVRGUITimeshiftProgress, BufferOnlyWithoutUTC)
{
  const TimeshiftProgress progress{0, 3643647, 0, 3643647};
  EXPECT_EQ(progress.GetSeekTime(0), 0);
  EXPECT_EQ(progress.GetSeekTime(50), 1821824);
  EXPECT_EQ(progress.GetSeekTime(100), 3643647);
  EXPECT_NEAR(progress.GetPercentage(3632046), 99.68161, 0.00001);
}

TEST(TestPVRGUITimeshiftProgress, NonzeroBufferOrigin)
{
  const TimeshiftProgress progress{123456, 3723456, 123456, 3723456};
  EXPECT_EQ(progress.GetSeekTime(0), 123456);
  EXPECT_EQ(progress.GetSeekTime(25), 1023456);
  EXPECT_EQ(progress.GetSeekTime(100), 3723456);
}

TEST(TestPVRGUITimeshiftProgress, EpgExtendsBothSidesOfBuffer)
{
  const TimeshiftProgress progress{600123, 2400123, 0, 3600000};
  EXPECT_EQ(progress.GetSeekTime(0), 600123);
  EXPECT_EQ(progress.GetSeekTime(50), 1800000);
  EXPECT_EQ(progress.GetSeekTime(100), 2400123);
}

TEST(TestPVRGUITimeshiftProgress, DoesNotUseRoundedBufferPercentage)
{
  const TimeshiftProgress progress{612345, 4200000, 0, 7200000};
  EXPECT_NEAR(progress.GetPercentage(progress.bufferStart), 8.5047916667, 0.000001);
  EXPECT_EQ(progress.GetSeekTime(50), 3600000);
}

TEST(TestPVRGUITimeshiftProgress, SimpleEpgTimelineInsideBuffer)
{
  const TimeshiftProgress progress{100000, 7300000, 1900000, 5500000};
  EXPECT_EQ(progress.GetSeekTime(0), 1900000);
  EXPECT_EQ(progress.GetSeekTime(50), 3700000);
  EXPECT_EQ(progress.GetSeekTime(100), 5500000);
}

TEST(TestPVRGUITimeshiftProgress, MillisecondBufferShorterThanOneSecond)
{
  const TimeshiftProgress progress{1250, 1750, 1250, 1750};
  EXPECT_EQ(progress.GetSeekTime(50), 1500);
  EXPECT_DOUBLE_EQ(progress.GetPercentage(1500), 50);
}

TEST(TestPVRGUITimeshiftProgress, DisplayAndSeekUseSameAxis)
{
  const TimeshiftProgress progress{123456, 3723456, -500000, 4000000};
  for (int64_t time = progress.bufferStart; time <= progress.bufferEnd; time += 1000)
    EXPECT_EQ(progress.GetSeekTime(progress.GetPercentage(time)), time);
}

TEST(TestPVRGUITimeshiftProgress, RejectsInvalidPercentage)
{
  const TimeshiftProgress progress{0, 3600000, 0, 3600000};
  for (const double percentage : {-1.0, 101.0, std::numeric_limits<double>::infinity(),
                                  std::numeric_limits<double>::quiet_NaN()})
    EXPECT_FALSE(progress.GetSeekTime(percentage));
}

TEST(TestPVRGUITimeshiftProgress, RejectsMissingOrReversedBounds)
{
  EXPECT_FALSE((TimeshiftProgress{0, 0, 0, 3600000}.GetSeekTime(50)));
  EXPECT_FALSE((TimeshiftProgress{100, 0, 0, 3600000}.GetSeekTime(50)));
  EXPECT_FALSE((TimeshiftProgress{0, 3600000, 100, 100}.GetSeekTime(50)));
  EXPECT_FALSE((TimeshiftProgress{0, 3600000, 100, 0}.GetSeekTime(50)));
  EXPECT_DOUBLE_EQ((TimeshiftProgress{}.GetPercentage(0)), 0);
}

/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "threads/Thread.h"
#include "utils/BitstreamStats.h"

#include "gtest/gtest.h"

#define BITS (256 * 8)
#define BYTES (256)

class CTestBitstreamStatsThread : public CThread
{
public:
  CTestBitstreamStatsThread() :
    CThread("TestBitstreamStats"){}

};

TEST(TestBitstreamStats, General)
{
  int i;
  BitstreamStats a;
  CTestBitstreamStatsThread t;

  i = 0;
  a.Start();
  EXPECT_EQ(0.0, a.GetBitrate());
  EXPECT_EQ(0.0, a.GetMaxBitrate());
  EXPECT_EQ(-1.0, a.GetMinBitrate());
  while (i <= BITS)
  {
    a.AddSampleBits(1);
    i++;
    t.Sleep(1);
  }
  a.CalculateBitrate();
  EXPECT_GT(a.GetBitrate(), 0.0);
  EXPECT_GT(a.GetMaxBitrate(), 0.0);
  EXPECT_GT(a.GetMinBitrate(), 0.0);

  i = 0;
  while (i <= BYTES)
  {
    a.AddSampleBytes(1);
    t.Sleep(2);
    i++;
  }
  a.CalculateBitrate();
  EXPECT_GT(a.GetBitrate(), 0.0);
  EXPECT_GT(a.GetMaxBitrate(), 0.0);
  EXPECT_LE(a.GetMinBitrate(), a.GetMaxBitrate());
}

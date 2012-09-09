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

#include "threads/Thread.h"
#include "utils/BitstreamStats.h"

#include "gtest/gtest.h"

#define BITS (256 * 8)
#define BYTES (256)

class CTestBitstreamStatsThread : public CThread
{
public:
  CTestBitstreamStatsThread() :
    CThread("CTestBitstreamStatsThread"){}
  
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

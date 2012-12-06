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

#include "utils/PerformanceSample.h"

#include "gtest/gtest.h"

class MyCPerformanceSample : public CPerformanceSample
{
public:
  MyCPerformanceSample(const std::string &statName, bool bCheckWhenDone = true)
  : CPerformanceSample(statName, bCheckWhenDone)
  {}
  std::string getStatName() { return m_statName; }
  bool getCheckWhenDown() { return m_bCheckWhenDone; }
  int64_t getStart() { return m_tmStart; }
  int64_t getFreq() { return m_tmFreq; }
};

TEST(TestPerformanceSample, General)
{
  MyCPerformanceSample a("test");

  a.Reset();
  a.CheckPoint();

  EXPECT_STREQ("test", a.getStatName().c_str());
  EXPECT_TRUE(a.getCheckWhenDown());
  EXPECT_GT(a.getStart(), (int64_t)0);
  EXPECT_GT(a.getFreq(), (int64_t)0);

  std::cout << "Estimated Error: " <<
    testing::PrintToString(a.GetEstimatedError()) << std::endl;
  std::cout << "Start: " << testing::PrintToString(a.getStart()) << std::endl;
  std::cout << "Frequency: " << testing::PrintToString(a.getFreq()) << std::endl;
}

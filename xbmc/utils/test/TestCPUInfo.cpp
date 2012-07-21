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

#include "utils/CPUInfo.h"

#include "gtest/gtest.h"

TEST(TestCPUInfo, getUsedPercentage)
{
  EXPECT_GE(g_cpuInfo.getUsedPercentage(), 0);
}

TEST(TestCPUInfo, getCPUCount)
{
  EXPECT_GT(g_cpuInfo.getCPUCount(), 0);
}

TEST(TestCPUInfo, getCPUFrequency)
{
  EXPECT_GE(g_cpuInfo.getCPUFrequency(), 0.f);
}

TEST(TestCPUInfo, getTemperature)
{
  CTemperature t = g_cpuInfo.getTemperature();
  EXPECT_TRUE(t.IsValid());
}

TEST(TestCPUInfo, getCPUModel)
{
  std::string s = g_cpuInfo.getCPUModel();
  EXPECT_STRNE("", s.c_str());
}

TEST(TestCPUInfo, CoreInfo)
{
  ASSERT_TRUE(g_cpuInfo.HasCoreId(0));
  const CoreInfo c = g_cpuInfo.GetCoreInfo(0);
  EXPECT_FALSE(c.m_strModel.empty());
}

TEST(TestCPUInfo, GetCoresUsageString)
{
  EXPECT_STRNE("", g_cpuInfo.GetCoresUsageString());
}

TEST(TestCPUInfo, GetCPUFeatures)
{
  unsigned int a = g_cpuInfo.GetCPUFeatures();
  (void)a;
}

TEST(TestCPUInfo, getUsedPercentage_output)
{
  CCPUInfo c;
  Sleep(1); /* TODO: Support option from main that sets this parameter */
  int r = c.getUsedPercentage();
  std::cout << "Percentage: " << testing::PrintToString(r) << "\n";
}

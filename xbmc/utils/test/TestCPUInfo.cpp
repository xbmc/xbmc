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

#include "utils/CPUInfo.h"
#include "utils/Temperature.h"
#include "settings/AdvancedSettings.h"

#ifdef TARGET_POSIX
#include "../linux/XTimeUtils.h"
#endif

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

namespace
{
class TemporarySetting
{
public:

  TemporarySetting(std::string &setting, const char *newValue) :
    m_Setting(setting),
    m_OldValue(setting)
  {
    m_Setting = newValue;
  }

  ~TemporarySetting()
  {
    m_Setting = m_OldValue;
  }

private:

  std::string &m_Setting;
  std::string m_OldValue;
};
}

TEST(TestCPUInfo, getTemperature)
{
  TemporarySetting command(g_advancedSettings.m_cpuTempCmd, "echo '50 c'");
  CTemperature t;
  EXPECT_TRUE(g_cpuInfo.getTemperature(t));
  EXPECT_TRUE(t.IsValid());
}

TEST(TestCPUInfo, getCPUModel)
{
  std::string s = g_cpuInfo.getCPUModel();
  EXPECT_STRNE("", s.c_str());
}

TEST(TestCPUInfo, getCPUBogoMips)
{
  std::string s = g_cpuInfo.getCPUBogoMips();
  EXPECT_STRNE("", s.c_str());
}

TEST(TestCPUInfo, getCPUHardware)
{
  std::string s = g_cpuInfo.getCPUHardware();
  EXPECT_STRNE("", s.c_str());
}

TEST(TestCPUInfo, getCPURevision)
{
  std::string s = g_cpuInfo.getCPURevision();
  EXPECT_STRNE("", s.c_str());
}

TEST(TestCPUInfo, getCPUSerial)
{
  std::string s = g_cpuInfo.getCPUSerial();
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
  EXPECT_STRNE("", g_cpuInfo.GetCoresUsageString().c_str());
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
  std::cout << "Percentage: " << testing::PrintToString(r) << std::endl;
}

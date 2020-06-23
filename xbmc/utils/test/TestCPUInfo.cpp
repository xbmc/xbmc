/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#if defined(TARGET_WINDOWS)
#  include <windows.h>
#endif

#include "utils/CPUInfo.h"
#include "utils/Temperature.h"
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"

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

//Disabled for windows because there is no implementation to get the CPU temp and there will probably never be one
#ifndef TARGET_WINDOWS
TEST(TestCPUInfo, getTemperature)
{
  TemporarySetting command(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_cpuTempCmd, "echo '50 c'");
  CTemperature t;
  EXPECT_TRUE(g_cpuInfo.getTemperature(t));
  EXPECT_TRUE(t.IsValid());
}
#endif

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
  Sleep(1); //! @todo Support option from main that sets this parameter
  int r = c.getUsedPercentage();
  std::cout << "Percentage: " << testing::PrintToString(r) << std::endl;
}

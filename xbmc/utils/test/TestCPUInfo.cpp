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

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/CPUInfo.h"
#include "utils/Temperature.h"
#include "utils/XTimeUtils.h"

#include <gtest/gtest.h>

struct TestCPUInfo : public ::testing::Test
{
  TestCPUInfo() { CServiceBroker::RegisterCPUInfo(CCPUInfo::GetCPUInfo()); }

  ~TestCPUInfo() { CServiceBroker::UnregisterCPUInfo(); }
};

TEST_F(TestCPUInfo, GetUsedPercentage)
{
  EXPECT_GE(CServiceBroker::GetCPUInfo()->GetUsedPercentage(), 0);
}

TEST_F(TestCPUInfo, GetCPUCount)
{
  EXPECT_GT(CServiceBroker::GetCPUInfo()->GetCPUCount(), 0);
}

TEST_F(TestCPUInfo, GetCPUFrequency)
{
  EXPECT_GE(CServiceBroker::GetCPUInfo()->GetCPUFrequency(), 0.f);
}

#if defined(TARGET_WINDOWS) or defined(TARGET_DARWIN_OSX)
TEST_F(TestCPUInfo, DISABLED_GetTemperature)
#else
TEST_F(TestCPUInfo, GetTemperature)
#endif
{
  CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_cpuTempCmd = "echo '50 c'";
  CTemperature t;
  EXPECT_TRUE(CServiceBroker::GetCPUInfo()->GetTemperature(t));
  EXPECT_TRUE(t.IsValid());
  EXPECT_EQ(t.ToCelsius(), 50);
}

TEST_F(TestCPUInfo, CoreInfo)
{
  ASSERT_TRUE(CServiceBroker::GetCPUInfo()->HasCoreId(0));
  const CoreInfo c = CServiceBroker::GetCPUInfo()->GetCoreInfo(0);
  EXPECT_TRUE(c.m_id == 0);
}

TEST_F(TestCPUInfo, GetCoresUsageString)
{
  EXPECT_STRNE("", CServiceBroker::GetCPUInfo()->GetCoresUsageString().c_str());
}

TEST_F(TestCPUInfo, GetCPUFeatures)
{
  unsigned int a = CServiceBroker::GetCPUInfo()->GetCPUFeatures();
  (void)a;
}

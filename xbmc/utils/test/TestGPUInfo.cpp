/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/GpuInfo.h"

#include <memory>

#include <gtest/gtest.h>

class TestGPUInfo : public ::testing::Test
{
protected:
  TestGPUInfo() = default;
};

#if defined(TARGET_WINDOWS)
TEST_F(TestGPUInfo, DISABLED_GetTemperatureFromCmd)
#else
TEST_F(TestGPUInfo, GetTemperatureFromCmd)
#endif
{
  CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_gpuTempCmd = "echo '50 c'";
  std::unique_ptr<CGPUInfo> gpuInfo = CGPUInfo::GetGPUInfo();
  EXPECT_NE(gpuInfo, nullptr);
  CTemperature t;
  bool success = gpuInfo->GetTemperature(t);
  EXPECT_TRUE(success);
  EXPECT_TRUE(t.IsValid());
  EXPECT_EQ(t.ToCelsius(), 50);
}

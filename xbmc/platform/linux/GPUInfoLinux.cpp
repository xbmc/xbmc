/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GPUInfoLinux.h"

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/Temperature.h"

#include "platform/linux/SysfsPath.h"

#include <array>
#include <string>
#include <string_view>

namespace
{
constexpr std::array<std::string_view, 4> SENSORS = {
    "amdgpu",
    "radeon",
    "nouveau",
    "gpu_thermal",
};

std::string FindSensor()
{
  const auto settings = CServiceBroker::GetSettingsComponent();
  const auto advancedSettings = settings ? settings->GetAdvancedSettings() : nullptr;

  if (advancedSettings && !advancedSettings->m_gpuTempHwmon.empty())
    return HwmonTemperaturePath(advancedSettings->m_gpuTempHwmon);

  for (const auto& sensor : SENSORS)
  {
    const std::string path{HwmonTemperaturePath(sensor)};
    if (!path.empty())
      return path;
  }

  return {};
}
} // namespace

std::unique_ptr<CGPUInfo> CGPUInfo::GetGPUInfo()
{
  return std::make_unique<CGPUInfoLinux>();
}

const std::string& CGPUInfoLinux::TemperaturePath() const
{
  if (!m_tempPathResolved)
  {
    m_tempPath = FindSensor();
    m_tempPathResolved = true;
  }

  return m_tempPath;
}

bool CGPUInfoLinux::SupportsPlatformTemperature() const
{
  return !TemperaturePath().empty();
}

bool CGPUInfoLinux::GetGPUPlatformTemperature(CTemperature& temperature) const
{
  auto temp = CSysfsPath(TemperaturePath()).Get<double>();
  if (!temp.has_value())
    return false;

  temperature = CTemperature::CreateFromCelsius(*temp / 1000.0);
  temperature.SetValid(true);

  return true;
}

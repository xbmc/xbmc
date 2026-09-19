/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GPUInfoLinux.h"

#include "utils/Set.h"
#include "utils/Temperature.h"

#include "platform/linux/SysfsPath.h"

#include <string>
#include <string_view>

namespace
{
constexpr auto SENSORS = make_set<std::string_view>({
    "amdgpu",
    "radeon",
    "nouveau",
    "gpu_thermal",
});

std::string FindSensor()
{
  for (int i = 0; i < 20; i++)
  {
    CSysfsPath namePath{"/sys/class/hwmon/hwmon" + std::to_string(i) + "/name"};
    if (!namePath.Exists())
      continue;

    auto name = namePath.Get<std::string>();
    if (!name.has_value() || !SENSORS.contains(*name))
      continue;

    std::string tempStr{"/sys/class/hwmon/hwmon" + std::to_string(i) + "/temp1_input"};
    if (CSysfsPath{tempStr}.Exists())
      return tempStr;
  }

  return {};
}
} // namespace

std::unique_ptr<CGPUInfo> CGPUInfo::GetGPUInfo()
{
  return std::make_unique<CGPUInfoLinux>();
}

CGPUInfoLinux::CGPUInfoLinux() : m_tempPath(FindSensor())
{
}

bool CGPUInfoLinux::SupportsPlatformTemperature() const
{
  return !m_tempPath.empty();
}

bool CGPUInfoLinux::GetGPUPlatformTemperature(CTemperature& temperature) const
{
  auto temp = CSysfsPath(m_tempPath).Get<double>();
  if (!temp.has_value())
    return false;

  temperature = CTemperature::CreateFromCelsius(*temp / 1000.0);
  temperature.SetValid(true);

  return true;
}

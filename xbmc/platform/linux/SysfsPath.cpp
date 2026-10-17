/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SysfsPath.h"

#include <exception>

bool CSysfsPath::Exists()
{
  std::ifstream file(m_path);

  if (!file.is_open())
    return false;

  return true;
}

template<>
std::optional<std::string> CSysfsPath::Get()
{
  try
  {
    std::ifstream file(m_path);

    std::string value;

    std::getline(file, value);

    if (file.bad())
    {
      CLog::LogF(LOGERROR, "error reading from '{}'", m_path);
      return std::nullopt;
    }

    return value;
  }
  catch (const std::exception& e)
  {
    CLog::LogF(LOGERROR, "exception reading from '{}': {}", m_path, e.what());
    return std::nullopt;
  }
}

std::string HwmonTemperaturePath(std::string_view sensor)
{
  const std::size_t slash = sensor.find('/');
  const std::string_view name = sensor.substr(0, slash);
  const std::string input{slash == std::string_view::npos ? "temp1_input"
                                                          : sensor.substr(slash + 1)};

  for (int i = 0; i < 20; i++)
  {
    const std::string device{"/sys/class/hwmon/hwmon" + std::to_string(i)};

    CSysfsPath namePath{device + "/name"};
    if (!namePath.Exists())
      continue;

    const auto deviceName = namePath.Get<std::string>();
    if (!deviceName.has_value() || *deviceName != name)
      continue;

    const std::string temperature{device + "/" + input};
    if (CSysfsPath{temperature}.Exists())
      return temperature;
  }

  return {};
}

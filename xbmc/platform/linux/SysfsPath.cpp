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

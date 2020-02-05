/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SysfsPath.h"

bool CSysfsPath::Exists()
{
  std::ifstream file(m_path);

  if (!file.is_open())
    return false;

  return true;
}

template<>
std::string CSysfsPath::Get()
{
  std::ifstream file(m_path);

  std::string value;

  std::getline(file, value);

  if (file.bad())
  {
    CLog::LogF(LOGERROR, "error reading from '{}'", m_path);
    throw std::runtime_error("error reading from " + m_path);
  }

  return value;
}

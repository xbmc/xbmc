/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/log.h"

#include <exception>
#include <fstream>
#include <optional>
#include <string>

class CSysfsPath
{
public:
  CSysfsPath() = default;
  CSysfsPath(const std::string& path) : m_path(path) {}
  ~CSysfsPath() = default;

  bool Exists();

  template<typename T>
  std::optional<T> Get()
  {
    try
    {
      std::ifstream file(m_path);

      T value;

      file >> value;

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

private:
  std::string m_path;
};

template<>
std::optional<std::string> CSysfsPath::Get<std::string>();

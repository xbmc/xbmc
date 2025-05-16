/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/log.h"
#include "utils/StringUtils.h"

#include <exception>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

class CSysfsPath
{
public:
  CSysfsPath() = default;
  explicit CSysfsPath(const std::string& path) : m_path(path) {}
  template<typename T>
  explicit CSysfsPath(const std::string& path, T value) : m_path(path) { if (Exists()) { Set(value); } }
  ~CSysfsPath() = default;

  bool Exists() const;

  template <typename T>
  T GetOrDefault()
  {
    if (Exists())
    {
      auto result = Get<T>();
      if (result.has_value()) return result.value();
    }
    return T{};
  }

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

  template<typename T>
  void Set(T value)
  {
    std::ofstream file(m_path);

    file << value;

    if (file.bad())
    {
      CLog::LogF(LOGERROR, "error write to '{}'", m_path);
      throw std::runtime_error("error write to " + m_path);
    }
  }

private:
  std::string m_path;
};

template<>
std::optional<std::string> CSysfsPath::Get<std::string>();

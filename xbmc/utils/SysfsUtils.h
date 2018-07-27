/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class SysfsUtils
{
public:
  static int SetString(const std::string& path, const std::string& valstr);
  static int GetString(const std::string& path, std::string& valstr);
  static int SetInt(const std::string& path, const int val);
  static int GetInt(const std::string& path, int& val);
  static bool Has(const std::string& path);
  static bool HasRW(const std::string &path);
};

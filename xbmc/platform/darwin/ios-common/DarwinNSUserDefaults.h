/*
 *  Copyright (C) 2015 Team MrMC
 *      https://github.com/MrMC
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CDarwinNSUserDefaults
{
public:
  static bool Synchronize();

  static bool GetKey(const std::string &key, std::string &value);
  static bool SetKey(const std::string &key, const std::string &value, bool synchronize);
  static bool DeleteKey(const std::string &key, bool synchronize);
  static bool KeyExists(const std::string &key);

  static bool IsKeyFromPath(const std::string &key);
  static bool GetKeyFromPath(const std::string &path, std::string &value);
  static bool SetKeyFromPath(const std::string &path, const std::string &value, bool synchronize);
  static bool DeleteKeyFromPath(const std::string &path, bool synchronize);
  static bool KeyFromPathExists(const std::string &key);
};

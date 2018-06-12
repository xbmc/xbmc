/*
 *      Copyright (C) 2015 Team MrMC
 *      https://github.com/MrMC
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with MrMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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

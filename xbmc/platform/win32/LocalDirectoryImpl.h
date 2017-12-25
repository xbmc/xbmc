#pragma once
/*
 *      Copyright (C) 2014 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>

#include "URL.h"

namespace KODI
{
namespace PLATFORM
{
namespace DETAILS
{

class CLocalDirectoryImpl
{
public:
  static bool GetDirectory(const CURL &url, std::vector<std::string> &items);
  static bool GetDirectory(std::string path, std::vector<std::string> &items);
  static bool Create(const CURL &url);
  static bool Create(std::string path);
  static bool Exists(const CURL &url);
  static bool Exists(const std::string &url);
  static bool Remove(const CURL &url);
  static bool Remove(const std::string &url);
  static bool RemoveRecursive(const CURL &url);
  static bool RemoveRecursive(std::string url);

private:
  static bool CreateInternal(std::wstring path);
};

} // namespace DETAILS
} // namespace PLATFORM
} // namespace KODI

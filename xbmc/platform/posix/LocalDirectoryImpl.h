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
bool GetDirectory(std::string url, std::vector<std::string> &items);
bool Create(std::string path);
bool Exists(std::string url);
bool Remove(std::string url);
bool RemoveRecursive(std::string url);
std::string CreateSystemTempDirectory(std::string directory = "xbmctempdirXXXXXX");
} // namespace DETAILS
} // namespace PLATFORM
} // namespace KODI

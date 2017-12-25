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

#include "LocalDirectory.h"

#if defined(TARGET_WINDOWS) || defined(TARGET_WINDOWS_STORE)
#include "platform/win32/LocalDirectoryImpl.h"
#elif defined(TARGET_POSIX)
#include "platform/posix/LocalDirectoryImpl.h"
#else
#error "No implementation yet"
#endif

namespace KODI
{
namespace PLATFORM
{

bool CLocalDirectory::GetDirectory(const CURL &url, std::vector<std::string> &items)
{
  return DETAILS::GetDirectory(url.Get(), items);
}

bool CLocalDirectory::GetDirectory(std::string url, std::vector<std::string> &items)
{
  return DETAILS::GetDirectory(url, items);
}

bool CLocalDirectory::Create(const CURL &url)
{
  return DETAILS::Create(url.Get());
}

bool CLocalDirectory::Create(std::string url)
{
  return DETAILS::Create(url);
}

bool CLocalDirectory::Remove(const CURL &url)
{
  return DETAILS::Remove(url.Get());
}

bool CLocalDirectory::Remove(const std::string &url)
{
  return DETAILS::Remove(url);
}

bool CLocalDirectory::RemoveRecursive(const CURL &url)
{
  return DETAILS::RemoveRecursive(url.Get());
}

bool CLocalDirectory::RemoveRecursive(std::string url)
{
  return DETAILS::RemoveRecursive(url);
}

std::string CLocalDirectory::CreateSystemTempDirectory(std::string directory)
{
  return DETAILS::CreateSystemTempDirectory(directory);
}

bool CLocalDirectory::Exists(const CURL &url)
{
  return DETAILS::Exists(url.Get());
}

bool CLocalDirectory::Exists(const std::string &url)
{
  return DETAILS::Exists(url);
}

} // namespace PLATFORM
} // namespace KODI

/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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

#include "platform/Filesystem.h"
#include "platform/win32/CharsetConverter.h"

#include <Windows.h>

namespace win = KODI::PLATFORM::WINDOWS;

namespace KODI
{
namespace PLATFORM
{
namespace FILESYSTEM
{
space_info space(const std::string& path, std::error_code& ec)
{

  ec.clear();
  space_info sp;
  auto pathW = win::ToW(path);

  ULARGE_INTEGER capacity;
  ULARGE_INTEGER available;
  ULARGE_INTEGER free;
  auto result = GetDiskFreeSpaceExW(pathW.c_str(), &available, &capacity, &free);

  if (result == FALSE)
  {
    ec.assign(GetLastError(), std::system_category());
    sp.available = static_cast<uintmax_t>(-1);
    sp.capacity = static_cast<uintmax_t>(-1);
    sp.free = static_cast<uintmax_t>(-1);
    return sp;
  }

  sp.available = static_cast<uintmax_t>(available.QuadPart);
  sp.capacity = static_cast<uintmax_t>(capacity.QuadPart);
  sp.free = static_cast<uintmax_t>(free.QuadPart);

  return sp;
}

std::string temp_directory_path(std::error_code &ec)
{
  wchar_t lpTempPathBuffer[MAX_PATH + 1];

  if (!GetTempPathW(MAX_PATH, lpTempPathBuffer))
  {
    ec.assign(GetLastError(), std::system_category());
    return std::string();
  }

  ec.clear();
  return win::FromW(lpTempPathBuffer);
}

std::string create_temp_directory(std::error_code &ec)
{
  wchar_t lpTempPathBuffer[MAX_PATH + 1];

  std::wstring xbmcTempPath = win::ToW(temp_directory_path(ec));

  if (ec)
    return std::string();

  if (!GetTempFileNameW(xbmcTempPath.c_str(), L"xbm", 0, lpTempPathBuffer))
  {
    ec.assign(GetLastError(), std::system_category());
    return std::string();
  }

  DeleteFileW(lpTempPathBuffer);

  if (!CreateDirectoryW(lpTempPathBuffer, nullptr))
  {
    ec.assign(GetLastError(), std::system_category());
    return std::string();
  }

  ec.clear();
  return win::FromW(lpTempPathBuffer);
}

std::string temp_file_path(std::string, std::error_code &ec)
{
  wchar_t lpTempPathBuffer[MAX_PATH + 1];

  std::wstring xbmcTempPath = win::ToW(create_temp_directory(ec));

  if (ec)
    return std::string();

  if (!GetTempFileNameW(xbmcTempPath.c_str(), L"xbm", 0, lpTempPathBuffer))
  {
    ec.assign(GetLastError(), std::system_category());
    return std::string();
  }

  DeleteFileW(lpTempPathBuffer);

  ec.clear();
  return win::FromW(lpTempPathBuffer);
}

}
}
}

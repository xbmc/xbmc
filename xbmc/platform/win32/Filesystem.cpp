/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/Filesystem.h"

#include "utils/StringUtils.h"

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

namespace
{
std::wstring temp_directory_path_w(std::error_code& ec)
{
  wchar_t lpTempPathBuffer[MAX_PATH + 1];

  if (!GetTempPathW(MAX_PATH, lpTempPathBuffer))
  {
    ec.assign(GetLastError(), std::system_category());
    return std::wstring();
  }

  // Save this to return in case the tests succeed
  auto tempDir = std::wstring(lpTempPathBuffer);

  if (!GetTempFileNameW(tempDir.c_str(), L"xbm", 0, lpTempPathBuffer))
  {
    tempDir = std::wstring(L"C:\\Windows\\SystemTemp\\");
    if (!GetTempFileNameW(tempDir.c_str(), L"xbm", 0, lpTempPathBuffer))
    {
      ec.assign(GetLastError(), std::system_category());
      return std::wstring();
    }
  }

  if (!DeleteFileW(lpTempPathBuffer))
  {
    auto err = GetLastError();
    if (err != ERROR_FILE_NOT_FOUND)
    {
      ec.assign(err, std::system_category());
      return std::wstring();
    }
  }

  ec.clear();
  return tempDir;
}
} // namespace

std::string temp_directory_path(std::error_code& ec)
{
  return win::FromW(temp_directory_path_w(ec));
}

std::string create_temp_directory(std::error_code& ec)
{
  wchar_t lpTempPathBuffer[MAX_PATH + 1];

  std::wstring xbmcTempPath = temp_directory_path_w(ec);

  if (!GetTempFileNameW(xbmcTempPath.c_str(), L"xbm", 0, lpTempPathBuffer))
  {
    ec.assign(GetLastError(), std::system_category());
    return std::string();
  }

  if (!DeleteFileW(lpTempPathBuffer))
  {
    auto err = GetLastError();
    if (err != ERROR_FILE_NOT_FOUND)
    {
      ec.assign(err, std::system_category());
      return std::string();
    }
  }

  if (!CreateDirectoryW(lpTempPathBuffer, nullptr))
  {
    ec.assign(GetLastError(), std::system_category());
    return std::string();
  }

  ec.clear();
  return win::FromW(lpTempPathBuffer);
}

std::string temp_file_path(const std::string&, std::error_code& ec)
{
  wchar_t lpTempPathBuffer[MAX_PATH + 1];

  std::wstring xbmcTempPath = temp_directory_path_w(ec);

  if (ec)
    return std::string();

  if (!GetTempFileNameW(xbmcTempPath.c_str(), L"xbm", 0, lpTempPathBuffer))
  {
    ec.assign(GetLastError(), std::system_category());
    return std::string();
  }

  if (!DeleteFileW(lpTempPathBuffer))
  {
    auto err = GetLastError();
    if (err != ERROR_FILE_NOT_FOUND)
    {
      ec.assign(err, std::system_category());
      return std::string();
    }
  }

  ec.clear();
  return win::FromW(lpTempPathBuffer);
}

} // namespace FILESYSTEM
} // namespace PLATFORM
} // namespace KODI

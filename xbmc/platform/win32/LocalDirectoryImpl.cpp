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

#include "LocalDirectoryImpl.h"
#include "URL.h"
#include "platform/win32/CharsetConverter.h"
#include "platform/win32/WIN32Util.h"
#include "utils/SystemInfo.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif // WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace KODI
{
namespace PLATFORM
{
namespace DETAILS
{
// check for empty string, remove trailing slash if any, convert to win32 form
inline static std::wstring prepareWin32DirectoryName(const std::string &strPath)
{
  if (strPath.empty())
    return std::wstring(); // empty string

  std::wstring nameW(CWIN32Util::ConvertPathToWin32Form(strPath));
  if (!nameW.empty())
  {
    if (nameW.back() == L'\\')
      nameW.pop_back();                              // remove slash at the end if any
    if (nameW.length() == 6 && nameW.back() == L':') // 6 is the length of "\\?\x:"
      nameW.push_back(L'\\');                        // always add backslash for root folders
  }
  return nameW;
}

bool CreateInternal(std::wstring path)
{
  if (!CreateDirectoryW(path.c_str(), nullptr))
  {
    if (GetLastError() == ERROR_ALREADY_EXISTS)
      return true;

    if (GetLastError() != ERROR_PATH_NOT_FOUND)
      return false;

    auto sep = path.rfind(L'\\');
    if (sep == std::wstring::npos)
      return false;

    if (CreateInternal(path.substr(0, sep)))
      return CreateInternal(path);

    return false;
  }

  // if directory name starts from dot, make it hidden
  const auto lastSlashPos = path.rfind(L'\\');
  if (lastSlashPos < path.length() - 1 && path[lastSlashPos + 1] == L'.')
  {
    DWORD dirAttrs = GetFileAttributesW(path.c_str());
    if (dirAttrs != INVALID_FILE_ATTRIBUTES &&
        SetFileAttributesW(path.c_str(), dirAttrs | FILE_ATTRIBUTE_HIDDEN))
      return true;
  }

  return true;
}

bool GetDirectory(std::string path, std::vector<std::string> &items)
{
  using KODI::PLATFORM::WINDOWS::FromW;

  items.clear();

  if (!path.empty() && path.back() != '\\')
    path.push_back('\\');

  std::wstring searchMask(CWIN32Util::ConvertPathToWin32Form(path));
  if (searchMask.empty())
    return false;

  //! @todo support m_strFileMask, require rewrite of internal caching
  searchMask += L'*';

  HANDLE hSearch;
  WIN32_FIND_DATAW findData = {};

  hSearch = FindFirstFileExW(searchMask.c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch,
                             NULL, FIND_FIRST_EX_LARGE_FETCH);

  if (hSearch == INVALID_HANDLE_VALUE)
    return GetLastError() == ERROR_FILE_NOT_FOUND
               ? Exists(path)
               : false; // return true if directory exist and empty

  do
  {
    std::wstring itemNameW(findData.cFileName);
    if (itemNameW == L"." || itemNameW == L"..")
      continue;

    auto itemName = FromW(itemNameW);
    if (itemName.empty())
    {
      CLog::Log(LOGERROR, "%s: Can't convert wide string name to UTF-8 encoding", __FUNCTION__);
      continue;
    }

    if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
      items.emplace_back(URIUtils::AppendSlash(URIUtils::AddFileToFolder(path, itemName)));
    else
      items.emplace_back(URIUtils::AddFileToFolder(path, itemName));

  } while (FindNextFileW(hSearch, &findData));

  FindClose(hSearch);

  return true;
}

bool Create(std::string url)
{
  auto nameW(prepareWin32DirectoryName(url));
  if (nameW.empty())
    return false;

  if (!CreateInternal(nameW))
    return Exists(url);

  return true;
}

bool Exists(std::string url)
{
  std::wstring nameW(prepareWin32DirectoryName(url));
  if (nameW.empty())
    return false;

  DWORD fileAttrs = GetFileAttributesW(nameW.c_str());
  if (fileAttrs == INVALID_FILE_ATTRIBUTES || (fileAttrs & FILE_ATTRIBUTE_DIRECTORY) == 0)
    return false;

  return true;
}

bool Remove(std::string url)
{
  std::wstring nameW(prepareWin32DirectoryName(url));
  if (nameW.empty())
    return false;

  if (RemoveDirectoryW(nameW.c_str()))
    return true;

  return !Exists(url);
}

bool RemoveRecursive(std::string path)
{
  using KODI::PLATFORM::WINDOWS::FromW;

  if (!path.empty() && path.back() != '\\')
    path.push_back('\\');

  auto basePath = CWIN32Util::ConvertPathToWin32Form(path);
  if (basePath.empty())
    return false;

  auto searchMask = basePath + L'*';

  HANDLE hSearch;
  WIN32_FIND_DATAW findData = {};

  if (g_sysinfo.IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin7))
    hSearch = FindFirstFileExW(searchMask.c_str(), FindExInfoBasic, &findData,
                               FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
  else
    hSearch = FindFirstFileExW(searchMask.c_str(), FindExInfoStandard, &findData,
                               FindExSearchNameMatch, nullptr, 0);

  if (hSearch == INVALID_HANDLE_VALUE)
    return GetLastError() == ERROR_FILE_NOT_FOUND
               ? Exists(path)
               : false; // return true if directory exist and empty

  bool success = true;
  do
  {
    std::wstring itemNameW(findData.cFileName);
    if (itemNameW == L"." || itemNameW == L"..")
      continue;

    auto pathW = basePath + itemNameW;
    if (0 != (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
      auto path = FromW(pathW);
      if (path.empty())
      {
        CLog::Log(LOGERROR, "%s: Can't convert wide string name to UTF-8 encoding", __FUNCTION__);
        continue;
      }

      if (!RemoveRecursive(path))
      {
        success = false;
        break;
      }
    }
    else
    {
      if (FALSE == DeleteFileW(pathW.c_str()))
      {
        success = false;
        break;
      }
    }
  } while (FindNextFileW(hSearch, &findData));

  FindClose(hSearch);

  if (success)
  {
    if (FALSE == RemoveDirectoryW(basePath.c_str()))
      success = false;
  }

  return success;
}

std::string CreateSystemTempDirectory(std::string directory)
{
  using namespace KODI::PLATFORM::WINDOWS;

  wchar_t lpTempPathBuffer[MAX_PATH + 1];

  if (!GetTempPathW(MAX_PATH, lpTempPathBuffer))
    return std::string();

  std::wstring xbmcTempPath = lpTempPathBuffer;
  if (!GetTempFileNameW(xbmcTempPath.c_str(), ToW(directory).c_str(), 0, lpTempPathBuffer))
    return std::string();

  DeleteFileW(lpTempPathBuffer);

  if (!CreateDirectoryW(lpTempPathBuffer, nullptr))
    return std::string();

  return FromW(lpTempPathBuffer);
}

} // namespace DETAILS
} // namespace PLATFORM
} // namespace KODI

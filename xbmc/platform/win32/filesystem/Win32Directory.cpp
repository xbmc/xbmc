/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Win32Directory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "utils/CharsetConverter.h"
#include "utils/SystemInfo.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include "platform/win32/WIN32Util.h"

#include <Windows.h>

using namespace XFILE;

// check for empty string, remove trailing slash if any, convert to win32 form
inline static std::wstring prepareWin32DirectoryName(const std::string& strPath)
{
  if (strPath.empty())
    return std::wstring(); // empty string

  std::wstring nameW(CWIN32Util::ConvertPathToWin32Form(strPath));
  if (!nameW.empty())
  {
    if (nameW.back() == L'\\')
      nameW.pop_back(); // remove slash at the end if any
    if (nameW.length() == 6 && nameW.back() == L':') // 6 is the length of "\\?\x:"
      nameW.push_back(L'\\'); // always add backslash for root folders
  }
  return nameW;
}

CWin32Directory::CWin32Directory(void)
{}

CWin32Directory::~CWin32Directory(void)
{}

bool CWin32Directory::GetDirectory(const CURL& url, CFileItemList &items)
{
  std::string pathWithSlash(url.Get());
  if (!pathWithSlash.empty() && pathWithSlash.back() != '\\')
    pathWithSlash.push_back('\\');

  std::wstring searchMask(CWIN32Util::ConvertPathToWin32Form(pathWithSlash));
  if (searchMask.empty())
    return false;

  //! @todo support m_strFileMask, require rewrite of internal caching
  searchMask += L'*';

  HANDLE hSearch;
  WIN32_FIND_DATAW findData = {};

  hSearch = FindFirstFileExW(searchMask.c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);

  if (hSearch == INVALID_HANDLE_VALUE)
    return GetLastError() == ERROR_FILE_NOT_FOUND ? Exists(url) : false; // return true if directory exist and empty

  do
  {
    std::wstring itemNameW(findData.cFileName);
    if (itemNameW == L"." || itemNameW == L"..")
      continue;

    std::string itemName;
    if (!g_charsetConverter.wToUTF8(itemNameW, itemName, true) || itemName.empty())
    {
      CLog::Log(LOGERROR, "{}: Can't convert wide string name to UTF-8 encoding", __FUNCTION__);
      continue;
    }

    CFileItemPtr pItem(new CFileItem(itemName));

    pItem->m_bIsFolder = ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
    if (pItem->m_bIsFolder)
      pItem->SetPath(pathWithSlash + itemName + '\\');
    else
      pItem->SetPath(pathWithSlash + itemName);

    if ((findData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) != 0
          || itemName.front() == '.') // mark files starting from dot as hidden
      pItem->SetProperty("file:hidden", true);

    // calculation of size and date costs a little on win32
    // so DIR_FLAG_NO_FILE_INFO flag is ignored
    KODI::TIME::FileTime fileTime;
    fileTime.lowDateTime = findData.ftLastWriteTime.dwLowDateTime;
    fileTime.highDateTime = findData.ftLastWriteTime.dwHighDateTime;
    KODI::TIME::FileTime localTime;
    if (KODI::TIME::FileTimeToLocalFileTime(&fileTime, &localTime) == TRUE)
      pItem->m_dateTime = localTime;
    else
      pItem->m_dateTime = 0;

    if (!pItem->m_bIsFolder)
        pItem->m_dwSize = (__int64(findData.nFileSizeHigh) << 32) + findData.nFileSizeLow;

    items.Add(pItem);
  } while (FindNextFileW(hSearch, &findData));

  FindClose(hSearch);

  return true;
}

bool CWin32Directory::Create(const CURL& url)
{
  auto nameW(prepareWin32DirectoryName(url.Get()));
  if (nameW.empty())
    return false;

  if (!Create(nameW))
    return Exists(url);

  return true;
}

bool CWin32Directory::Exists(const CURL& url)
{
  std::wstring nameW(prepareWin32DirectoryName(url.Get()));
  if (nameW.empty())
    return false;

  DWORD fileAttrs = GetFileAttributesW(nameW.c_str());
  if (fileAttrs == INVALID_FILE_ATTRIBUTES || (fileAttrs & FILE_ATTRIBUTE_DIRECTORY) == 0)
    return false;

  return true;
}

bool CWin32Directory::Remove(const CURL& url)
{
  std::wstring nameW(prepareWin32DirectoryName(url.Get()));
  if (nameW.empty())
    return false;

  if (RemoveDirectoryW(nameW.c_str()))
    return true;

  return !Exists(url);
}

bool CWin32Directory::RemoveRecursive(const CURL& url)
{
  std::string pathWithSlash(url.Get());
  if (!pathWithSlash.empty() && pathWithSlash.back() != '\\')
    pathWithSlash.push_back('\\');

  auto basePath = CWIN32Util::ConvertPathToWin32Form(pathWithSlash);
  if (basePath.empty())
    return false;

  auto searchMask = basePath + L'*';

  HANDLE hSearch;
  WIN32_FIND_DATAW findData = {};

  hSearch = FindFirstFileExW(searchMask.c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch,
                             nullptr, FIND_FIRST_EX_LARGE_FETCH);

  if (hSearch == INVALID_HANDLE_VALUE)
    return GetLastError() == ERROR_FILE_NOT_FOUND ? Exists(url) : false; // return true if directory exist and empty

  bool success = true;
  do
  {
    std::wstring itemNameW(findData.cFileName);
    if (itemNameW == L"." || itemNameW == L"..")
      continue;

    auto pathW = basePath + itemNameW;
    if (0 != (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
      std::string path;
      if (!g_charsetConverter.wToUTF8(pathW, path, true))
      {
        CLog::Log(LOGERROR, "{}: Can't convert wide string name to UTF-8 encoding", __FUNCTION__);
        continue;
      }

      if (!RemoveRecursive(CURL{ path }))
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

bool CWin32Directory::Create(std::wstring path) const
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

    if (Create(path.substr(0, sep)))
      return Create(path);

    return false;
  }

  // if directory name starts from dot, make it hidden
  const auto lastSlashPos = path.rfind(L'\\');
  if (lastSlashPos < path.length() - 1 && path[lastSlashPos + 1] == L'.')
  {
    DWORD dirAttrs = GetFileAttributesW(path.c_str());
    if (dirAttrs != INVALID_FILE_ATTRIBUTES && SetFileAttributesW(path.c_str(), dirAttrs | FILE_ATTRIBUTE_HIDDEN))
      return true;
  }

  return true;
}

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "HDDirectory.h"
#include "Util.h"
#include "iso9660.h"
#include "URL.h"
#include "FileItem.h"
#include "utils/AutoPtrHandle.h"
#include "utils/AliasShortcutUtils.h"
#include "utils/URIUtils.h"

#ifdef TARGET_WINDOWS
#include "utils/CharsetConverter.h"
#include "win32/WIN32Util.h"
#endif

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD) -1)
#endif

#ifdef TARGET_WINDOWS
typedef WIN32_FIND_DATAW LOCAL_WIN32_FIND_DATA;
#define LocalFindFirstFile FindFirstFileW
#define LocalFindNextFile FindNextFileW
#else
typedef WIN32_FIND_DATA LOCAL_WIN32_FIND_DATA;
#define LocalFindFirstFile FindFirstFile
#define LocalFindNextFile FindNextFile
#endif

using namespace AUTOPTR;
using namespace XFILE;

CHDDirectory::CHDDirectory(void)
{}

CHDDirectory::~CHDDirectory(void)
{}

bool CHDDirectory::GetDirectory(const CStdString& strPath1, CFileItemList &items)
{
  LOCAL_WIN32_FIND_DATA wfd;
  memset(&wfd, 0, sizeof(wfd));

  CStdString strPath=strPath1;

  if (IsAliasShortcut(strPath))
    TranslateAliasShortcut(strPath);

  CStdString strRoot = strPath;
  CURL url(strPath);

  URIUtils::AddSlashAtEnd(strRoot);
  if (URIUtils::IsDVD(strRoot) && m_isoReader.IsScanned())
  {
    // Reset iso reader and remount or
    // we can't access the dvd-rom
    m_isoReader.Reset();
  }

#ifdef TARGET_WINDOWS
  std::wstring strSearchMask(CWIN32Util::ConvertPathToWin32Form(strRoot));
  strSearchMask += L"*.*";
#else
  CStdString strSearchMask(strRoot);
#endif

  FILETIME localTime;
  CAutoPtrFind hFind ( LocalFindFirstFile(strSearchMask.c_str(), &wfd));

  // on error, check if path exists at all, this will return true if empty folder
  if (!hFind.isValid())
      return Exists(strPath1);

  if (hFind.isValid())
  {
    do
    {
      if (wfd.cFileName[0] != 0)
      {
        CStdString strLabel;
#ifdef TARGET_WINDOWS
        g_charsetConverter.wToUTF8(wfd.cFileName,strLabel, true);
#else
        strLabel = wfd.cFileName;
#endif
        if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
        {
          if (strLabel != "." && strLabel != "..")
          {
            CFileItemPtr pItem(new CFileItem(strLabel));
            CStdString itemPath(URIUtils::AddFileToFolder(strRoot, strLabel));
            URIUtils::AddSlashAtEnd(itemPath);
            pItem->SetPath(itemPath);
            pItem->m_bIsFolder = true;
            FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
            pItem->m_dateTime=localTime;

            if (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
              pItem->SetProperty("file:hidden", true);
            items.Add(pItem);
          }
        }
        else
        {
          CFileItemPtr pItem(new CFileItem(strLabel));
          pItem->SetPath(URIUtils::AddFileToFolder(strRoot, strLabel));
          pItem->m_bIsFolder = false;
          pItem->m_dwSize = CUtil::ToInt64(wfd.nFileSizeHigh, wfd.nFileSizeLow);
          FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
          pItem->m_dateTime=localTime;

          if (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
            pItem->SetProperty("file:hidden", true);

          items.Add(pItem);
        }
      }
    }
    while (LocalFindNextFile((HANDLE)hFind, &wfd));
  }
  return true;
}

bool CHDDirectory::Create(const char* strPath)
{
  if (!strPath || !*strPath)
    return false;
  CStdString strPath1 = strPath;
  URIUtils::AddSlashAtEnd(strPath1);

#ifdef TARGET_WINDOWS
  if (strPath1.size() == 3 && strPath1[1] == ':')
    return Exists(strPath);  // A drive - we can't "create" a drive
  if(::CreateDirectoryW(CWIN32Util::ConvertPathToWin32Form(strPath1).c_str(), NULL))
#else
  if(::CreateDirectory(strPath1.c_str(), NULL))
#endif
    return true;
  else if(GetLastError() == ERROR_ALREADY_EXISTS)
    return true;

  return false;
}

bool CHDDirectory::Remove(const char* strPath)
{
  if (!strPath || !*strPath)
    return false;
#ifdef TARGET_WINDOWS
  return (::RemoveDirectoryW(CWIN32Util::ConvertPathToWin32Form(strPath).c_str()) || GetLastError() == ERROR_PATH_NOT_FOUND) ? true : false;
#else
  return ::RemoveDirectory(strPath) ? true : false;
#endif
}

bool CHDDirectory::Exists(const char* strPath)
{
  if (!strPath || !*strPath)
    return false;
#ifdef TARGET_WINDOWS
  DWORD attributes = GetFileAttributesW(CWIN32Util::ConvertPathToWin32Form(strPath).c_str());
#else
  DWORD attributes = GetFileAttributes(strPath);
#endif
  if(attributes == INVALID_FILE_ATTRIBUTES)
    return false;
  if (FILE_ATTRIBUTE_DIRECTORY & attributes) return true;
  return false;
}

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

#ifndef TARGET_POSIX
#include "utils/CharsetConverter.h"
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

  CStdString strPath=strPath1;

  if (IsAliasShortcut(strPath))
    TranslateAliasShortcut(strPath);

  CStdString strRoot = strPath;
  CURL url(strPath);

  memset(&wfd, 0, sizeof(wfd));
  URIUtils::AddSlashAtEnd(strRoot);
#ifdef TARGET_WINDOWS
  strRoot.Replace("/", "\\");
#endif
  if (URIUtils::IsDVD(strRoot) && m_isoReader.IsScanned())
  {
    // Reset iso reader and remount or
    // we can't access the dvd-rom
    m_isoReader.Reset();
  }

#ifdef TARGET_WINDOWS
  CStdStringW strSearchMask;
  g_charsetConverter.utf8ToW(strRoot, strSearchMask, false);
  strSearchMask.Insert(0, L"\\\\?\\");
  strSearchMask += "*.*";
#else
  CStdString strSearchMask = strRoot;
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
        g_charsetConverter.wToUTF8(wfd.cFileName,strLabel);
#else
        strLabel = wfd.cFileName;
#endif
        if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
        {
          if (strLabel != "." && strLabel != "..")
          {
            CFileItemPtr pItem(new CFileItem(strLabel));
            CStdString itemPath = strRoot + strLabel;
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
          pItem->SetPath(strRoot + strLabel);
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
  CStdString strPath1 = strPath;
  URIUtils::AddSlashAtEnd(strPath1);

#ifdef TARGET_WINDOWS
  if (strPath1.size() == 3 && strPath1[1] == ':')
    return Exists(strPath);  // A drive - we can't "create" a drive
  CStdStringW strWPath1;
  strPath1.Replace("/", "\\");
  g_charsetConverter.utf8ToW(strPath1, strWPath1, false);
  strWPath1.Insert(0, L"\\\\?\\");
  if(::CreateDirectoryW(strWPath1, NULL))
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
#ifdef TARGET_WINDOWS
  CStdStringW strWPath;
  g_charsetConverter.utf8ToW(strPath, strWPath, false);
  strWPath.Replace(L"/", L"\\");
  strWPath.Insert(0, L"\\\\?\\");
  return (::RemoveDirectoryW(strWPath) || GetLastError() == ERROR_PATH_NOT_FOUND) ? true : false;
#else
  return ::RemoveDirectory(strPath) ? true : false;
#endif
}

bool CHDDirectory::Exists(const char* strPath)
{
  if (!strPath || !*strPath)
    return false;
  CStdString strReplaced=strPath;
#ifdef TARGET_WINDOWS
  CStdStringW strWReplaced;
  strReplaced.Replace("/","\\");
  URIUtils::AddSlashAtEnd(strReplaced);
  g_charsetConverter.utf8ToW(strReplaced, strWReplaced, false);
  strWReplaced.Insert(0, L"\\\\?\\");
  DWORD attributes = GetFileAttributesW(strWReplaced);
#else
  DWORD attributes = GetFileAttributes(strReplaced.c_str());
#endif
  if(attributes == INVALID_FILE_ATTRIBUTES)
    return false;
  if (FILE_ATTRIBUTE_DIRECTORY & attributes) return true;
  return false;
}

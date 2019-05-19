/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ISO9660Directory.h"

#include "FileItem.h"
#include "URL.h"
#include "Util.h"
#include "iso9660.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XTimeUtils.h"

#ifdef TARGET_WINDOWS
#include "platform/win32/CharsetConverter.h"
#endif

using namespace XFILE;

CISO9660Directory::CISO9660Directory(void) = default;

CISO9660Directory::~CISO9660Directory(void) = default;

bool CISO9660Directory::GetDirectory(const CURL& url, CFileItemList &items)
{
  std::string strRoot = url.Get();
  URIUtils::AddSlashAtEnd(strRoot);

  // Scan active disc if not done before
  if (!m_isoReader.IsScanned())
    m_isoReader.Scan();

  Win32FindData wfd;
  HANDLE hFind;

  memset(&wfd, 0, sizeof(wfd));

  std::string strSearchMask;
  std::string strDirectory = url.GetFileName();
  if (strDirectory != "")
  {
    strSearchMask = StringUtils::Format("\\%s", strDirectory.c_str());
  }
  else
  {
    strSearchMask = "\\";
  }
  for (int i = 0; i < (int)strSearchMask.size(); ++i )
  {
    if (strSearchMask[i] == '/') strSearchMask[i] = '\\';
  }

  hFind = m_isoReader.FindFirstFile9660(strSearchMask.c_str(), &wfd);
  if (hFind == NULL)
    return false;

  do
  {
    if (wfd.fileName[0] != 0)
    {
      if ((wfd.fileAttributes & FILE_ATTRIBUTE_DIRECTORY))
      {
        std::string strDir = wfd.fileName;
        if (strDir != "." && strDir != "..")
        {
          CFileItemPtr pItem(new CFileItem(strDir));
          std::string path = strRoot + strDir;
          URIUtils::AddSlashAtEnd(path);
          pItem->SetPath(path);
          pItem->m_bIsFolder = true;
          FILETIME localTime;
          KODI::TIME::FileTimeToLocalFileTime(&wfd.lastWriteTime, &localTime);
          pItem->m_dateTime=localTime;
          items.Add(pItem);
        }
      }
      else
      {
        std::string strDir = wfd.fileName;
        CFileItemPtr pItem(new CFileItem(strDir));
        pItem->SetPath(strRoot + strDir);
        pItem->m_bIsFolder = false;
        pItem->m_dwSize = CUtil::ToInt64(wfd.fileSizeHigh, wfd.fileSizeLow);
        FILETIME localTime;
        KODI::TIME::FileTimeToLocalFileTime(&wfd.lastWriteTime, &localTime);
        pItem->m_dateTime=localTime;
        items.Add(pItem);
      }
    }
  }
  while (m_isoReader.FindNextFile(hFind, &wfd));
  m_isoReader.FindClose(hFind);

  return true;
}

bool CISO9660Directory::Exists(const CURL& url)
{
  CFileItemList items;
  if (GetDirectory(url,items))
    return true;

  return false;
}

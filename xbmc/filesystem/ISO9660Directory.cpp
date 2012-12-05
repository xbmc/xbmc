/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "ISO9660Directory.h"
#include "iso9660.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "FileItem.h"

using namespace XFILE;

CISO9660Directory::CISO9660Directory(void)
{}

CISO9660Directory::~CISO9660Directory(void)
{}

bool CISO9660Directory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CStdString strRoot = strPath;
  URIUtils::AddSlashAtEnd(strRoot);

  // Scan active disc if not done before
  if (!m_isoReader.IsScanned())
    m_isoReader.Scan();

  CURL url(strPath);

  WIN32_FIND_DATA wfd;
  HANDLE hFind;

  memset(&wfd, 0, sizeof(wfd));

  CStdString strSearchMask;
  CStdString strDirectory = url.GetFileName();
  if (strDirectory != "")
  {
    strSearchMask.Format("\\%s", strDirectory.c_str());
  }
  else
  {
    strSearchMask = "\\";
  }
  for (int i = 0; i < (int)strSearchMask.size(); ++i )
  {
    if (strSearchMask[i] == '/') strSearchMask[i] = '\\';
  }

  hFind = m_isoReader.FindFirstFile((char*)strSearchMask.c_str(), &wfd);
  if (hFind == NULL)
    return false;

  do
  {
    if (wfd.cFileName[0] != 0)
    {
      if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
      {
        CStdString strDir = wfd.cFileName;
        if (strDir != "." && strDir != "..")
        {
          CFileItemPtr pItem(new CFileItem(wfd.cFileName));
          CStdString path = strRoot + wfd.cFileName;
          URIUtils::AddSlashAtEnd(path);
          pItem->SetPath(path);
          pItem->m_bIsFolder = true;
          FILETIME localTime;
          FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
          pItem->m_dateTime=localTime;
          items.Add(pItem);
        }
      }
      else
      {
        CFileItemPtr pItem(new CFileItem(wfd.cFileName));
        pItem->SetPath(strRoot + wfd.cFileName);
        pItem->m_bIsFolder = false;
        pItem->m_dwSize = CUtil::ToInt64(wfd.nFileSizeHigh, wfd.nFileSizeLow);
        FILETIME localTime;
        FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
        pItem->m_dateTime=localTime;
        items.Add(pItem);
      }
    }
  }
  while (m_isoReader.FindNextFile(hFind, &wfd));
  m_isoReader.FindClose(hFind);

  return true;
}

bool CISO9660Directory::Exists(const char* strPath)
{
  CFileItemList items;
  if (GetDirectory(strPath,items))
    return true;

  return false;
}

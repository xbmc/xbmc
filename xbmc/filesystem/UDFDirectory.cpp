/*
 *      Copyright (C) 2010 Team Boxee
 *      http://www.boxee.tv
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "UDFDirectory.h"
#include "udf25.h"
#include "Util.h"
#include "URL.h"
#include "FileItem.h"
#include "utils/URIUtils.h"

using namespace XFILE;

CUDFDirectory::CUDFDirectory(void)
{
}

CUDFDirectory::~CUDFDirectory(void)
{
}

bool CUDFDirectory::GetDirectory(const CStdString& strPath,
                                 CFileItemList &items)
{
  CStdString strRoot = strPath;
  URIUtils::AddSlashAtEnd(strRoot);

  CURL url(strPath);

  CStdString strDirectory = url.GetHostName();
  CURL::Decode(strDirectory);

  udf25 udfIsoReader;
  udf_dir_t *dirp = udfIsoReader.OpenDir(strDirectory);

  if (dirp == NULL)
    return false;

  udf_dirent_t *dp = NULL;
  while ((dp = udfIsoReader.ReadDir(dirp)) != NULL)
  {
    if (dp->d_type == DVD_DT_DIR)
    {
      CStdString strDir = (char*)dp->d_name;
      if (strDir != "." && strDir != "..")
      {
        CFileItemPtr pItem(new CFileItem((char*)dp->d_name));
        pItem->m_strPath = strRoot;
        pItem->m_strPath += (char*)dp->d_name;
        pItem->m_bIsFolder = true;
        URIUtils::AddSlashAtEnd(pItem->m_strPath);

        items.Add(pItem);
      }
    }
    else
    {
      CFileItemPtr pItem(new CFileItem((char*)dp->d_name));
      pItem->m_strPath = strRoot;
      pItem->m_strPath += (char*)dp->d_name;
      pItem->m_bIsFolder = false;
      pItem->m_dwSize = dp->d_filesize;

      items.Add(pItem);
    }	
  }

  udfIsoReader.CloseDir(dirp);

  return true;
}

bool CUDFDirectory::Exists(const char* strPath)
{
  CFileItemList items;
  if (GetDirectory(strPath,items))
    return true;

  return false;
}

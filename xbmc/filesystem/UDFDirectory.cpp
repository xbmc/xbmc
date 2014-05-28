/*
 *      Copyright (C) 2010 Team Boxee
 *      http://www.boxee.tv
 *
 *      Copyright (C) 2010-2013 Team XBMC
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
#ifndef FILESYSTEM_UDFDIRECTORY_H_INCLUDED
#define FILESYSTEM_UDFDIRECTORY_H_INCLUDED
#include "UDFDirectory.h"
#endif

#ifndef FILESYSTEM_UDF25_H_INCLUDED
#define FILESYSTEM_UDF25_H_INCLUDED
#include "udf25.h"
#endif

#ifndef FILESYSTEM_UTIL_H_INCLUDED
#define FILESYSTEM_UTIL_H_INCLUDED
#include "Util.h"
#endif

#ifndef FILESYSTEM_URL_H_INCLUDED
#define FILESYSTEM_URL_H_INCLUDED
#include "URL.h"
#endif

#ifndef FILESYSTEM_FILEITEM_H_INCLUDED
#define FILESYSTEM_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif

#ifndef FILESYSTEM_UTILS_URIUTILS_H_INCLUDED
#define FILESYSTEM_UTILS_URIUTILS_H_INCLUDED
#include "utils/URIUtils.h"
#endif


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
  CStdString strRoot, strSub;
  CURL url;
  if(StringUtils::StartsWith(strPath, "udf://"))
  {
    url.Parse(strPath);
    CURL url(strPath);
  }
  else
  {
    url.SetProtocol("udf");
    url.SetHostName(strPath);
  }
  strRoot  = url.Get();
  strSub   = url.GetFileName();

  URIUtils::AddSlashAtEnd(strRoot);
  URIUtils::AddSlashAtEnd(strSub);

  udf25 udfIsoReader;
  if(!udfIsoReader.Open(url.GetHostName()))
     return false;

  udf_dir_t *dirp = udfIsoReader.OpenDir(strSub);

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
        strDir = strRoot + (char*)dp->d_name;
        URIUtils::AddSlashAtEnd(strDir);
        pItem->SetPath(strDir);
        pItem->m_bIsFolder = true;

        items.Add(pItem);
      }
    }
    else
    {
      CFileItemPtr pItem(new CFileItem((char*)dp->d_name));
      pItem->SetPath(strRoot + (char*)dp->d_name);
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

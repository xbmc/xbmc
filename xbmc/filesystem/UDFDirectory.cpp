/*
 *  Copyright (C) 2010 Team Boxee
 *      http://www.boxee.tv
 *
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "UDFDirectory.h"
#include "udf25.h"
#include "Util.h"
#include "URL.h"
#include "FileItem.h"
#include "utils/URIUtils.h"

using namespace XFILE;

CUDFDirectory::CUDFDirectory(void) = default;

CUDFDirectory::~CUDFDirectory(void) = default;

bool CUDFDirectory::GetDirectory(const CURL& url,
                                 CFileItemList &items)
{
  std::string strRoot, strSub;
  CURL url2(url);
  if (!url2.IsProtocol("udf"))
  { // path to an image
    url2.Reset();
    url2.SetProtocol("udf");
    url2.SetHostName(url.Get());
  }
  strRoot  = url2.Get();
  strSub   = url2.GetFileName();

  URIUtils::AddSlashAtEnd(strRoot);
  URIUtils::AddSlashAtEnd(strSub);

  udf25 udfIsoReader;
  if(!udfIsoReader.Open(url2.GetHostName().c_str()))
     return false;

  udf_dir_t *dirp = udfIsoReader.OpenDir(strSub.c_str());

  if (dirp == NULL)
    return false;

  udf_dirent_t *dp = NULL;
  while ((dp = udfIsoReader.ReadDir(dirp)) != NULL)
  {
    if (dp->d_type == DVD_DT_DIR)
    {
      std::string strDir = (char*)dp->d_name;
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

bool CUDFDirectory::Exists(const CURL& url)
{
  CFileItemList items;
  if (GetDirectory(url, items))
    return true;

  return false;
}

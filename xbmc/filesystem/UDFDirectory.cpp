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

#include "FileItem.h"
#include "URL.h"
#include "Util.h"
#include "utils/URIUtils.h"

#include <cdio/udf.h>

using namespace XFILE;

bool CUDFDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  CURL url2(url);
  if (!url2.IsProtocol("udf"))
  {
    url2.Reset();
    url2.SetProtocol("udf");
    url2.SetHostName(url.Get());
  }

  std::string strRoot(url2.Get());
  std::string strSub(url2.GetFileName());

  URIUtils::AddSlashAtEnd(strRoot);
  URIUtils::AddSlashAtEnd(strSub);

  udf_t* udf = udf_open(url2.GetHostName().c_str());

  if (!udf)
    return false;

  udf_dirent_t* root = udf_get_root(udf, true, 0);

  if (!root)
  {
    udf_close(udf);
    return false;
  }

  udf_dirent_t* path = udf_fopen(root, strSub.c_str());

  if (!path)
  {
    udf_dirent_free(root);
    udf_close(udf);
    return false;
  }

  while (udf_readdir(path))
  {
    if (path->b_parent)
      continue;

    if (udf_is_dir(path))
    {
      std::string filename = udf_get_filename(path);
      if (filename != "." && filename != "..")
      {
        CFileItemPtr pItem(new CFileItem(filename));
        std::string strDir(strRoot + filename);
        URIUtils::AddSlashAtEnd(strDir);
        pItem->SetPath(strDir);
        pItem->m_bIsFolder = true;

        items.Add(pItem);
      }
    }
    else
    {
      std::string filename = udf_get_filename(path);
      CFileItemPtr pItem(new CFileItem(filename));
      pItem->SetPath(strRoot + filename);
      pItem->m_bIsFolder = false;
      pItem->m_dwSize = udf_get_file_length(path);

      items.Add(pItem);
    }
  }

  udf_dirent_free(root);
  udf_close(udf);

  return true;
}

bool CUDFDirectory::Exists(const CURL& url)
{
  CFileItemList items;
  return GetDirectory(url, items);
}

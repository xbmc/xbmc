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
#include "FileItemList.h"
#include "URL.h"
#include "Util.h"
#include "filesystem/UDFBlockInput.h"
#include "utils/URIUtils.h"

#include <udfread/udfread.h>

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

  auto udf = udfread_init();

  if (!udf)
    return false;

  CUDFBlockInput udfbi;

  auto bi = udfbi.GetBlockInput(url2.GetHostName());

  if (udfread_open_input(udf, bi) < 0)
  {
    udfread_close(udf);
    return false;
  }

  auto path = udfread_opendir(udf, strSub.c_str());
  if (!path)
  {
    udfread_close(udf);
    return false;
  }

  struct udfread_dirent dirent;

  while (udfread_readdir(path, &dirent))
  {
    if (dirent.d_type == UDF_DT_DIR)
    {
      std::string filename = dirent.d_name;
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
      std::string filename = dirent.d_name;
      std::string filenameWithPath{strSub + filename};
      auto file = udfread_file_open(udf, filenameWithPath.c_str());
      if (!file)
        continue;

      CFileItemPtr pItem(new CFileItem(filename));
      pItem->SetPath(strRoot + filename);
      pItem->m_bIsFolder = false;
      pItem->m_dwSize = udfread_file_size(file);
      items.Add(pItem);

      udfread_file_close(file);
    }
  }

  udfread_closedir(path);
  udfread_close(udf);

  return true;
}

bool CUDFDirectory::Exists(const CURL& url)
{
  CFileItemList items;
  return GetDirectory(url, items);
}

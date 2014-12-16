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

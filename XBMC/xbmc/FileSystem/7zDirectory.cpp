/*
* XBMC
* 7z Filesystem
* Copyright (c) 2008 topfs2
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "stdafx.h"
//#include "FileDAAP.h"
#include "7zDirectory.h"
#include "CacheManager.h"
#include "../Util.h"
#include "DirectoryCache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace DIRECTORY
{
  C7zDirectory::C7zDirectory(void)
  { }

  C7zDirectory::~C7zDirectory(void)
  { }

  bool C7zDirectory::GetDirectory(const CStdString& strPathOrig, CFileItemList &items)
  {
    CStdString strPath;

    /* if this isn't a proper archive path, assume it's the path to a archive file */
    if( !strPathOrig.Left(5).Equals("7z://") )
      CUtil::CreateArchivePath(strPath, "7z", strPathOrig, "");
    else
      strPath = strPathOrig;

    CURL url(strPath);
    CStdString strArchive = url.GetHostName();
    CStdString strOptions = url.GetOptions();
    CStdString strPathInArchive = url.GetFileName();
    url.SetOptions("");

    CStdString strSlashPath;
    url.GetURL(strSlashPath);

    return g_7zManager.GetFilesIn7z(items, strArchive, true, strPathInArchive.c_str());
  }

  bool C7zDirectory::ContainsFiles(const CStdString& strPath)
  {
    CURL url(strPath);
    CStdString strArchive = url.GetHostName();
    CStdString strOptions = url.GetOptions();
    CStdString strPathInArchive = url.GetFileName();
    url.SetOptions("");

    CStdString strSlashPath;
    url.GetURL(strSlashPath);

    g_7zManager.IsFileIn7z(strArchive, strPathInArchive);
  }
  bool C7zDirectory::Exists(const char* strPath)
  {
    CURL url(strPath);
    CStdString strArchive = url.GetHostName();
    CStdString strOptions = url.GetOptions();
    CStdString strPathInArchive = url.GetFileName();
    url.SetOptions("");

    CStdString strSlashPath;
    url.GetURL(strSlashPath);

    g_7zManager.IsFileIn7z(strArchive, strPathInArchive);
  }
}


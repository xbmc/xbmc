/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "RarDirectory.h"
#include "RarManager.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "FileItem.h"

namespace XFILE
{
  CRarDirectory::CRarDirectory()
  {
  }

  CRarDirectory::~CRarDirectory()
  {
  }

  bool CRarDirectory::GetDirectory(const CStdString& strPathOrig, CFileItemList& items)
  {
    CStdString strPath;

    /* if this isn't a proper archive path, assume it's the path to a archive file */
    if( !strPathOrig.Left(6).Equals("rar://") )
      URIUtils::CreateArchivePath(strPath, "rar", strPathOrig, "");
    else
      strPath = strPathOrig;

    CURL url(strPath);
    CStdString strArchive = url.GetHostName();
    CStdString strOptions = url.GetOptions();
    CStdString strPathInArchive = url.GetFileName();
    url.SetOptions("");

    CStdString strSlashPath = url.Get();

    // the RAR code depends on things having a "\" at the end of the path
    URIUtils::AddSlashAtEnd(strSlashPath);

    if (g_RarManager.GetFilesInRar(items,strArchive,true,strPathInArchive))
    {
      // fill in paths
      for( int iEntry=0;iEntry<items.Size();++iEntry)
      {
        if (items[iEntry]->IsParentFolder())
          continue;
        items[iEntry]->SetPath(URIUtils::AddFileToFolder(strSlashPath,items[iEntry]->GetPath()+strOptions));
        items[iEntry]->m_iDriveType = 0;
        //CLog::Log(LOGDEBUG, "RarXFILE::GetDirectory() retrieved file: %s", items[iEntry]->m_strPath.c_str());
      }
      return( true);
    }
    else
    {
      CLog::Log(LOGWARNING,"%s: rar lib returned no files in archive %s, likely corrupt",__FUNCTION__,strArchive.c_str());
      return( false );
    }
  }

  bool CRarDirectory::Exists(const char* strPath)
  {
    CFileItemList items;
    if (GetDirectory(strPath,items))
      return true;

    return false;
  }

  bool CRarDirectory::ContainsFiles(const CStdString& strPath)
  {
    CFileItemList items;
    if (g_RarManager.GetFilesInRar(items,strPath))
    {
      if (items.Size() > 1)
        return true;

      return false;
    }

    return false;
  }
}


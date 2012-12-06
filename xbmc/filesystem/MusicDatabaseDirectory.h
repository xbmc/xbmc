#pragma once
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

#include "IDirectory.h"
#include "MusicDatabaseDirectory/DirectoryNode.h"
#include "MusicDatabaseDirectory/QueryParams.h"

namespace XFILE
{
  class CMusicDatabaseDirectory : public IDirectory
  {
  public:
    CMusicDatabaseDirectory(void);
    virtual ~CMusicDatabaseDirectory(void);
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    virtual bool IsAllowed(const CStdString &strFile) const { return true; };
    virtual bool Exists(const char* strPath);
    static MUSICDATABASEDIRECTORY::NODE_TYPE GetDirectoryChildType(const CStdString& strPath);
    static MUSICDATABASEDIRECTORY::NODE_TYPE GetDirectoryType(const CStdString& strPath);
    static MUSICDATABASEDIRECTORY::NODE_TYPE GetDirectoryParentType(const CStdString& strPath);
    bool IsArtistDir(const CStdString& strDirectory);
    bool HasAlbumInfo(const CStdString& strDirectory);
    void ClearDirectoryCache(const CStdString& strDirectory);
    static bool IsAllItem(const CStdString& strDirectory);
    static bool GetLabel(const CStdString& strDirectory, CStdString& strLabel);
    bool ContainsSongs(const CStdString &path);
    static bool CanCache(const CStdString& strPath);
    static CStdString GetIcon(const CStdString& strDirectory);
  };
}

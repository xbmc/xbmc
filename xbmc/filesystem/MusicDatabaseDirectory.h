#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
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
    ~CMusicDatabaseDirectory(void) override;
    bool GetDirectory(const CURL& url, CFileItemList &items) override;
    bool AllowAll() const override { return true; }
    bool Exists(const CURL& url) override;
    static MUSICDATABASEDIRECTORY::NODE_TYPE GetDirectoryChildType(const std::string& strPath);
    static MUSICDATABASEDIRECTORY::NODE_TYPE GetDirectoryType(const std::string& strPath);
    static MUSICDATABASEDIRECTORY::NODE_TYPE GetDirectoryParentType(const std::string& strPath);
    bool IsArtistDir(const std::string& strDirectory);
    void ClearDirectoryCache(const std::string& strDirectory);
    static bool IsAllItem(const std::string& strDirectory);
    static bool GetLabel(const std::string& strDirectory, std::string& strLabel);
    bool ContainsSongs(const std::string &path);
    static bool CanCache(const std::string& strPath);
    static std::string GetIcon(const std::string& strDirectory);
  };
}

#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "IDirectory.h"
#include "VideoDatabaseDirectory/DirectoryNode.h"
#include "VideoDatabaseDirectory/QueryParams.h"

namespace XFILE
{
  class CVideoDatabaseDirectory : public IDirectory
  {
  public:
    CVideoDatabaseDirectory(void);
    virtual ~CVideoDatabaseDirectory(void);
    virtual bool GetDirectory(const CURL& url, CFileItemList &items);
    virtual bool Exists(const CURL& url);
    virtual bool AllowAll() const { return true; }
    static VIDEODATABASEDIRECTORY::NODE_TYPE GetDirectoryChildType(const std::string& strPath);
    static VIDEODATABASEDIRECTORY::NODE_TYPE GetDirectoryType(const std::string& strPath);
    static VIDEODATABASEDIRECTORY::NODE_TYPE GetDirectoryParentType(const std::string& strPath);
    static bool GetQueryParams(const std::string& strPath, VIDEODATABASEDIRECTORY::CQueryParams& params);
    void ClearDirectoryCache(const std::string& strDirectory);
    static bool IsAllItem(const std::string& strDirectory);
    static bool GetLabel(const std::string& strDirectory, std::string& strLabel);
    static std::string GetIcon(const std::string& strDirectory);
    bool ContainsMovies(const std::string &path);
    static bool CanCache(const std::string &path);
  };
}

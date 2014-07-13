#pragma once
/*
 *      Copyright (C) 2011-2013 Team XBMC
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
#include "NFSFile.h"

namespace XFILE
{
  class CNFSDirectory : public IDirectory
  {
    public:
      CNFSDirectory(void);
      virtual ~CNFSDirectory(void);
      virtual bool GetDirectory(const CURL& url, CFileItemList &items);
      virtual DIR_CACHE_TYPE GetCacheType(const CURL& url) const { return DIR_CACHE_ONCE; };
      virtual bool Create(const CURL& url);
      virtual bool Exists(const CURL& url);
      virtual bool Remove(const CURL& url);
    private:
      bool GetServerList(CFileItemList &items);
      bool GetDirectoryFromExportList(const std::string& strPath, CFileItemList &items);
      bool ResolveSymlink( const std::string &dirName, struct nfsdirent *dirent, CURL &resolvedUrl);
  };
}


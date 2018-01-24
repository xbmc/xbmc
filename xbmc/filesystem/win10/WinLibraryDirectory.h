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
#pragma once

#include "filesystem/IDirectory.h"

namespace XFILE
{
  class CWinLibraryDirectory : public IDirectory
  {
  public:
    CWinLibraryDirectory();
    virtual ~CWinLibraryDirectory(void);
    virtual bool GetDirectory(const CURL& url, CFileItemList &items);
    virtual DIR_CACHE_TYPE GetCacheType(const CURL& url) const { return DIR_CACHE_ONCE; };
    virtual bool Create(const CURL& url);
    virtual bool Exists(const CURL& url);
    virtual bool Remove(const CURL& url);

    static bool GetStoragePath(std::string library, std::string& path);
    static bool IsValid(const CURL& url);

    static Windows::Storage::StorageFolder^ GetRootFolder(const CURL& url);
    static Windows::Storage::StorageFolder^ GetFolder(const CURL &url);
  };
}

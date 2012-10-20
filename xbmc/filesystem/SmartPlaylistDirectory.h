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

#include "IFileDirectory.h"
#include "utils/SortUtils.h"

class CSmartPlaylist;

namespace XFILE
{
  class CSmartPlaylistDirectory : public IFileDirectory
  {
  public:
    CSmartPlaylistDirectory();
    ~CSmartPlaylistDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList& items);
    virtual bool IsAllowed(const CStdString &strFile) const { return true; };
    virtual bool ContainsFiles(const CStdString& strPath);
    virtual bool Remove(const char *strPath);

    static bool GetDirectory(const CSmartPlaylist &playlist, CFileItemList& items, const CStdString &strBaseDir = "", bool filter = false);

    static CStdString GetPlaylistByName(const CStdString& name, const CStdString& playlistType);
  };
}

#pragma once
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

#include "IDirectory.h"
#include "Directory.h"

#include <set>

class CFileItem;

namespace DIRECTORY
{
  class CDirectoryCache
  {
    class CDir
    {
    public:
      CDir(const CStdString &strPath, DIR_CACHE_TYPE cacheType);
      virtual ~CDir();
      CStdString m_strPath;
      CFileItemList* m_Items;
      DIR_CACHE_TYPE m_cacheType;
    };
  public:
    CDirectoryCache(void);
    virtual ~CDirectoryCache(void);
    bool GetDirectory(const CStdString& strPath, CFileItemList &items) const;
    void SetDirectory(const CStdString& strPath, const CFileItemList &items, DIR_CACHE_TYPE cacheType);
    void ClearDirectory(const CStdString& strPath);
    void ClearSubPaths(const CStdString& strPath);
    void Clear();
    bool FileExists(const CStdString& strPath, bool& bInCache) const;
    void InitThumbCache();
    void ClearThumbCache();
    void InitMusicThumbCache();
    void ClearMusicThumbCache();
  protected:
    void InitCache(std::set<CStdString>& dirs);
    void ClearCache(std::set<CStdString>& dirs);
    bool IsCacheDir(const CStdString &strPath) const;

    std::vector<CDir*> m_vecCache;
    typedef std::vector<CDir*>::iterator ivecCache;
    typedef std::vector<CDir*>::const_iterator civecCache;

    CCriticalSection m_cs;
    std::set<CStdString> m_thumbDirs;
    std::set<CStdString> m_musicThumbDirs;
    int m_iThumbCacheRefCount;
    int m_iMusicThumbCacheRefCount;
  };
}
extern DIRECTORY::CDirectoryCache g_directoryCache;

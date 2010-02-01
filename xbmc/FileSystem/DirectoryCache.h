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
#include "utils/CriticalSection.h"

#include <map>
#include <set>

class CFileItem;

namespace XFILE
{
  class CDirectoryCache
  {
    class CDir
    {
    public:
      CDir(DIR_CACHE_TYPE cacheType);
      virtual ~CDir();

      void SetLastAccess(unsigned int &accessCounter);
      unsigned int GetLastAccess() const { return m_lastAccess; };

      CFileItemList* m_Items;
      DIR_CACHE_TYPE m_cacheType;
    private:
      unsigned int m_lastAccess;
    };
  public:
    CDirectoryCache(void);
    virtual ~CDirectoryCache(void);
    bool GetDirectory(const CStdString& strPath, CFileItemList &items, bool retrieveAll = false);
    void SetDirectory(const CStdString& strPath, const CFileItemList &items, DIR_CACHE_TYPE cacheType);
    void ClearDirectory(const CStdString& strPath);
    void ClearFile(const CStdString& strFile);
    void ClearSubPaths(const CStdString& strPath);
    void Clear();
    void AddFile(const CStdString& strFile);
    bool FileExists(const CStdString& strPath, bool& bInCache);
    void InitThumbCache();
    void ClearThumbCache();
    void InitMusicThumbCache();
    void ClearMusicThumbCache();
#ifdef _DEBUG
    void PrintStats() const;
#endif
  protected:
    void InitCache(std::set<CStdString>& dirs);
    void ClearCache(std::set<CStdString>& dirs);
    bool IsCacheDir(const CStdString &strPath) const;
    void CheckIfFull();

    std::map<CStdString, CDir*> m_cache;
    typedef std::map<CStdString, CDir*>::iterator iCache;
    typedef std::map<CStdString, CDir*>::const_iterator ciCache;
    void Delete(iCache i);

    CCriticalSection m_cs;
    std::set<CStdString> m_thumbDirs;
    std::set<CStdString> m_musicThumbDirs;
    int m_iThumbCacheRefCount;
    int m_iMusicThumbCacheRefCount;

    unsigned int m_accessCounter;

#ifdef _DEBUG
    unsigned int m_cacheHits;
    unsigned int m_cacheMisses;
#endif
  };
}
extern XFILE::CDirectoryCache g_directoryCache;

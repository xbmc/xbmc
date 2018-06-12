/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "IDirectory.h"
#include "Directory.h"
#include "threads/CriticalSection.h"

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
      explicit CDir(DIR_CACHE_TYPE cacheType);
      virtual ~CDir();

      void SetLastAccess(unsigned int &accessCounter);
      unsigned int GetLastAccess() const { return m_lastAccess; };

      CFileItemList* m_Items;
      DIR_CACHE_TYPE m_cacheType;
    private:
      CDir(const CDir&) = delete;
      CDir& operator=(const CDir&) = delete;
      unsigned int m_lastAccess;
    };
  public:
    CDirectoryCache(void);
    virtual ~CDirectoryCache(void);
    bool GetDirectory(const std::string& strPath, CFileItemList &items, bool retrieveAll = false);
    void SetDirectory(const std::string& strPath, const CFileItemList &items, DIR_CACHE_TYPE cacheType);
    void ClearDirectory(const std::string& strPath);
    void ClearFile(const std::string& strFile);
    void ClearSubPaths(const std::string& strPath);
    void Clear();
    void AddFile(const std::string& strFile);
    bool FileExists(const std::string& strPath, bool& bInCache);
#ifdef _DEBUG
    void PrintStats() const;
#endif
  protected:
    void InitCache(std::set<std::string>& dirs);
    void ClearCache(std::set<std::string>& dirs);
    void CheckIfFull();

    std::map<std::string, CDir*> m_cache;
    typedef std::map<std::string, CDir*>::iterator iCache;
    typedef std::map<std::string, CDir*>::const_iterator ciCache;
    void Delete(iCache i);

    CCriticalSection m_cs;

    unsigned int m_accessCounter;

#ifdef _DEBUG
    unsigned int m_cacheHits;
    unsigned int m_cacheMisses;
#endif
  };
}
extern XFILE::CDirectoryCache g_directoryCache;

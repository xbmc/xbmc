/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"
#include "threads/CriticalSection.h"

#include <map>
#include <memory>
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
      CDir(CDir&& dir) = default;
      CDir& operator=(CDir&& dir) = default;
      virtual ~CDir();

      void SetLastAccess(unsigned int &accessCounter);
      unsigned int GetLastAccess() const { return m_lastAccess; }

      std::unique_ptr<CFileItemList> m_Items;
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
    void InitCache(const std::set<std::string>& dirs);
    void ClearCache(std::set<std::string>& dirs);
    void CheckIfFull();

    std::map<std::string, CDir> m_cache;

    mutable CCriticalSection m_cs;

    unsigned int m_accessCounter;

#ifdef _DEBUG
    unsigned int m_cacheHits;
    unsigned int m_cacheMisses;
#endif
  };
}
extern XFILE::CDirectoryCache g_directoryCache;

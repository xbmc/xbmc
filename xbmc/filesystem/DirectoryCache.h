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

#include <memory>
#include <set>
#include <unordered_map>

class CURL;

namespace XFILE
{
  class CDirectoryCache
  {
    class CDir
    {
    public:
      explicit CDir(CacheType cacheType);
      CDir(CDir&& dir) = default;
      CDir& operator=(CDir&& dir) = default;
      virtual ~CDir();

      void SetLastAccess(unsigned int &accessCounter);
      unsigned int GetLastAccess() const { return m_lastAccess; }

      std::unique_ptr<CFileItemList> m_Items;
      CacheType m_cacheType;

    private:
      CDir(const CDir&) = delete;
      CDir& operator=(const CDir&) = delete;
      unsigned int m_lastAccess;
    };
  public:
    CDirectoryCache(void);
    virtual ~CDirectoryCache(void);
    bool GetDirectory(const CURL& url, CFileItemList& items, bool retrieveAll = false);
    void SetDirectory(const CURL& url, const CFileItemList& items, CacheType cacheType);
    void ClearDirectory(const CURL& url);
    void ClearFile(const CURL& url);
    void ClearSubPaths(const CURL& url);
    void Clear();
    void AddFile(const CURL& url);
    bool FileExists(const CURL& url, bool& foundInCache);
#ifdef _DEBUG
    void PrintStats() const;
#endif
  protected:
    void InitCache(const std::set<std::string>& dirs);
    void ClearCache(std::set<std::string>& dirs);
    void CheckIfFull();

    std::unordered_map<std::string, CDir> m_cache;

    mutable CCriticalSection m_cs;

    unsigned int m_accessCounter;

#ifdef _DEBUG
    unsigned int m_cacheHits;
    unsigned int m_cacheMisses;
#endif
  };
}
extern XFILE::CDirectoryCache g_directoryCache;

/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

/*!
 * \brief Format-agnostic cache for archive file system entries.
 *
 * Manages cache storage, lookup, invalidation (by mtime), and eviction.
 * Thread-safe via CCriticalSection (recursive mutex).
 */
template<typename TEntry>
class CFileSystemCache
{
public:
  /*!
   * \brief Returns true and fills items if path is cached and mtime matches.
   */
  bool GetCachedList(const std::string& path, int64_t currentMtime, std::vector<TEntry>& items)
  {
    std::unique_lock lock(m_lock);
    auto it = m_cache.find(path);
    if (it == m_cache.end())
      return false;

    auto dateIt = m_dates.find(path);
    if (dateIt == m_dates.end() || dateIt->second != currentMtime)
    {
      // mtime changed — invalidate
      m_cache.erase(it);
      if (dateIt != m_dates.end())
        m_dates.erase(dateIt);
      return false;
    }

    items = it->second;
    return true;
  }

  /*!
   * \brief Stores parsed entries in cache.
   */
  void StoreList(const std::string& path, int64_t mtime, std::vector<TEntry> items)
  {
    std::unique_lock lock(m_lock);
    m_cache[path] = std::move(items);
    m_dates[path] = mtime;
  }

  /*!
   * \brief Evicts a path from the cache.
   */
  void Release(const std::string& path)
  {
    std::unique_lock lock(m_lock);
    m_cache.erase(path);
    m_dates.erase(path);
  }

private:
  std::map<std::string, std::vector<TEntry>> m_cache;
  std::map<std::string, int64_t> m_dates;
  mutable CCriticalSection m_lock;
};

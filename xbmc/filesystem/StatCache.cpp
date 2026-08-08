/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "StatCache.h"

#include "URL.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <mutex>

using namespace XFILE;

namespace
{
std::string GetKey(const CURL& url)
{
  // Use the full URL, including options, as the key: for several protocols (eg. http)
  // the options carry the query string which is part of the resource identity and must not be
  // collapsed away (e.g. https://host/media?id=1 vs https://host/media?id=2).
  // Only the path portion is normalized (trailing slash removed): the options are appended
  // afterwards untouched, so a query string that happens to end in '/' isn't corrupted.
  std::string path = url.GetWithoutOptions();
  URIUtils::RemoveSlashAtEnd(path);
  return path + url.GetOptions();
}
} // namespace

bool CStatCache::Get(const CURL& url, struct __stat64* buffer)
{
  std::unique_lock lock(m_cs);

  const std::string key = GetKey(url);

  const auto i = m_cache.find(key);
  if (i == m_cache.end())
    return false;

  CEntry& entry = i->second;
  if (std::chrono::steady_clock::now() - entry.m_time > STAT_CACHE_TTL)
  {
    m_cache.erase(i);
    return false;
  }

  *buffer = entry.m_stat;
  entry.m_lastAccess = m_accessCounter++;
  CLog::LogF(LOGDEBUG, "StatCache hit for {} ({} entries in cache, {} total hits)", key,
             m_cache.size(), m_accessCounter);
  return true;
}

void CStatCache::Set(const CURL& url, const struct __stat64* buffer)
{
  std::unique_lock lock(m_cs);

  const std::string key = GetKey(url);
  m_cache.erase(key);

  CheckIfFull();

  CEntry entry;
  entry.m_stat = *buffer;
  entry.m_time = std::chrono::steady_clock::now();
  entry.m_lastAccess = m_accessCounter++;
  m_cache.emplace(key, entry);
}

void CStatCache::Remove(const CURL& url)
{
  std::unique_lock lock(m_cs);

  const std::string key = GetKey(url);
  m_cache.erase(key);
}

void CStatCache::RemoveRecursive(const CURL& url)
{
  std::unique_lock lock(m_cs);

  // Cache keys carry URL options, and options are a suffix of the URL
  // rather than part of its path hierarchy.
  // Remove options from both sides before comparing so a removed
  // directory's descendants are matched.
  std::string parentPath = url.GetWithoutOptions();
  URIUtils::RemoveSlashAtEnd(parentPath);

  std::erase_if(m_cache,
                [&parentPath](const auto& i)
                {
                  std::string entryPath = CURL(i.first).GetWithoutOptions();
                  URIUtils::RemoveSlashAtEnd(entryPath);
                  return entryPath == parentPath || URIUtils::PathHasParent(entryPath, parentPath);
                });
}

void CStatCache::Clear()
{
  std::unique_lock lock(m_cs);
  m_cache.clear();
}

void CStatCache::CheckIfFull()
{
  // find the least recently accessed entry, and remove it if we're over the limit
  if (m_cache.size() < MAX_CACHED_STATS)
    return;

  auto lastAccessed = m_cache.end();
  for (auto i = m_cache.begin(); i != m_cache.end(); ++i)
  {
    if (lastAccessed == m_cache.end() || i->second.m_lastAccess < lastAccessed->second.m_lastAccess)
      lastAccessed = i;
  }
  if (lastAccessed != m_cache.end())
    m_cache.erase(lastAccessed);
}

/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "PlatformDefs.h"

class CURL;
class TestStatCache;

namespace XFILE
{
// Maximum number of stat results to keep in our cache
static constexpr int MAX_CACHED_STATS{200};

// How long a cached stat result is considered valid for
static constexpr std::chrono::seconds STAT_CACHE_TTL{15};

class CStatCache
{
public:
  CStatCache() = default;
  ~CStatCache() = default;
  CStatCache(const CStatCache&) = delete;
  CStatCache& operator=(const CStatCache&) = delete;

  bool Get(const CURL& url, struct __stat64* buffer);
  void Set(const CURL& url, const struct __stat64* buffer);
  void Remove(const CURL& url);
  // removes the entry for url as well as any entries for paths below it
  void RemoveRecursive(const CURL& url);
  void Clear();

private:
  friend class ::TestStatCache;

  struct StringHash
  {
    using is_transparent = void; // Enables heterogeneous operations.
    std::size_t operator()(std::string_view sv) const
    {
      std::hash<std::string_view> hasher;
      return hasher(sv);
    }
  };

  struct CEntry
  {
    struct __stat64 m_stat;
    std::chrono::steady_clock::time_point m_time;
    uint64_t m_lastAccess{0};
  };

  void CheckIfFull();

  using StatCacheMap = std::unordered_map<std::string, CEntry, StringHash, std::equal_to<>>;
  StatCacheMap m_cache;

  mutable CCriticalSection m_cs;

  uint64_t m_accessCounter{0};
};
} // namespace XFILE

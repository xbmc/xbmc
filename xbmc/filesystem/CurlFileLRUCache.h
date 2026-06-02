/*
 *  Copyright (C) 2025 Team CoreELEC
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * Global LRU block cache for HTTP ISO streaming.
 *
 * Caches raw HTTP bytes keyed by (url, mod_time, block_num).
 * Block size and total capacity are configurable via advancedsettings.xml
 * (<curlfurlrucache><blocksize> / <maxbytes>), defaults to 1MB / 128MB.
 * LRU eviction when capacity is exceeded.
 *
 * Thread-safe: uses a global mutex.  Block data is returned via shared_ptr
 * so callers can safely read while the cache mutex is released.
 */
class CCurlFileLRUCache
{
public:
  static CCurlFileLRUCache& Instance();

  /** Look up a block. Returns nullptr on miss. */
  std::shared_ptr<std::vector<uint8_t>> Get(const std::string& url,
                                             int64_t modTime,
                                             int64_t blockNum);

  /** Insert a block. May evict the LRU entry if cache is full. */
  void Put(const std::string& url,
           int64_t modTime,
           int64_t blockNum,
           const uint8_t* data,
           size_t size);

  /** Remove all blocks belonging to an older version of the given URL. */
  size_t InvalidateUrlVersion(const std::string& url, int64_t modTime);

  /** Configure block size and total capacity (triggers full clear on size change). */
  void SetLimits(size_t blockSize, size_t totalSize);

  /** Clear everything. */
  void Clear();

  /** Remove all blocks belonging to a specific URL (e.g. on video close). */
  size_t EvictUrl(const std::string& url);

private:
  CCurlFileLRUCache() = default;
  ~CCurlFileLRUCache() = default;
  CCurlFileLRUCache(const CCurlFileLRUCache&) = delete;
  CCurlFileLRUCache& operator=(const CCurlFileLRUCache&) = delete;

  struct Key
  {
    std::string url;
    int64_t modTime{0};
    int64_t blockNum{0};

    bool operator==(const Key& o) const;
  };

  struct KeyHash
  {
    size_t operator()(const Key& k) const;
  };

  std::mutex m_mutex;
  std::list<Key> m_lruOrder;
  std::unordered_map<Key,
                     std::pair<std::list<Key>::iterator, std::shared_ptr<std::vector<uint8_t>>>,
                     KeyHash> m_blocks;

  size_t m_blockSize{1024 * 1024};
  size_t m_maxBlocks{64};
};

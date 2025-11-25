/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <chrono>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace KODI
{
namespace SEMANTIC
{

/*!
 * @brief LRU (Least Recently Used) cache for query results
 *
 * Thread-safe cache implementation with automatic eviction of least
 * recently used entries when capacity is reached.
 *
 * Template parameters:
 * - KeyType: Type of the cache key (typically std::string for queries)
 * - ValueType: Type of the cached value (search results, embeddings, etc.)
 */
template<typename KeyType, typename ValueType>
class CLRUCache
{
public:
  /*!
   * @brief Constructor
   * @param maxSize Maximum number of entries to cache
   * @param ttlSeconds Time-to-live for cache entries in seconds (0 = no expiration)
   */
  explicit CLRUCache(size_t maxSize, int ttlSeconds = 0)
    : m_maxSize(maxSize), m_ttlSeconds(ttlSeconds)
  {
  }

  /*!
   * @brief Insert or update a cache entry
   * @param key Cache key
   * @param value Value to cache
   */
  void Put(const KeyType& key, const ValueType& value)
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cache.find(key);

    // Remove old entry from LRU list if it exists
    if (it != m_cache.end())
    {
      m_lruList.erase(it->second.listIt);
      m_cache.erase(it);
    }

    // Add new entry to front of LRU list
    m_lruList.push_front({key, value, std::chrono::steady_clock::now()});
    m_cache[key] = {value, m_lruList.begin()};

    // Evict oldest entry if capacity exceeded
    if (m_cache.size() > m_maxSize)
    {
      auto last = m_lruList.back();
      m_cache.erase(last.key);
      m_lruList.pop_back();
    }
  }

  /*!
   * @brief Get a value from the cache
   * @param key Cache key
   * @return Optional value (empty if not found or expired)
   */
  std::optional<ValueType> Get(const KeyType& key)
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cache.find(key);
    if (it == m_cache.end())
    {
      return std::nullopt;
    }

    // Check if entry has expired
    if (m_ttlSeconds > 0)
    {
      auto& entry = *it->second.listIt;
      auto age = std::chrono::duration_cast<std::chrono::seconds>(
                     std::chrono::steady_clock::now() - entry.timestamp)
                     .count();

      if (age > m_ttlSeconds)
      {
        // Entry expired, remove it
        m_lruList.erase(it->second.listIt);
        m_cache.erase(it);
        return std::nullopt;
      }
    }

    // Move accessed entry to front of LRU list
    auto& cacheEntry = it->second;
    m_lruList.splice(m_lruList.begin(), m_lruList, cacheEntry.listIt);

    return cacheEntry.value;
  }

  /*!
   * @brief Check if a key exists in the cache
   * @param key Cache key
   * @return true if key exists and is not expired
   */
  bool Contains(const KeyType& key)
  {
    return Get(key).has_value();
  }

  /*!
   * @brief Remove a specific entry from the cache
   * @param key Cache key to remove
   */
  void Remove(const KeyType& key)
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cache.find(key);
    if (it != m_cache.end())
    {
      m_lruList.erase(it->second.listIt);
      m_cache.erase(it);
    }
  }

  /*!
   * @brief Clear all entries from the cache
   */
  void Clear()
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
    m_lruList.clear();
  }

  /*!
   * @brief Get current cache size
   * @return Number of entries in cache
   */
  size_t Size() const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cache.size();
  }

  /*!
   * @brief Get maximum cache capacity
   * @return Maximum number of entries
   */
  size_t Capacity() const { return m_maxSize; }

  /*!
   * @brief Get cache memory usage estimate
   * @return Estimated memory usage in bytes
   */
  size_t GetMemoryUsage() const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    // Rough estimate: size * (key size + value size + overhead)
    return m_cache.size() * (sizeof(KeyType) + sizeof(ValueType) + 64);
  }

  /*!
   * @brief Update cache capacity
   * @param newMaxSize New maximum size
   */
  void SetCapacity(size_t newMaxSize)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_maxSize = newMaxSize;

    // Evict entries if new size is smaller
    while (m_cache.size() > m_maxSize)
    {
      auto last = m_lruList.back();
      m_cache.erase(last.key);
      m_lruList.pop_back();
    }
  }

  /*!
   * @brief Update TTL setting
   * @param ttlSeconds New TTL in seconds (0 = no expiration)
   */
  void SetTTL(int ttlSeconds) { m_ttlSeconds = ttlSeconds; }

private:
  struct LRUNode
  {
    KeyType key;
    ValueType value;
    std::chrono::steady_clock::time_point timestamp;
  };

  struct CacheEntry
  {
    ValueType value;
    typename std::list<LRUNode>::iterator listIt;
  };

  size_t m_maxSize;
  int m_ttlSeconds;

  mutable std::mutex m_mutex;
  std::unordered_map<KeyType, CacheEntry> m_cache;
  std::list<LRUNode> m_lruList;
};

/*!
 * @brief Query cache manager for semantic search
 *
 * Provides specialized caching for different types of queries:
 * - Search query results
 * - Generated embeddings
 * - Database query results
 */
class CQueryCache
{
public:
  CQueryCache();
  ~CQueryCache();

  /*!
   * @brief Initialize the query cache
   * @param maxSizeMB Maximum cache size in megabytes
   * @param resultTTL Time-to-live for search results in seconds
   * @param embeddingTTL Time-to-live for embeddings in seconds
   * @return true if initialization succeeded
   */
  bool Initialize(size_t maxSizeMB = 50, int resultTTL = 300, int embeddingTTL = 3600);

  /*!
   * @brief Enable or disable the cache
   * @param enabled true to enable, false to disable
   */
  void SetEnabled(bool enabled) { m_enabled = enabled; }

  /*!
   * @brief Check if cache is enabled
   * @return true if cache is enabled
   */
  bool IsEnabled() const { return m_enabled; }

  // ===== Search Result Caching =====

  /*!
   * @brief Cache search results
   * @param query Search query string
   * @param results Search results to cache
   */
  template<typename ResultType>
  void PutSearchResults(const std::string& query, const ResultType& results)
  {
    if (!m_enabled)
      return;

    m_searchCache.Put(query, results);
    UpdateMemoryStats();
  }

  /*!
   * @brief Get cached search results
   * @param query Search query string
   * @return Optional search results (empty if not found)
   */
  template<typename ResultType>
  std::optional<ResultType> GetSearchResults(const std::string& query)
  {
    if (!m_enabled)
      return std::nullopt;

    return m_searchCache.Get(query);
  }

  // ===== Embedding Caching =====

  /*!
   * @brief Cache an embedding vector
   * @param text Input text
   * @param embedding Generated embedding
   */
  template<typename EmbeddingType>
  void PutEmbedding(const std::string& text, const EmbeddingType& embedding)
  {
    if (!m_enabled)
      return;

    m_embeddingCache.Put(text, embedding);
    UpdateMemoryStats();
  }

  /*!
   * @brief Get cached embedding
   * @param text Input text
   * @return Optional embedding (empty if not found)
   */
  template<typename EmbeddingType>
  std::optional<EmbeddingType> GetEmbedding(const std::string& text)
  {
    if (!m_enabled)
      return std::nullopt;

    return m_embeddingCache.Get(text);
  }

  // ===== Cache Management =====

  /*!
   * @brief Clear all caches
   */
  void Clear();

  /*!
   * @brief Clear only search result cache
   */
  void ClearSearchCache();

  /*!
   * @brief Clear only embedding cache
   */
  void ClearEmbeddingCache();

  /*!
   * @brief Get total cache memory usage
   * @return Estimated memory usage in bytes
   */
  size_t GetMemoryUsage() const;

  /*!
   * @brief Get cache statistics
   * @return Map of cache name to size
   */
  std::map<std::string, size_t> GetStats() const;

private:
  void UpdateMemoryStats();

  bool m_enabled{false};
  size_t m_maxSizeBytes{0};

  // Separate caches for different data types
  // Note: Using std::string as placeholder for actual result types
  CLRUCache<std::string, std::string> m_searchCache{100, 300};
  CLRUCache<std::string, std::string> m_embeddingCache{1000, 3600};

  mutable std::mutex m_mutex;
};

} // namespace SEMANTIC
} // namespace KODI

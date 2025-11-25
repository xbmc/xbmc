/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "QueryCache.h"

#include "PerformanceMonitor.h"
#include "utils/log.h"

using namespace KODI::SEMANTIC;

CQueryCache::CQueryCache() = default;

CQueryCache::~CQueryCache() = default;

bool CQueryCache::Initialize(size_t maxSizeMB, int resultTTL, int embeddingTTL)
{
  m_maxSizeBytes = maxSizeMB * 1024 * 1024;

  // Calculate capacity for each cache based on estimated sizes
  // Search results: ~10KB average
  // Embeddings: ~1.5KB (384 floats)
  size_t searchCacheCapacity = (m_maxSizeBytes * 60 / 100) / 10240; // 60% for search results
  size_t embeddingCacheCapacity = (m_maxSizeBytes * 40 / 100) / 1536; // 40% for embeddings

  m_searchCache.SetCapacity(searchCacheCapacity);
  m_searchCache.SetTTL(resultTTL);

  m_embeddingCache.SetCapacity(embeddingCacheCapacity);
  m_embeddingCache.SetTTL(embeddingTTL);

  m_enabled = true;

  CLog::Log(LOGINFO,
            "QueryCache: Initialized with maxSize={}MB, searchCapacity={}, embeddingCapacity={}",
            maxSizeMB, searchCacheCapacity, embeddingCacheCapacity);

  return true;
}

void CQueryCache::Clear()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_searchCache.Clear();
  m_embeddingCache.Clear();

  UpdateMemoryStats();
  CLog::Log(LOGINFO, "QueryCache: All caches cleared");
}

void CQueryCache::ClearSearchCache()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_searchCache.Clear();
  UpdateMemoryStats();

  CLog::Log(LOGINFO, "QueryCache: Search cache cleared");
}

void CQueryCache::ClearEmbeddingCache()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_embeddingCache.Clear();
  UpdateMemoryStats();

  CLog::Log(LOGINFO, "QueryCache: Embedding cache cleared");
}

size_t CQueryCache::GetMemoryUsage() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_searchCache.GetMemoryUsage() + m_embeddingCache.GetMemoryUsage();
}

std::map<std::string, size_t> CQueryCache::GetStats() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  std::map<std::string, size_t> stats;
  stats["searchCacheSize"] = m_searchCache.Size();
  stats["searchCacheCapacity"] = m_searchCache.Capacity();
  stats["embeddingCacheSize"] = m_embeddingCache.Size();
  stats["embeddingCacheCapacity"] = m_embeddingCache.Capacity();
  stats["totalMemoryBytes"] = GetMemoryUsage();

  return stats;
}

void CQueryCache::UpdateMemoryStats()
{
  // Update performance monitor with current cache memory usage
  auto& perfMon = CPerformanceMonitor::GetInstance();
  if (perfMon.IsEnabled())
  {
    size_t memUsage = m_searchCache.GetMemoryUsage() + m_embeddingCache.GetMemoryUsage();
    perfMon.UpdateMemoryUsage(memUsage, "cache");
    perfMon.UpdateCacheStats(m_searchCache.Size() + m_embeddingCache.Size(),
                             m_searchCache.Capacity() + m_embeddingCache.Capacity());
  }
}

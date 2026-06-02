/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BlurayIsoCache.h"

#include "utils/log.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace
{
constexpr const char* LOG_TAG = "CBlurayIsoCache";
}

CBlurayIsoCache::CBlurayIsoCache(int64_t sourceLength, ReadCallback readCallback, Config config)
  : m_config(std::move(config)), m_readCallback(std::move(readCallback)), m_sourceLength(sourceLength)
{
  if (m_config.pageSize < BLOCK_SIZE)
    m_config.pageSize = BLOCK_SIZE;

  const size_t remainder = m_config.pageSize % BLOCK_SIZE;
  if (remainder != 0)
    m_config.pageSize += BLOCK_SIZE - remainder;

  if (m_config.maxBytes < m_config.pageSize)
    m_config.maxBytes = m_config.pageSize;

  m_maxPages = std::max<size_t>(1, m_config.maxBytes / m_config.pageSize);
}

CBlurayIsoCache::~CBlurayIsoCache()
{
  Stop();
}

void CBlurayIsoCache::Start()
{
  Stop();

  if (m_sourceLength <= 0 || !m_readCallback)
  {
    CLog::Log(LOGDEBUG, "{}::{} - disable cache, invalid source length {}", LOG_TAG, __FUNCTION__,
              m_sourceLength);
    return;
  }

  m_started = true;

  CLog::Log(LOGDEBUG,
            "{}::{} - cache config pageSize={} maxBytes={} maxPages={}",
            LOG_TAG, __FUNCTION__, m_config.pageSize, m_config.maxBytes, m_maxPages);

  CLog::Log(LOGINFO, "{}::{} - Bluray ISO cache started for {} bytes source",
            LOG_TAG, __FUNCTION__, m_sourceLength);

  m_prefetchRunning = true;
  m_prefetchThread = std::thread(&CBlurayIsoCache::PrefetchWorker, this);
}

void CBlurayIsoCache::Stop()
{
  if (m_prefetchRunning.exchange(false))
  {
    {
      std::lock_guard<std::mutex> lk(m_prefetchMutex);
      m_prefetchPending.clear();
    }
    m_prefetchCv.notify_one();
    if (m_prefetchThread.joinable())
      m_prefetchThread.join();
  }

  if (m_started)
    LogStats("stop");

  {
    std::lock_guard<std::mutex> cacheLock(m_cacheMutex);
    m_pages.clear();
    m_lruOrder.clear();
  }

  m_started = false;
}

int CBlurayIsoCache::ReadBlocks(uint8_t* buffer, int lba, int numBlocks)
{
  if (!buffer || lba < 0 || numBlocks <= 0)
    return -1;

  ++m_readRequests;
  m_requestedBlocks += static_cast<uint64_t>(numBlocks);

  const int64_t offset = static_cast<int64_t>(lba) * BLOCK_SIZE;
  const int64_t requestedBytes = static_cast<int64_t>(numBlocks) * BLOCK_SIZE;
  if (offset < 0 || requestedBytes <= 0 || offset >= m_sourceLength)
    return -1;

  const int64_t availableBytes = std::min<int64_t>(requestedBytes, m_sourceLength - offset);

  const int64_t firstPage = offset / static_cast<int64_t>(m_config.pageSize);
  const int64_t lastPage = (offset + availableBytes - 1) / static_cast<int64_t>(m_config.pageSize);

  int64_t copied = 0;
  for (int64_t pageIndex = firstPage; pageIndex <= lastPage; ++pageIndex)
  {
    PagePtr page = GetPage(pageIndex);
    if (!page)
    {
      ++m_pageMisses;
      page = LoadPage(pageIndex);
    }
    else
      ++m_pageHits;

    if (!page)
      return copied > 0 ? static_cast<int>(copied / BLOCK_SIZE) : -1;

    const int64_t pageStart = pageIndex * static_cast<int64_t>(m_config.pageSize);
    const int64_t copyStart = std::max<int64_t>(offset, pageStart);
    const int64_t copyEnd = std::min<int64_t>(offset + availableBytes,
                                              pageStart + static_cast<int64_t>(page->validBytes));
    if (copyEnd <= copyStart)
      break;

    const size_t pageOffset = static_cast<size_t>(copyStart - pageStart);
    const size_t chunk = static_cast<size_t>(copyEnd - copyStart);
    std::memcpy(buffer + copied, page->data.data() + pageOffset, chunk);
    copied += static_cast<int64_t>(chunk);
  }

  if (copied <= 0)
    return -1;

  if (m_prefetchRunning.load(std::memory_order_acquire))
  {
    const int64_t pfStart = lastPage + 1;
    const int64_t pfEnd = std::min<int64_t>(lastPage + BURST_AHEAD, GetMaxPageIndex());
    if (pfStart <= pfEnd)
      SchedulePrefetch(pfStart, pfEnd);
  }

  return static_cast<int>(copied / BLOCK_SIZE);
}

CBlurayIsoCache::PagePtr CBlurayIsoCache::GetPage(int64_t pageIndex)
{
  std::lock_guard<std::mutex> lock(m_cacheMutex);
  auto it = m_pages.find(pageIndex);
  if (it == m_pages.end())
    return nullptr;

  TouchPageUnlocked(it);
  return it->second.page;
}

CBlurayIsoCache::PagePtr CBlurayIsoCache::LoadPage(int64_t pageIndex)
{
  if (!IsValidPageIndex(pageIndex) || !m_readCallback)
    return nullptr;

  auto page = std::make_shared<Page>();
  page->data.resize(m_config.pageSize);

  const int64_t offset = pageIndex * static_cast<int64_t>(m_config.pageSize);
  const size_t bytesToRead = static_cast<size_t>(
      std::min<int64_t>(static_cast<int64_t>(m_config.pageSize), m_sourceLength - offset));
  const int64_t bytesRead = m_readCallback(offset, page->data.data(), bytesToRead);
  if (bytesRead <= 0)
    return nullptr;

  if (static_cast<size_t>(bytesRead) > bytesToRead)
  {
    CLog::Log(LOGERROR, "{}::{} - callback returned {} bytes, expected at most {}",
              LOG_TAG, __FUNCTION__, bytesRead, bytesToRead);
    return nullptr;
  }

  page->validBytes = static_cast<size_t>(bytesRead);
  InsertPage(pageIndex, page);
  ++m_syncPageLoads;

  return page;
}

void CBlurayIsoCache::InsertPage(int64_t pageIndex, PagePtr page)
{
  if (!page)
    return;

  std::lock_guard<std::mutex> lock(m_cacheMutex);
  auto it = m_pages.find(pageIndex);
  if (it != m_pages.end())
  {
    it->second.page = std::move(page);
    TouchPageUnlocked(it);
    return;
  }

  m_lruOrder.push_front(pageIndex);
  m_pages.emplace(pageIndex, CacheSlot{m_lruOrder.begin(), std::move(page)});
  TrimUnlocked();
}

void CBlurayIsoCache::TouchPageUnlocked(std::unordered_map<int64_t, CacheSlot>::iterator it)
{
  if (it->second.lruIt != m_lruOrder.begin())
    m_lruOrder.splice(m_lruOrder.begin(), m_lruOrder, it->second.lruIt);
  it->second.lruIt = m_lruOrder.begin();
}

void CBlurayIsoCache::TrimUnlocked()
{
  while (m_pages.size() > m_maxPages && !m_lruOrder.empty())
  {
    const int64_t pageIndex = m_lruOrder.back();
    m_lruOrder.pop_back();
    m_pages.erase(pageIndex);
    ++m_evictions;
  }
}

void CBlurayIsoCache::LogStats(const char* reason)
{
  size_t cachedPages = 0;
  {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    cachedPages = m_pages.size();
  }

  CLog::Log(LOGDEBUG,
            "{}::{} - {} detail: reads={} reqBlocks={} pageHits={} misses={} syncLoads={} evicts={} prefetchLoads={} pages={}",
            LOG_TAG, __FUNCTION__, reason, m_readRequests.load(), m_requestedBlocks.load(),
            m_pageHits.load(), m_pageMisses.load(), m_syncPageLoads.load(),
            m_evictions.load(), m_prefetchPageLoads.load(), cachedPages);

  CLog::Log(LOGINFO,
            "{}::{} - {}: {} reads, {} pageHits / {} pageMisses, {} prefetchLoads, {} evicts",
            LOG_TAG, __FUNCTION__, reason,
            m_readRequests.load(), m_pageHits.load(), m_pageMisses.load(),
            m_prefetchPageLoads.load(), m_evictions.load());
}

bool CBlurayIsoCache::IsValidPageIndex(int64_t pageIndex) const
{
  return pageIndex >= 0 && pageIndex <= GetMaxPageIndex();
}

int64_t CBlurayIsoCache::GetMaxPageIndex() const
{
  if (m_sourceLength <= 0)
    return -1;
  return (m_sourceLength - 1) / static_cast<int64_t>(m_config.pageSize);
}

void CBlurayIsoCache::SchedulePrefetch(int64_t fromPage, int64_t toPage)
{
  std::lock_guard<std::mutex> lk(m_prefetchMutex);
  for (int64_t p = fromPage; p <= toPage; ++p)
  {
    if (!IsValidPageIndex(p))
      break;

    {
      std::lock_guard<std::mutex> clk(m_cacheMutex);
      if (m_pages.count(p))
        continue;
    }
    m_prefetchPending.insert(p);
  }
  if (!m_prefetchPending.empty())
    m_prefetchCv.notify_one();
}

void CBlurayIsoCache::PrefetchWorker()
{
  CLog::Log(LOGDEBUG, "{}::PrefetchWorker - started", LOG_TAG);

  while (m_prefetchRunning.load(std::memory_order_acquire))
  {
    int64_t pageIndex = -1;
    {
      std::unique_lock<std::mutex> lk(m_prefetchMutex);
      m_prefetchCv.wait(lk, [this] {
        return !m_prefetchPending.empty() || !m_prefetchRunning.load(std::memory_order_acquire);
      });

      if (!m_prefetchRunning.load(std::memory_order_acquire))
        break;

      if (m_prefetchPending.empty())
        continue;

      auto it = m_prefetchPending.begin();
      pageIndex = *it;
      m_prefetchPending.erase(it);
    }

    if (pageIndex < 0)
      continue;

    if (GetPage(pageIndex))
      continue;

    auto page = LoadPage(pageIndex);
    if (page)
      ++m_prefetchPageLoads;
  }

  CLog::Log(LOGDEBUG, "{}::PrefetchWorker - stopped", LOG_TAG);
}

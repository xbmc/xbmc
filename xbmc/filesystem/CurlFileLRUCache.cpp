/*
 *  Copyright (C) 2025 Team CoreELEC
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CurlFileLRUCache.h"

bool CCurlFileLRUCache::Key::operator==(const Key& o) const
{
  return blockNum == o.blockNum && modTime == o.modTime && url == o.url;
}

size_t CCurlFileLRUCache::KeyHash::operator()(const Key& k) const
{
  size_t h1 = std::hash<std::string>{}(k.url);
  size_t h2 = std::hash<int64_t>{}(k.modTime);
  size_t h3 = std::hash<int64_t>{}(k.blockNum);
  return h1 ^ (h2 * 2654435761ULL) ^ (h3 * 11400714819323198485ULL);
}

CCurlFileLRUCache& CCurlFileLRUCache::Instance()
{
  static CCurlFileLRUCache cache;
  return cache;
}

std::shared_ptr<std::vector<uint8_t>> CCurlFileLRUCache::Get(const std::string& url,
                                                              int64_t modTime,
                                                              int64_t blockNum)
{
  Key key{url, modTime, blockNum};
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_blocks.find(key);
  if (it == m_blocks.end())
    return nullptr;
  m_lruOrder.splice(m_lruOrder.begin(), m_lruOrder, it->second.first);
  return it->second.second;
}

void CCurlFileLRUCache::Put(const std::string& url,
                             int64_t modTime,
                             int64_t blockNum,
                             const uint8_t* data,
                             size_t size)
{
  Key key{url, modTime, blockNum};
  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_blocks.find(key);
  if (it != m_blocks.end())
  {
    it->second.second = std::make_shared<std::vector<uint8_t>>(data, data + size);
    m_lruOrder.splice(m_lruOrder.begin(), m_lruOrder, it->second.first);
    return;
  }

  while (m_blocks.size() >= m_maxBlocks && !m_lruOrder.empty())
  {
    m_blocks.erase(m_lruOrder.back());
    m_lruOrder.pop_back();
  }

  m_lruOrder.push_front(key);
  m_blocks[key] = {m_lruOrder.begin(), std::make_shared<std::vector<uint8_t>>(data, data + size)};
}

size_t CCurlFileLRUCache::InvalidateUrlVersion(const std::string& url, int64_t modTime)
{
  size_t removed = 0;
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto it = m_lruOrder.begin(); it != m_lruOrder.end();)
  {
    if (it->url == url && it->modTime != modTime)
    {
      m_blocks.erase(*it);
      it = m_lruOrder.erase(it);
      ++removed;
    }
    else
    {
      ++it;
    }
  }
  return removed;
}

void CCurlFileLRUCache::SetLimits(size_t blockSize, size_t totalSize)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (blockSize == 0)
    blockSize = 1024 * 1024;
  if (m_blockSize != blockSize)
  {
    m_lruOrder.clear();
    m_blocks.clear();
  }
  m_blockSize = blockSize;
  m_maxBlocks = totalSize / m_blockSize;
  if (m_maxBlocks == 0)
    m_maxBlocks = 1;
  while (m_blocks.size() > m_maxBlocks && !m_lruOrder.empty())
  {
    m_blocks.erase(m_lruOrder.back());
    m_lruOrder.pop_back();
  }
}

void CCurlFileLRUCache::Clear()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_lruOrder.clear();
  m_blocks.clear();
}

size_t CCurlFileLRUCache::EvictUrl(const std::string& url)
{
  size_t removed = 0;
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto it = m_lruOrder.begin(); it != m_lruOrder.end();)
  {
    if (it->url == url)
    {
      m_blocks.erase(*it);
      it = m_lruOrder.erase(it);
      ++removed;
    }
    else
    {
      ++it;
    }
  }
  return removed;
}

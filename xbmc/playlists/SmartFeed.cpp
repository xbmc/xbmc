/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "playlists/SmartFeed.h"

#include "utils/Random.h"

#include <algorithm>
#include <iterator>
#include <mutex>
#include <numeric>
#include <utility>

namespace KODI::PLAYLIST
{

CSmartFeed::CSmartFeed(int total, Fetch fetch)
  : m_fetch(std::move(fetch)),
    m_pool(static_cast<size_t>(std::max(total, 0)))
{
  std::iota(m_pool.begin(), m_pool.end(), 0);
  KODI::UTILS::RandomShuffle(m_pool.begin(), m_pool.end());
}

std::vector<std::shared_ptr<CFileItem>> CSmartFeed::Take(int count)
{
  const size_t wanted = static_cast<size_t>(std::max(count, 0));
  std::vector<std::shared_ptr<CFileItem>> items;
  // A fetch leaves out matches that no longer exist, so more are claimed to make up the count
  while (items.size() < wanted)
  {
    std::vector<int> slice;
    {
      std::unique_lock lock(m_critSection);
      const size_t end = std::min(m_pool.size(), m_next + wanted - items.size());
      slice.assign(m_pool.begin() + m_next, m_pool.begin() + end);
      m_next = end;
    }
    if (slice.empty())
      break;
    std::vector<std::shared_ptr<CFileItem>> fetched = m_fetch(slice);
    items.insert(items.end(), std::make_move_iterator(fetched.begin()),
                 std::make_move_iterator(fetched.end()));
  }
  return items;
}

void CSmartFeed::Restart()
{
  std::unique_lock lock(m_critSection);
  KODI::UTILS::RandomShuffle(m_pool.begin(), m_pool.end());
  m_next = 0;
}

int CSmartFeed::GetTotal() const
{
  std::unique_lock lock(m_critSection);
  return static_cast<int>(m_pool.size());
}

int CSmartFeed::GetLeft() const
{
  std::unique_lock lock(m_critSection);
  return static_cast<int>(m_pool.size() - m_next);
}

} // namespace KODI::PLAYLIST

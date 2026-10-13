/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "playlists/PlayListFeed.h"
#include "threads/CriticalSection.h"

#include <functional>
#include <memory>
#include <vector>

class CFileItem;

namespace KODI::PLAYLIST
{
/*!
 * \brief A feed that deals a pool of matches in random order and has its creator fetch the items
 * as they are taken. A match is an index into the creator's own list of what matched; the feed
 * never learns what it names.
 */
class CSmartFeed final : public IFeed
{
public:
  /*!
   * \brief Fetch the items a slice of matches names, in the slice's order, leaving out any that
   * no longer exist. Called without the feed's lock held.
   */
  using Fetch = std::function<std::vector<std::shared_ptr<CFileItem>>(const std::vector<int>&)>;

  //! Deal matches 0 to total - 1.
  CSmartFeed(int total, Fetch fetch);

  std::vector<std::shared_ptr<CFileItem>> Take(int count) override;
  void Restart() override;
  int GetTotal() const override;
  int GetLeft() const override;

private:
  mutable CCriticalSection m_critSection;
  const Fetch m_fetch;
  std::vector<int> m_pool;
  //! The next pool entry to hand over.
  size_t m_next{0};
};
} // namespace KODI::PLAYLIST

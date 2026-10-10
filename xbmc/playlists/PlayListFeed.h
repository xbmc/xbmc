/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <vector>

class CFileItem;

namespace KODI::PLAYLIST
{

/*!
 * \brief Items not yet placed on a playlist, handed over a few at a time in the order they are
 * placed, for a playlist that plays more than it is sensible to place at once.
 *
 * A feed knows nothing of entries, the cursor or the shuffle. Taking may be slow, so it is never
 * called with a playlist's lock held.
 */
class IFeed
{
public:
  virtual ~IFeed() = default;

  /*!
   * \return Up to count items, in placing order; fewer once the feed has run out.
   */
  virtual std::vector<std::shared_ptr<CFileItem>> Take(int count) = 0;

  /*!
   * \brief Deal what the feed held again, for a playlist that wraps.
   */
  virtual void Restart() = 0;

  //! How many items the feed held.
  virtual int GetTotal() const = 0;

  //! How many it has not handed over yet.
  virtual int GetLeft() const = 0;
};

} // namespace KODI::PLAYLIST

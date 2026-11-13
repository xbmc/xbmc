/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PlayListTypes.h"

#include <random>
#include <unordered_map>
#include <vector>

namespace KODI::PLAYLIST
{

/*!
 * \brief Decides the play order of a playlist without reordering it.
 *
 * It is told of every change to the list, by entry id, and answers what follows and what
 * precedes an entry. Whether a change needs acting on is the routine's business.
 */
class IPlayListShuffle
{
public:
  virtual ~IPlayListShuffle() = default;

  /*!
   * \brief Start over from the whole list.
   * \param listOrder Every entry, in list order.
   * \param current The current entry, or NO_ENTRY.
   */
  virtual void Reset(const std::vector<EntryId>& listOrder, EntryId current) = 0;

  //! An entry was inserted at a position in list order; current is the current entry.
  virtual void OnAdded(EntryId entry, int position, EntryId current) = 0;
  virtual void OnRemoved(EntryId entry) = 0;
  //! An entry now stands at this position in list order.
  virtual void OnMoved(EntryId entry, int position) = 0;

  /*!
   * \brief Play an entry straight after another, as a request to play it next does.
   * \param after The entry it follows, or NO_ENTRY to lead the order.
   */
  virtual void PlayAfter(EntryId entry, EntryId after) = 0;

  /*!
   * \return The entry after the given one in play order, or NO_ENTRY after the last. Given
   * NO_ENTRY, the first entry.
   */
  virtual EntryId Following(EntryId entry) const = 0;

  /*!
   * \return The entry before the given one in play order, or NO_ENTRY before the first. Given
   * NO_ENTRY, the last entry.
   */
  virtual EntryId Preceding(EntryId entry) const = 0;

  //! Every entry, in play order.
  virtual std::vector<EntryId> GetOrder() const = 0;
  //! An entry's place in play order, from 0, or -1.
  virtual int GetOrderPosition(EntryId entry) const = 0;

  /*!
   * \return Whether this plays the list in the order it was built, which is what "not shuffled"
   * means.
   */
  virtual bool IsListOrder() const = 0;
};

/*!
 * \brief A routine that keeps its play order as a sequence of entry ids.
 */
class CPlayListStoredOrder : public IPlayListShuffle
{
public:
  void OnRemoved(EntryId entry) override;
  void PlayAfter(EntryId entry, EntryId after) override;
  EntryId Following(EntryId entry) const override;
  EntryId Preceding(EntryId entry) const override;
  std::vector<EntryId> GetOrder() const override { return m_order; }
  int GetOrderPosition(EntryId entry) const override;

protected:
  std::vector<EntryId> m_order;

private:
  int IndexOf(EntryId entry) const;

  //! Where each entry was when last looked for; checked on use and rebuilt when out of date.
  mutable std::unordered_map<EntryId, int> m_positions;
};

/*!
 * \brief Plays the list in the order it was built.
 */
class CPlayListNoShuffle : public CPlayListStoredOrder
{
public:
  void Reset(const std::vector<EntryId>& listOrder, EntryId current) override;
  void OnAdded(EntryId entry, int position, EntryId current) override;
  void OnMoved(EntryId entry, int position) override;
  bool IsListOrder() const override { return true; }
};

/*!
 * \brief Plays the list in a uniformly random order. The current entry leads the order it
 * shuffles, and entries added later are placed at random among those still to come.
 */
class CPlayListRandomShuffle : public CPlayListStoredOrder
{
public:
  CPlayListRandomShuffle();

  void Reset(const std::vector<EntryId>& listOrder, EntryId current) override;
  void OnAdded(EntryId entry, int position, EntryId current) override;
  void OnMoved(EntryId entry, int position) override {}
  bool IsListOrder() const override { return false; }

private:
  std::mt19937 m_random;
};

} // namespace KODI::PLAYLIST

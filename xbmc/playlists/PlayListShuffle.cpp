/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayListShuffle.h"

#include <algorithm>

namespace KODI::PLAYLIST
{

void CPlayListStoredOrder::OnRemoved(EntryId entry)
{
  std::erase(m_order, entry);
}

void CPlayListStoredOrder::PlayAfter(EntryId entry, EntryId after)
{
  std::erase(m_order, entry);
  const auto it = std::ranges::find(m_order, after);
  m_order.insert(it == m_order.end() ? m_order.begin() : std::next(it), entry);
}

int CPlayListStoredOrder::GetOrderPosition(EntryId entry) const
{
  return IndexOf(entry);
}

int CPlayListStoredOrder::IndexOf(EntryId entry) const
{
  const auto fits = [this](int position, EntryId id)
  { return position < static_cast<int>(m_order.size()) && m_order[position] == id; };

  if (const auto it = m_positions.find(entry); it != m_positions.end() && fits(it->second, entry))
    return it->second;

  m_positions.clear();
  for (int position = 0; position < static_cast<int>(m_order.size()); ++position)
    m_positions.emplace(m_order[position], position);

  const auto it = m_positions.find(entry);
  return it == m_positions.end() ? -1 : it->second;
}

EntryId CPlayListStoredOrder::Following(EntryId entry) const
{
  if (m_order.empty())
    return NO_ENTRY;
  if (entry == NO_ENTRY)
    return m_order.front();

  const int position = IndexOf(entry);
  if (position < 0 || position + 1 == static_cast<int>(m_order.size()))
    return NO_ENTRY;
  return m_order[position + 1];
}

EntryId CPlayListStoredOrder::Preceding(EntryId entry) const
{
  if (m_order.empty())
    return NO_ENTRY;
  if (entry == NO_ENTRY)
    return m_order.back();

  const int position = IndexOf(entry);
  if (position <= 0)
    return NO_ENTRY;
  return m_order[position - 1];
}

void CPlayListNoShuffle::Reset(const std::vector<EntryId>& listOrder, EntryId current)
{
  m_order = listOrder;
}

void CPlayListNoShuffle::OnAdded(EntryId entry, int position, EntryId current)
{
  if (position < 0 || position > static_cast<int>(m_order.size()))
    position = static_cast<int>(m_order.size());
  m_order.insert(m_order.begin() + position, entry);
}

void CPlayListNoShuffle::OnMoved(EntryId entry, int position)
{
  OnRemoved(entry);
  OnAdded(entry, position, NO_ENTRY);
}

CPlayListRandomShuffle::CPlayListRandomShuffle() : m_random(std::random_device{}())
{
}

void CPlayListRandomShuffle::Reset(const std::vector<EntryId>& listOrder, EntryId current)
{
  m_order = listOrder;
  auto toShuffle = m_order.begin();
  if (const auto it = std::ranges::find(m_order, current); it != m_order.end())
  {
    std::iter_swap(m_order.begin(), it);
    ++toShuffle;
  }
  std::shuffle(toShuffle, m_order.end(), m_random);
}

void CPlayListRandomShuffle::OnAdded(EntryId entry, int position, EntryId current)
{
  // Anywhere after the current entry, including straight after it and at the very end.
  const auto it = std::ranges::find(m_order, current);
  const size_t first = it == m_order.end() ? 0 : std::distance(m_order.begin(), it) + 1;
  std::uniform_int_distribution<size_t> pick(first, m_order.size());
  m_order.insert(m_order.begin() + pick(m_random), entry);
}

} // namespace KODI::PLAYLIST

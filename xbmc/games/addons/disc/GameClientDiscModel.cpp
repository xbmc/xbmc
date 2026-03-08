/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscModel.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

size_t CGameClientDiscModel::Size() const
{
  return m_discs.size();
}

bool CGameClientDiscModel::Empty() const
{
  return m_discs.empty();
}

void CGameClientDiscModel::Clear()
{
  m_discs.clear();
  m_selectedType = DiscSelectionType::NoDisc;
  m_selectedDiscIndex.reset();
  m_isEjected = false;
}

void CGameClientDiscModel::SetDiscs(const std::vector<GameClientDiscEntry>& discs)
{
  m_discs = discs;
  m_selectedType = DiscSelectionType::NoDisc;
  m_selectedDiscIndex.reset();
}

void CGameClientDiscModel::AddDisc(const std::string& path, const std::string& cachedLabel)
{
  m_discs.emplace_back(GameClientDiscEntry{GameClientDiscEntry::DiscSlotType::Disc, path,
                                           DeriveBasename(path), cachedLabel});

  if (m_discs.size() == 1)
  {
    m_selectedType = DiscSelectionType::Disc;
    m_selectedDiscIndex = 0;
  }
}

void CGameClientDiscModel::AddRemovedSlot()
{
  m_discs.emplace_back(
      GameClientDiscEntry{GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""});
}

bool CGameClientDiscModel::RemoveDiscByPath(const std::string& path)
{
  if (path.empty())
    return false;

  const auto index = GetDiscIndexByPath(path);
  if (!index.has_value())
    return false;

  return RemoveDiscByIndex(*index);
}

bool CGameClientDiscModel::RemoveDiscByIndex(size_t index)
{
  return MarkRemovedByIndex(index);
}

bool CGameClientDiscModel::MarkRemovedByIndex(size_t index)
{
  if (index >= m_discs.size())
    return false;

  GameClientDiscEntry& disc = m_discs[index];
  disc.slotType = GameClientDiscEntry::DiscSlotType::RemovedSlot;
  disc.path.clear();
  disc.basename.clear();
  disc.cachedLabel.clear();

  const bool wasSelectedDisc = (m_selectedType == DiscSelectionType::Disc &&
                                m_selectedDiscIndex.has_value() && *m_selectedDiscIndex == index);

  if (wasSelectedDisc)
  {
    m_selectedType = DiscSelectionType::NoDisc;
    m_selectedDiscIndex.reset();
  }

  return true;
}

bool CGameClientDiscModel::EraseDiscByIndex(size_t index)
{
  if (index >= m_discs.size())
    return false;

  const bool wasSelectedDisc = (m_selectedType == DiscSelectionType::Disc &&
                                m_selectedDiscIndex.has_value() && *m_selectedDiscIndex == index);

  m_discs.erase(m_discs.begin() + index);

  if (!m_selectedDiscIndex.has_value())
    return true;

  if (wasSelectedDisc)
  {
    m_selectedType = DiscSelectionType::NoDisc;
    m_selectedDiscIndex.reset();
  }
  else if (*m_selectedDiscIndex > index)
  {
    --(*m_selectedDiscIndex);
  }

  return true;
}

std::optional<size_t> CGameClientDiscModel::GetDiscIndexByPath(const std::string& path) const
{
  const auto it = std::find_if(
      m_discs.begin(), m_discs.end(), [&path](const GameClientDiscEntry& disc)
      { return disc.slotType == GameClientDiscEntry::DiscSlotType::Disc && disc.path == path; });
  if (it == m_discs.end())
    return std::nullopt;

  return static_cast<size_t>(it - m_discs.begin());
}

std::optional<size_t> CGameClientDiscModel::GetDiscIndexByBasename(
    const std::string& basename) const
{
  const auto it = std::find_if(m_discs.begin(), m_discs.end(),
                               [&basename](const GameClientDiscEntry& disc) {
                                 return disc.slotType == GameClientDiscEntry::DiscSlotType::Disc &&
                                        disc.basename == basename;
                               });
  if (it == m_discs.end())
    return std::nullopt;

  return static_cast<size_t>(it - m_discs.begin());
}

bool CGameClientDiscModel::SetSelectedDiscByPath(const std::string& path)
{
  const auto index = GetDiscIndexByPath(path);
  if (!index.has_value())
    return false;

  return SetSelectedDiscByIndex(*index);
}

bool CGameClientDiscModel::SetSelectedDiscByIndex(size_t index)
{
  if (!IsSelectableSlotByIndex(index))
    return false;

  m_selectedType = DiscSelectionType::Disc;
  m_selectedDiscIndex = index;

  return true;
}

void CGameClientDiscModel::SetSelectedNoDisc()
{
  m_selectedType = DiscSelectionType::NoDisc;
  m_selectedDiscIndex.reset();
}

std::optional<size_t> CGameClientDiscModel::GetSelectedDiscIndex() const
{
  if (m_selectedType != DiscSelectionType::Disc)
    return std::nullopt;

  return m_selectedDiscIndex;
}

std::string CGameClientDiscModel::GetSelectedDiscPath() const
{
  if (!m_selectedDiscIndex.has_value())
    return "";

  return GetPathByIndex(*m_selectedDiscIndex);
}

bool CGameClientDiscModel::UpdateCachedLabel(const std::string& path, const std::string& label)
{
  const auto index = GetDiscIndexByPath(path);
  if (!index.has_value())
    return false;

  m_discs[*index].cachedLabel = label;

  return true;
}

std::string CGameClientDiscModel::GetPathByIndex(size_t index) const
{
  const GameClientDiscEntry* disc = GetDiscByIndex(index);
  if (disc == nullptr)
    return "";

  if (disc->slotType != GameClientDiscEntry::DiscSlotType::Disc)
    return "";

  return disc->path;
}

std::string CGameClientDiscModel::GetLabelByIndex(size_t index) const
{
  const GameClientDiscEntry* disc = GetDiscByIndex(index);
  if (disc == nullptr)
    return "";

  if (disc->slotType == GameClientDiscEntry::DiscSlotType::RemovedSlot)
    return "";

  if (!disc->cachedLabel.empty())
    return disc->cachedLabel;

  if (!disc->basename.empty())
    return disc->basename;

  const std::string basename = DeriveBasename(disc->path);
  if (!basename.empty())
    return basename;

  return disc->path;
}

bool CGameClientDiscModel::IsRemovedSlotByIndex(size_t index) const
{
  const GameClientDiscEntry* disc = GetDiscByIndex(index);
  if (disc == nullptr)
    return false;

  return disc->slotType == GameClientDiscEntry::DiscSlotType::RemovedSlot;
}

bool CGameClientDiscModel::IsSelectableSlotByIndex(size_t index) const
{
  return index < m_discs.size() && !IsRemovedSlotByIndex(index);
}

const GameClientDiscEntry* CGameClientDiscModel::GetDiscByIndex(size_t index) const
{
  if (index >= m_discs.size())
    return nullptr;

  return &m_discs[index];
}

std::string CGameClientDiscModel::DeriveBasename(const std::string& path)
{
  if (path.empty())
    return "";

  const size_t end = path.find_last_not_of("/\\");
  if (end == std::string::npos)
    return "";

  const size_t pos = path.find_last_of("/\\", end);
  if (pos == std::string::npos)
    return path.substr(0, end + 1);

  return path.substr(pos + 1, end - pos);
}

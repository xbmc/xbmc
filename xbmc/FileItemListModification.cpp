/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItemListModification.h"

#include "music/windows/MusicFileItemListModifier.h"
#include "playlists/SmartPlaylistFileItemListModifier.h"
#include "video/windows/VideoFileItemListModifier.h"

#include <algorithm>

using namespace KODI;

CFileItemListModification::CFileItemListModification()
{
  m_modifiers.push_back(std::make_unique<PLAYLIST::CSmartPlaylistFileItemListModifier>());
  m_modifiers.push_back(std::make_unique<CMusicFileItemListModifier>());
  m_modifiers.push_back(std::make_unique<CVideoFileItemListModifier>());
}

CFileItemListModification& CFileItemListModification::GetInstance()
{
  static CFileItemListModification instance;
  return instance;
}

bool CFileItemListModification::CanModify(const CFileItemList &items) const
{
  return std::ranges::any_of(m_modifiers,
                             [&items](const auto& mod) { return mod->CanModify(items); });
}

bool CFileItemListModification::Modify(CFileItemList &items) const
{
  bool result = false;
  std::ranges::for_each(m_modifiers,
                        [&result, &items](const auto& mod) { result |= mod->Modify(items); });
  return result;
}

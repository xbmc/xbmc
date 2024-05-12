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

using namespace KODI;

CFileItemListModification::CFileItemListModification()
{
  m_modifiers.insert(new PLAYLIST::CSmartPlaylistFileItemListModifier());
  m_modifiers.insert(new CMusicFileItemListModifier());
  m_modifiers.insert(new CVideoFileItemListModifier());
}

CFileItemListModification::~CFileItemListModification()
{
  for (std::set<IFileItemListModifier*>::const_iterator modifier = m_modifiers.begin(); modifier != m_modifiers.end(); ++modifier)
    delete *modifier;

  m_modifiers.clear();
}

CFileItemListModification& CFileItemListModification::GetInstance()
{
  static CFileItemListModification instance;
  return instance;
}

bool CFileItemListModification::CanModify(const CFileItemList &items) const
{
  for (std::set<IFileItemListModifier*>::const_iterator modifier = m_modifiers.begin(); modifier != m_modifiers.end(); ++modifier)
  {
    if ((*modifier)->CanModify(items))
      return true;
  }

  return false;
}

bool CFileItemListModification::Modify(CFileItemList &items) const
{
  bool result = false;
  for (std::set<IFileItemListModifier*>::const_iterator modifier = m_modifiers.begin(); modifier != m_modifiers.end(); ++modifier)
    result |= (*modifier)->Modify(items);

  return result;
}

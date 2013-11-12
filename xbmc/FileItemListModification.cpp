/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "FileItemListModification.h"

#include "playlists/SmartPlaylistFileItemListModifier.h"

using namespace std;

CFileItemListModification::CFileItemListModification()
{
  m_modifiers.insert(new CSmartPlaylistFileItemListModifier());
}

CFileItemListModification::~CFileItemListModification()
{
  for (set<IFileItemListModifier*>::const_iterator modifier = m_modifiers.begin(); modifier != m_modifiers.end(); ++modifier)
    delete *modifier;

  m_modifiers.clear();
}

CFileItemListModification& CFileItemListModification::Get()
{
  static CFileItemListModification instance;
  return instance;
}

bool CFileItemListModification::CanModify(const CFileItemList &items) const
{
  for (set<IFileItemListModifier*>::const_iterator modifier = m_modifiers.begin(); modifier != m_modifiers.end(); ++modifier)
  {
    if ((*modifier)->CanModify(items))
      return true;
  }

  return false;
}

bool CFileItemListModification::Modify(CFileItemList &items) const
{
  bool result = false;
  for (set<IFileItemListModifier*>::const_iterator modifier = m_modifiers.begin(); modifier != m_modifiers.end(); ++modifier)
    result |= (*modifier)->Modify(items);

  return result;
}

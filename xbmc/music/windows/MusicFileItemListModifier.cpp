/*
*      Copyright (C) 2016 Team Kodi
*      http://kodi.tv
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
*  along with Kodi; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#include "MusicFileItemListModifier.h"
#include "ServiceBroker.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "FileItem.h"
#include "music/MusicDbUrl.h"
#include "settings/Settings.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"

using namespace XFILE::MUSICDATABASEDIRECTORY;

bool CMusicFileItemListModifier::CanModify(const CFileItemList &items) const
{
  if (items.IsMusicDb())
    return true;

  return false;
}

bool CMusicFileItemListModifier::Modify(CFileItemList &items) const
{
  AddQueuingFolder(items);
  return true;
}

//  Add an "* All ..." folder to the CFileItemList
//  depending on the child node
void CMusicFileItemListModifier::AddQueuingFolder(CFileItemList& items)
{
  if (!items.IsMusicDb())
    return;

  auto directoryNode = CDirectoryNode::ParseURL(items.GetPath());

  CFileItemPtr pItem;

  CMusicDbUrl musicUrl;
  if (!musicUrl.FromString(directoryNode->BuildPath()))
    return;

  // always show "all" items by default
  if (!CServiceBroker::GetSettings().GetBool(CSettings::SETTING_MUSICLIBRARY_SHOWALLITEMS))
    return;

  // no need for "all" item when only one item
  if (items.GetObjectCount() <= 1)
    return;

  switch (directoryNode->GetChildType())
  {
  case NODE_TYPE_ARTIST:
    if (directoryNode->GetType() == NODE_TYPE_OVERVIEW) return;
    pItem.reset(new CFileItem(g_localizeStrings.Get(15103)));  // "All Artists"
    musicUrl.AppendPath("-1/");
    pItem->SetPath(musicUrl.ToString());
    break;

    //  All album related nodes
  case NODE_TYPE_ALBUM:
    if (directoryNode->GetType() == NODE_TYPE_OVERVIEW) return;
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
  case NODE_TYPE_ALBUM_RECENTLY_ADDED:
  case NODE_TYPE_ALBUM_COMPILATIONS:
  case NODE_TYPE_ALBUM_TOP100:
  case NODE_TYPE_YEAR_ALBUM:
    pItem.reset(new CFileItem(g_localizeStrings.Get(15102)));  // "All Albums"
    musicUrl.AppendPath("-1/");
    pItem->SetPath(musicUrl.ToString());
    break;

  default:
    break;
  }

  if (pItem)
  {
    pItem->m_bIsFolder = true;
    pItem->SetSpecialSort(g_advancedSettings.m_bMusicLibraryAllItemsOnBottom ? SortSpecialOnBottom : SortSpecialOnTop);
    pItem->SetCanQueue(false);
    pItem->SetLabelPreformatted(true);
    if (g_advancedSettings.m_bMusicLibraryAllItemsOnBottom)
      items.Add(pItem);
    else
      items.AddFront(pItem, (items.Size() > 0 && items[0]->IsParentFolder()) ? 1 : 0);
  }
}
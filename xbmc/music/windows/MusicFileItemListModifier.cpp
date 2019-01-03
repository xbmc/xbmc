/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicFileItemListModifier.h"
#include "ServiceBroker.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "music/MusicDbUrl.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

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
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICLIBRARY_SHOWALLITEMS))
    return;

  // no need for "all" item when only one item
  if (items.GetObjectCount() <= 1)
    return;

  auto nodeChildType = directoryNode->GetChildType();

  // No need for "all" when overview node and child node albums or artists
  if (directoryNode->GetType() == NODE_TYPE_OVERVIEW &&
     (nodeChildType == NODE_TYPE_ARTIST || nodeChildType == NODE_TYPE_ALBUM))
    return;

  switch (nodeChildType)
  {
  case NODE_TYPE_ARTIST:
    pItem.reset(new CFileItem(g_localizeStrings.Get(15103)));  // "All Artists"
    musicUrl.AppendPath("-1/");
    pItem->SetPath(musicUrl.ToString());
    break;

  //  All album related nodes
  case NODE_TYPE_ALBUM:
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
    pItem->SetSpecialSort(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bMusicLibraryAllItemsOnBottom ? SortSpecialOnBottom : SortSpecialOnTop);
    pItem->SetCanQueue(false);
    pItem->SetLabelPreformatted(true);
    if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bMusicLibraryAllItemsOnBottom)
      items.Add(pItem);
    else
      items.AddFront(pItem, (items.Size() > 0 && items[0]->IsParentFolder()) ? 1 : 0);
  }
}

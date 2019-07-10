/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoFileItemListModifier.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "filesystem/VideoDatabaseDirectory/DirectoryNode.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "video/VideoDatabase.h"
#include "video/VideoDbUrl.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

bool CVideoFileItemListModifier::CanModify(const CFileItemList &items) const
{
  if (items.IsVideoDb())
    return true;

  return false;
}

bool CVideoFileItemListModifier::Modify(CFileItemList &items) const
{
  AddQueuingFolder(items);
  return true;
}

//  Add an "* All ..." folder to the CFileItemList
//  depending on the child node
void CVideoFileItemListModifier::AddQueuingFolder(CFileItemList& items)
{
  if (!items.IsVideoDb())
    return;

  auto directoryNode = CDirectoryNode::ParseURL(items.GetPath());

  CFileItemPtr pItem;

  // always show "all" items by default
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOLIBRARY_SHOWALLITEMS))
    return;

  // no need for "all" item when only one item
  if (items.GetObjectCount() <= 1)
    return;

  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(directoryNode->BuildPath()))
    return;

  // hack - as the season node might return episodes
  std::unique_ptr<CDirectoryNode> pNode(directoryNode);

  switch (pNode->GetChildType())
  {
  case NODE_TYPE_SEASONS:
  {
    std::string strLabel = g_localizeStrings.Get(20366);
    pItem.reset(new CFileItem(strLabel));  // "All Seasons"
    videoUrl.AppendPath("-1/");
    pItem->SetPath(videoUrl.ToString());
    // set the number of watched and unwatched items accordingly
    int watched = 0;
    int unwatched = 0;
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      watched += static_cast<int>(item->GetProperty("watchedepisodes").asInteger());
      unwatched += static_cast<int>(item->GetProperty("unwatchedepisodes").asInteger());
    }
    pItem->SetProperty("totalepisodes", watched + unwatched);
    pItem->SetProperty("numepisodes", watched + unwatched); // will be changed later to reflect watchmode setting
    pItem->SetProperty("watchedepisodes", watched);
    pItem->SetProperty("unwatchedepisodes", unwatched);
    if (items.Size() && items[0]->GetVideoInfoTag())
    {
      *pItem->GetVideoInfoTag() = *items[0]->GetVideoInfoTag();
      pItem->GetVideoInfoTag()->m_iSeason = -1;
    }
    pItem->GetVideoInfoTag()->m_strTitle = strLabel;
    pItem->GetVideoInfoTag()->m_iEpisode = watched + unwatched;
    pItem->GetVideoInfoTag()->SetPlayCount((unwatched == 0) ? 1 : 0);
    CVideoDatabase db;
    if (db.Open())
    {
      pItem->GetVideoInfoTag()->m_iDbId = db.GetSeasonId(pItem->GetVideoInfoTag()->m_iIdShow, -1);
      db.Close();
    }
    pItem->GetVideoInfoTag()->m_type = MediaTypeSeason;
  }
  break;
  case NODE_TYPE_MUSICVIDEOS_ALBUM:
    pItem.reset(new CFileItem(g_localizeStrings.Get(15102)));  // "All Albums"
    videoUrl.AppendPath("-1/");
    pItem->SetPath(videoUrl.ToString());
    break;
  default:
    break;
  }

  if (pItem)
  {
    pItem->m_bIsFolder = true;
    pItem->SetSpecialSort(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoLibraryAllItemsOnBottom ? SortSpecialOnBottom : SortSpecialOnTop);
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }
}

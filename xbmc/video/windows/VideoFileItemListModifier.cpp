/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoFileItemListModifier.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "filesystem/VideoDatabaseDirectory/DirectoryNode.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "video/VideoDatabase.h"
#include "video/VideoDbUrl.h"
#include "video/VideoFileItemClassify.h"

#include <memory>

using namespace KODI::VIDEO;
using namespace XFILE::VIDEODATABASEDIRECTORY;

bool CVideoFileItemListModifier::CanModify(const CFileItemList &items) const
{
  if (IsVideoDb(items))
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
  if (!IsVideoDb(items))
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
    const std::string& strLabel = g_localizeStrings.Get(20366);
    pItem = std::make_shared<CFileItem>(strLabel); // "All Seasons"
    videoUrl.AppendPath("-1/");
    pItem->SetPath(videoUrl.ToString());
    // set the number of watched and unwatched items accordingly
    int watched = 0;
    int unwatched = 0;
    int inprogress = 0;
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      watched += static_cast<int>(item->GetProperty("watchedepisodes").asInteger());
      unwatched += static_cast<int>(item->GetProperty("unwatchedepisodes").asInteger());
      inprogress += static_cast<int>(item->GetProperty("inprogressepisodes").asInteger());
    }
    const int totalEpisodes = watched + unwatched;
    pItem->SetProperty("totalepisodes", totalEpisodes);
    pItem->SetProperty("numepisodes",
                       totalEpisodes); // will be changed later to reflect watchmode setting
    pItem->SetProperty("watchedepisodes", watched);
    pItem->SetProperty("unwatchedepisodes", unwatched);
    pItem->SetProperty("inprogressepisodes", inprogress);
    pItem->SetProperty("watchedepisodepercent",
                       totalEpisodes > 0 ? watched * 100 / totalEpisodes : 0);

    // @note: The items list may contain additional items that do not belong to the show.
    // This is the case of the up directory (..) or movies linked to the tvshow.
    // Iterate through the list till the first season type is found and the infotag can safely be copied.

    if (items.Size() > 1)
    {
      for (int i = 1; i < items.Size(); i++)
      {
        if (items[i]->GetVideoInfoTag() && items[i]->GetVideoInfoTag()->m_type == MediaTypeSeason &&
            items[i]->GetVideoInfoTag()->m_iSeason > 0)
        {
          *pItem->GetVideoInfoTag() = *items[i]->GetVideoInfoTag();
          pItem->GetVideoInfoTag()->m_iSeason = -1;
          break;
        }
      }
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
    pItem = std::make_shared<CFileItem>("* " + g_localizeStrings.Get(16100)); // "* All Videos"
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

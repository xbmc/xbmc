/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeOverview.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "video/VideoDatabase.h"

#include <utility>

using namespace XFILE::VIDEODATABASEDIRECTORY;

Node OverviewChildren[] = {
    {NodeType::MOVIES_OVERVIEW, "movies", 342},
    {NodeType::TVSHOWS_OVERVIEW, "tvshows", 20343},
    {NodeType::MUSICVIDEOS_OVERVIEW, "musicvideos", 20389},
    {NodeType::RECENTLY_ADDED_MOVIES, "recentlyaddedmovies", 20386},
    {NodeType::RECENTLY_ADDED_EPISODES, "recentlyaddedepisodes", 20387},
    {NodeType::RECENTLY_ADDED_MUSICVIDEOS, "recentlyaddedmusicvideos", 20390},
    {NodeType::INPROGRESS_TVSHOWS, "inprogresstvshows", 626},
};

CDirectoryNodeOverview::CDirectoryNodeOverview(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::OVERVIEW, strName, pParent)
{

}

NodeType CDirectoryNodeOverview::GetChildType() const
{
  for (const Node& node : OverviewChildren)
    if (GetName() == node.id)
      return node.node;

  return NodeType::NONE;
}

std::string CDirectoryNodeOverview::GetLocalizedName() const
{
  for (const Node& node : OverviewChildren)
    if (GetName() == node.id)
      return g_localizeStrings.Get(node.label);
  return "";
}

bool CDirectoryNodeOverview::GetContent(CFileItemList& items) const
{
  CVideoDatabase database;
  database.Open();
  bool hasMovies = database.HasContent(VideoDbContentType::MOVIES);
  bool hasTvShows = database.HasContent(VideoDbContentType::TVSHOWS);
  bool hasMusicVideos = database.HasContent(VideoDbContentType::MUSICVIDEOS);
  std::vector<std::pair<const char*, int> > vec;
  if (hasMovies)
  {
    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MYVIDEOS_FLATTEN))
      vec.emplace_back("movies/titles", 342);
    else
      vec.emplace_back("movies", 342); // Movies
  }
  if (hasTvShows)
  {
    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MYVIDEOS_FLATTEN))
      vec.emplace_back("tvshows/titles", 20343);
    else
      vec.emplace_back("tvshows", 20343); // TV Shows
  }
  if (hasMusicVideos)
  {
    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MYVIDEOS_FLATTEN))
      vec.emplace_back("musicvideos/titles", 20389);
    else
      vec.emplace_back("musicvideos", 20389); // Music Videos
  }
  {
    if (hasMovies)
      vec.emplace_back("recentlyaddedmovies", 20386); // Recently Added Movies
    if (hasTvShows)
    {
      vec.emplace_back("recentlyaddedepisodes", 20387); // Recently Added Episodes
      vec.emplace_back("inprogresstvshows", 626); // InProgress TvShows
    }
    if (hasMusicVideos)
      vec.emplace_back("recentlyaddedmusicvideos", 20390); // Recently Added Music Videos
  }
  std::string path = BuildPath();
  for (unsigned int i = 0; i < vec.size(); ++i)
  {
    CFileItemPtr pItem(new CFileItem(path + vec[i].first + "/", true));
    pItem->SetLabel(g_localizeStrings.Get(vec[i].second));
    pItem->SetLabelPreformatted(true);
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}

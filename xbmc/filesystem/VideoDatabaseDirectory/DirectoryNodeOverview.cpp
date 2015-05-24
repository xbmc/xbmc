/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "video/VideoDatabase.h"
#include "DirectoryNodeOverview.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;
using namespace std;


Node OverviewChildren[] = {
                            { NODE_TYPE_MOVIES_OVERVIEW,            "movies",                   342 },
                            { NODE_TYPE_TVSHOWS_OVERVIEW,           "tvshows",                  20343 },
                            { NODE_TYPE_MUSICVIDEOS_OVERVIEW,       "musicvideos",              20389 },
                            { NODE_TYPE_RECENTLY_ADDED_MOVIES,      "recentlyaddedmovies",      20386 },
                            { NODE_TYPE_RECENTLY_ADDED_EPISODES,    "recentlyaddedepisodes",    20387 },
                            { NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS, "recentlyaddedmusicvideos", 20390 },
                          };

CDirectoryNodeOverview::CDirectoryNodeOverview(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeOverview::GetChildType() const
{
  for (unsigned int i = 0; i < sizeof(OverviewChildren) / sizeof(Node); ++i)
    if (GetName() == OverviewChildren[i].id)
      return OverviewChildren[i].node;

  return NODE_TYPE_NONE;
}

std::string CDirectoryNodeOverview::GetLocalizedName() const
{
  for (unsigned int i = 0; i < sizeof(OverviewChildren) / sizeof(Node); ++i)
    if (GetName() == OverviewChildren[i].id)
      return g_localizeStrings.Get(OverviewChildren[i].label);
  return "";
}

bool CDirectoryNodeOverview::GetContent(CFileItemList& items) const
{
  CVideoDatabase database;
  database.Open();
  bool hasMovies = database.HasContent(VIDEODB_CONTENT_MOVIES);
  bool hasTvShows = database.HasContent(VIDEODB_CONTENT_TVSHOWS);
  bool hasMusicVideos = database.HasContent(VIDEODB_CONTENT_MUSICVIDEOS);
  vector<pair<const char*, int> > vec;
  if (hasMovies)
  {
    if (CSettings::Get().GetBool("myvideos.flatten"))
      vec.push_back(make_pair("movies/titles", 342));
    else
      vec.push_back(make_pair("movies", 342));   // Movies
  }
  if (hasTvShows)
  {
    if (CSettings::Get().GetBool("myvideos.flatten"))
      vec.push_back(make_pair("tvshows/titles", 20343));
    else
      vec.push_back(make_pair("tvshows", 20343)); // TV Shows
  }
  if (hasMusicVideos)
  {
    if (CSettings::Get().GetBool("myvideos.flatten"))
      vec.push_back(make_pair("musicvideos/titles", 20389));
    else
      vec.push_back(make_pair("musicvideos", 20389)); // Music Videos
  }
  {
    if (hasMovies)
      vec.push_back(make_pair("recentlyaddedmovies", 20386));  // Recently Added Movies
    if (hasTvShows)
      vec.push_back(make_pair("recentlyaddedepisodes", 20387)); // Recently Added Episodes
    if (hasMusicVideos)
      vec.push_back(make_pair("recentlyaddedmusicvideos", 20390)); // Recently Added Music Videos
  }
  std::string path = BuildPath();
  for (unsigned int i = 0; i < vec.size(); ++i)
  {
    CFileItemPtr pItem(new CFileItem(path + vec[i].first + "/", true));
    pItem->SetLabel(g_localizeStrings.Get(vec[i].second));
    pItem->SetLabelPreformated(true);
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}

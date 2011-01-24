/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "video/VideoDatabase.h"
#include "DirectoryNodeOverview.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;
using namespace std;

CDirectoryNodeOverview::CDirectoryNodeOverview(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeOverview::GetChildType()
{
  if (GetName()=="1")
    return NODE_TYPE_MOVIES_OVERVIEW;
  else if (GetName()=="2")
    return NODE_TYPE_TVSHOWS_OVERVIEW;
  else if (GetName() == "3")
    return NODE_TYPE_MUSICVIDEOS_OVERVIEW;
  else if (GetName() == "4")
    return NODE_TYPE_RECENTLY_ADDED_MOVIES;
  else if (GetName() == "5")
    return NODE_TYPE_RECENTLY_ADDED_EPISODES;
  else if (GetName() == "6")
    return NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS;

  return NODE_TYPE_NONE;
}

bool CDirectoryNodeOverview::GetContent(CFileItemList& items)
{
  CVideoDatabase database;
  database.Open();
  bool hasMovies = database.HasContent(VIDEODB_CONTENT_MOVIES);
  bool hasTvShows = database.HasContent(VIDEODB_CONTENT_TVSHOWS);
  bool hasMusicVideos = database.HasContent(VIDEODB_CONTENT_MUSICVIDEOS);
  vector<pair<const char*, int> > vec;
  if (hasMovies)
  {
    if (g_settings.m_bMyVideoNavFlatten)
      vec.push_back(make_pair("1/2", 342));
    else
      vec.push_back(make_pair("1", 342));   // Movies
  }
  if (hasTvShows)
  {
    if (g_settings.m_bMyVideoNavFlatten)
      vec.push_back(make_pair("2/2", 20343));
    else
      vec.push_back(make_pair("2", 20343)); // TV Shows
  }
  if (hasMusicVideos)
  {
    if (g_settings.m_bMyVideoNavFlatten)
      vec.push_back(make_pair("3/2", 20389));
    else
      vec.push_back(make_pair("3", 20389)); // Music Videos
  }
  if (!g_advancedSettings.m_bVideoLibraryHideRecentlyAddedItems)
  {
    if (hasMovies)
      vec.push_back(make_pair("4", 20386));  // Recently Added Movies
    if (hasTvShows)
      vec.push_back(make_pair("5", 20387)); // Recently Added Episodes
    if (hasMusicVideos)
      vec.push_back(make_pair("6", 20390)); // Recently Added Music Videos
  }
  CStdString path = BuildPath();
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

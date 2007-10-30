#include "stdafx.h"
#include "../../VideoDatabase.h"
#include "DirectoryNodeOverview.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;
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
  int iMovies = database.GetMovieCount();
  int iTvShows = database.GetTvShowCount();
  int iMusicVideos = database.GetMusicVideoCount();
  vector<pair<const char*, int> > vec;
  if (iMovies > 0)
    vec.push_back(make_pair("1", 342));   // Movies
  if (iTvShows > 0)
    vec.push_back(make_pair("2", 20343)); // TV Shows
  if (iMusicVideos > 0)
    vec.push_back(make_pair("3", 20389)); // Music Videos
  if (!g_advancedSettings.m_bVideoLibraryHideRecentlyAddedItems)
  {
    if (iMovies > 0)
      vec.push_back(make_pair("4", 20386));  // Recently Added Movies
    if (iTvShows > 0)
      vec.push_back(make_pair("5", 20387)); // Recently Added Episodes
    if (iMusicVideos > 0)
      vec.push_back(make_pair("6", 20390)); // Recently Added Music Videos
  }
  CStdString path = BuildPath();
  for (int i = 0; i < (int)vec.size(); ++i)
  {
    CFileItem* pItem = new CFileItem(path + vec[i].first + "/", true);
    pItem->SetLabel(g_localizeStrings.Get(vec[i].second));
    pItem->SetLabelPreformated(true);
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}

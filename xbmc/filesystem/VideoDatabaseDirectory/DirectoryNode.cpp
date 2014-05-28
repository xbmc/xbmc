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

#ifndef VIDEODATABASEDIRECTORY_DIRECTORYNODE_H_INCLUDED
#define VIDEODATABASEDIRECTORY_DIRECTORYNODE_H_INCLUDED
#include "DirectoryNode.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_UTILS_URIUTILS_H_INCLUDED
#define VIDEODATABASEDIRECTORY_UTILS_URIUTILS_H_INCLUDED
#include "utils/URIUtils.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_QUERYPARAMS_H_INCLUDED
#define VIDEODATABASEDIRECTORY_QUERYPARAMS_H_INCLUDED
#include "QueryParams.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_DIRECTORYNODEROOT_H_INCLUDED
#define VIDEODATABASEDIRECTORY_DIRECTORYNODEROOT_H_INCLUDED
#include "DirectoryNodeRoot.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_DIRECTORYNODEOVERVIEW_H_INCLUDED
#define VIDEODATABASEDIRECTORY_DIRECTORYNODEOVERVIEW_H_INCLUDED
#include "DirectoryNodeOverview.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_DIRECTORYNODEGROUPED_H_INCLUDED
#define VIDEODATABASEDIRECTORY_DIRECTORYNODEGROUPED_H_INCLUDED
#include "DirectoryNodeGrouped.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_DIRECTORYNODETITLEMOVIES_H_INCLUDED
#define VIDEODATABASEDIRECTORY_DIRECTORYNODETITLEMOVIES_H_INCLUDED
#include "DirectoryNodeTitleMovies.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_DIRECTORYNODETITLETVSHOWS_H_INCLUDED
#define VIDEODATABASEDIRECTORY_DIRECTORYNODETITLETVSHOWS_H_INCLUDED
#include "DirectoryNodeTitleTvShows.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_DIRECTORYNODEMOVIESOVERVIEW_H_INCLUDED
#define VIDEODATABASEDIRECTORY_DIRECTORYNODEMOVIESOVERVIEW_H_INCLUDED
#include "DirectoryNodeMoviesOverview.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_DIRECTORYNODETVSHOWSOVERVIEW_H_INCLUDED
#define VIDEODATABASEDIRECTORY_DIRECTORYNODETVSHOWSOVERVIEW_H_INCLUDED
#include "DirectoryNodeTvShowsOverview.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_DIRECTORYNODESEASONS_H_INCLUDED
#define VIDEODATABASEDIRECTORY_DIRECTORYNODESEASONS_H_INCLUDED
#include "DirectoryNodeSeasons.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_DIRECTORYNODEEPISODES_H_INCLUDED
#define VIDEODATABASEDIRECTORY_DIRECTORYNODEEPISODES_H_INCLUDED
#include "DirectoryNodeEpisodes.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_DIRECTORYNODERECENTLYADDEDMOVIES_H_INCLUDED
#define VIDEODATABASEDIRECTORY_DIRECTORYNODERECENTLYADDEDMOVIES_H_INCLUDED
#include "DirectoryNodeRecentlyAddedMovies.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_DIRECTORYNODERECENTLYADDEDEPISODES_H_INCLUDED
#define VIDEODATABASEDIRECTORY_DIRECTORYNODERECENTLYADDEDEPISODES_H_INCLUDED
#include "DirectoryNodeRecentlyAddedEpisodes.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_DIRECTORYNODEMUSICVIDEOSOVERVIEW_H_INCLUDED
#define VIDEODATABASEDIRECTORY_DIRECTORYNODEMUSICVIDEOSOVERVIEW_H_INCLUDED
#include "DirectoryNodeMusicVideosOverview.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_DIRECTORYNODERECENTLYADDEDMUSICVIDEOS_H_INCLUDED
#define VIDEODATABASEDIRECTORY_DIRECTORYNODERECENTLYADDEDMUSICVIDEOS_H_INCLUDED
#include "DirectoryNodeRecentlyAddedMusicVideos.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_DIRECTORYNODETITLEMUSICVIDEOS_H_INCLUDED
#define VIDEODATABASEDIRECTORY_DIRECTORYNODETITLEMUSICVIDEOS_H_INCLUDED
#include "DirectoryNodeTitleMusicVideos.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_VIDEO_VIDEOINFOTAG_H_INCLUDED
#define VIDEODATABASEDIRECTORY_VIDEO_VIDEOINFOTAG_H_INCLUDED
#include "video/VideoInfoTag.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_URL_H_INCLUDED
#define VIDEODATABASEDIRECTORY_URL_H_INCLUDED
#include "URL.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_SETTINGS_ADVANCEDSETTINGS_H_INCLUDED
#define VIDEODATABASEDIRECTORY_SETTINGS_ADVANCEDSETTINGS_H_INCLUDED
#include "settings/AdvancedSettings.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_FILEITEM_H_INCLUDED
#define VIDEODATABASEDIRECTORY_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_FILESYSTEM_FILE_H_INCLUDED
#define VIDEODATABASEDIRECTORY_FILESYSTEM_FILE_H_INCLUDED
#include "filesystem/File.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_UTILS_STRINGUTILS_H_INCLUDED
#define VIDEODATABASEDIRECTORY_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_GUILIB_LOCALIZESTRINGS_H_INCLUDED
#define VIDEODATABASEDIRECTORY_GUILIB_LOCALIZESTRINGS_H_INCLUDED
#include "guilib/LocalizeStrings.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_UTILS_VARIANT_H_INCLUDED
#define VIDEODATABASEDIRECTORY_UTILS_VARIANT_H_INCLUDED
#include "utils/Variant.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_VIDEO_VIDEODATABASE_H_INCLUDED
#define VIDEODATABASEDIRECTORY_VIDEO_VIDEODATABASE_H_INCLUDED
#include "video/VideoDatabase.h"
#endif


using namespace std;
using namespace XFILE::VIDEODATABASEDIRECTORY;

//  Constructor is protected use ParseURL()
CDirectoryNode::CDirectoryNode(NODE_TYPE Type, const CStdString& strName, CDirectoryNode* pParent)
{
  m_Type=Type;
  m_strName=strName;
  m_pParent=pParent;
}

CDirectoryNode::~CDirectoryNode()
{
  delete m_pParent;
}

//  Parses a given path and returns the current node of the path
CDirectoryNode* CDirectoryNode::ParseURL(const CStdString& strPath)
{
  CURL url(strPath);

  CStdString strDirectory=url.GetFileName();
  URIUtils::RemoveSlashAtEnd(strDirectory);

  CStdStringArray Path;
  StringUtils::SplitString(strDirectory, "/", Path);
  if (!strDirectory.empty())
    Path.insert(Path.begin(), "");

  CDirectoryNode* pNode=NULL;
  CDirectoryNode* pParent=NULL;
  NODE_TYPE NodeType=NODE_TYPE_ROOT;

  for (int i=0; i<(int)Path.size(); ++i)
  {
    pNode=CDirectoryNode::CreateNode(NodeType, Path[i], pParent);
    NodeType= pNode ? pNode->GetChildType() : NODE_TYPE_NONE;
    pParent=pNode;
  }

  // Add all the additional URL options to the last node
  if (pNode)
    pNode->AddOptions(url.GetOptions());

  return pNode;
}

//  returns the database ids of the path,
void CDirectoryNode::GetDatabaseInfo(const CStdString& strPath, CQueryParams& params)
{
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));

  if (!pNode.get())
    return;

  pNode->CollectQueryParams(params);
}

//  Create a node object
CDirectoryNode* CDirectoryNode::CreateNode(NODE_TYPE Type, const CStdString& strName, CDirectoryNode* pParent)
{
  switch (Type)
  {
  case NODE_TYPE_ROOT:
    return new CDirectoryNodeRoot(strName, pParent);
  case NODE_TYPE_OVERVIEW:
    return new CDirectoryNodeOverview(strName, pParent);
  case NODE_TYPE_GENRE:
  case NODE_TYPE_COUNTRY:
  case NODE_TYPE_SETS:
  case NODE_TYPE_TAGS:
  case NODE_TYPE_YEAR:
  case NODE_TYPE_ACTOR:
  case NODE_TYPE_DIRECTOR:
  case NODE_TYPE_STUDIO:
  case NODE_TYPE_MUSICVIDEOS_ALBUM:
    return new CDirectoryNodeGrouped(Type, strName, pParent);
  case NODE_TYPE_TITLE_MOVIES:
    return new CDirectoryNodeTitleMovies(strName, pParent);
  case NODE_TYPE_TITLE_TVSHOWS:
    return new CDirectoryNodeTitleTvShows(strName, pParent);
  case NODE_TYPE_MOVIES_OVERVIEW:
    return new CDirectoryNodeMoviesOverview(strName, pParent);
  case NODE_TYPE_TVSHOWS_OVERVIEW:
    return new CDirectoryNodeTvShowsOverview(strName, pParent);
  case NODE_TYPE_SEASONS:
    return new CDirectoryNodeSeasons(strName, pParent);
  case NODE_TYPE_EPISODES:
    return new CDirectoryNodeEpisodes(strName, pParent);
  case NODE_TYPE_RECENTLY_ADDED_MOVIES:
    return new CDirectoryNodeRecentlyAddedMovies(strName,pParent);
  case NODE_TYPE_RECENTLY_ADDED_EPISODES:
    return new CDirectoryNodeRecentlyAddedEpisodes(strName,pParent);
  case NODE_TYPE_MUSICVIDEOS_OVERVIEW:
    return new CDirectoryNodeMusicVideosOverview(strName,pParent);
  case NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS:
    return new CDirectoryNodeRecentlyAddedMusicVideos(strName,pParent);
  case NODE_TYPE_TITLE_MUSICVIDEOS:
    return new CDirectoryNodeTitleMusicVideos(strName,pParent);
  default:
    break;
  }

  return NULL;
}

//  Current node name
const CStdString& CDirectoryNode::GetName() const
{
  return m_strName;
}

int CDirectoryNode::GetID() const
{
  return atoi(m_strName.c_str());
}

CStdString CDirectoryNode::GetLocalizedName() const
{
  return "";
}

//  Current node type
NODE_TYPE CDirectoryNode::GetType() const
{
  return m_Type;
}

//  Return the parent directory node or NULL, if there is no
CDirectoryNode* CDirectoryNode::GetParent() const
{
  return m_pParent;
}

void CDirectoryNode::RemoveParent()
{
  m_pParent=NULL;
}

//  should be overloaded by a derived class
//  to get the content of a node. Will be called
//  by GetChilds() of a parent node
bool CDirectoryNode::GetContent(CFileItemList& items) const
{
  return false;
}

//  Creates a videodb url
CStdString CDirectoryNode::BuildPath() const
{
  CStdStringArray array;

  if (!m_strName.empty())
    array.insert(array.begin(), m_strName);

  CDirectoryNode* pParent=m_pParent;
  while (pParent!=NULL)
  {
    const CStdString& strNodeName=pParent->GetName();
    if (!strNodeName.empty())
      array.insert(array.begin(), strNodeName);

    pParent=pParent->GetParent();
  }

  CStdString strPath="videodb://";
  for (int i=0; i<(int)array.size(); ++i)
    strPath+=array[i]+"/";

  string options = m_options.GetOptionsString();
  if (!options.empty())
    strPath += "?" + options;

  return strPath;
}

void CDirectoryNode::AddOptions(const CStdString &options)
{
  if (options.empty())
    return;

  m_options.AddOptions(options);
}

//  Collects Query params from this and all parent nodes. If a NODE_TYPE can
//  be used as a database parameter, it will be added to the
//  params object.
void CDirectoryNode::CollectQueryParams(CQueryParams& params) const
{
  params.SetQueryParam(m_Type, m_strName);

  CDirectoryNode* pParent=m_pParent;
  while (pParent!=NULL)
  {
    params.SetQueryParam(pParent->GetType(), pParent->GetName());
    pParent=pParent->GetParent();
  }
}

//  Should be overloaded by a derived class.
//  Returns the NODE_TYPE of the child nodes.
NODE_TYPE CDirectoryNode::GetChildType() const
{
  return NODE_TYPE_NONE;
}

//  Get the child fileitems of this node
bool CDirectoryNode::GetChilds(CFileItemList& items)
{
  if (CanCache() && items.Load())
    return true;

  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::CreateNode(GetChildType(), "", this));

  bool bSuccess=false;
  if (pNode.get())
  {
    pNode->m_options = m_options;
    bSuccess=pNode->GetContent(items);
    if (bSuccess)
    {
      AddQueuingFolder(items);
      if (CanCache())
        items.SetCacheToDisc(CFileItemList::CACHE_ALWAYS);
    }
    else
      items.Clear();

    pNode->RemoveParent();
  }

  return bSuccess;
}

//  Add an "* All ..." folder to the CFileItemList
//  depending on the child node
void CDirectoryNode::AddQueuingFolder(CFileItemList& items) const
{
  CFileItemPtr pItem;

  // always hide "all" items
  if (g_advancedSettings.m_bVideoLibraryHideAllItems)
    return;

  // no need for "all" item when only one item
  if (items.GetObjectCount() <= 1)
    return;

  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(BuildPath()))
    return;

  // hack - as the season node might return episodes
  auto_ptr<CDirectoryNode> pNode(ParseURL(items.GetPath()));

  switch (pNode->GetChildType())
  {
    case NODE_TYPE_SEASONS:
      {
        CStdString strLabel = g_localizeStrings.Get(20366);
        pItem.reset(new CFileItem(strLabel));  // "All Seasons"
        videoUrl.AppendPath("-1/");
        pItem->SetPath(videoUrl.ToString());
        // set the number of watched and unwatched items accordingly
        int watched = 0;
        int unwatched = 0;
        for (int i = 0; i < items.Size(); i++)
        {
          CFileItemPtr item = items[i];
          watched += (int)item->GetProperty("watchedepisodes").asInteger();
          unwatched += (int)item->GetProperty("unwatchedepisodes").asInteger();
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
        pItem->GetVideoInfoTag()->m_playCount = (unwatched == 0) ? 1 : 0;
        CVideoDatabase db;
        if (db.Open())
        {
          pItem->GetVideoInfoTag()->m_iDbId = db.GetSeasonId(pItem->GetVideoInfoTag()->m_iIdShow, -1);
          db.Close();
        }
        pItem->GetVideoInfoTag()->m_type = "season";
      }
      break;
    default:
      break;
  }

  if (pItem)
  {
    pItem->m_bIsFolder = true;
    pItem->SetSpecialSort(g_advancedSettings.m_bVideoLibraryAllItemsOnBottom ? SortSpecialOnBottom : SortSpecialOnTop);
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }
}

bool CDirectoryNode::CanCache() const
{
  // no caching is required - the list is cached in CGUIMediaWindow::GetDirectory
  // if it was slow to fetch anyway.
  return false;
}

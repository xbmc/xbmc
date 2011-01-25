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

#include "DirectoryNode.h"
#include "utils/URIUtils.h"
#include "QueryParams.h"
#include "DirectoryNodeRoot.h"
#include "DirectoryNodeOverview.h"
#include "DirectoryNodeGenre.h"
#include "DirectoryNodeCountry.h"
#include "DirectoryNodeSets.h"
#include "DirectoryNodeTitleMovies.h"
#include "DirectoryNodeTitleTvShows.h"
#include "DirectoryNodeYear.h"
#include "DirectoryNodeActor.h"
#include "DirectoryNodeDirector.h"
#include "DirectoryNodeMoviesOverview.h"
#include "DirectoryNodeTvShowsOverview.h"
#include "DirectoryNodeSeasons.h"
#include "DirectoryNodeEpisodes.h"
#include "DirectoryNodeRecentlyAddedMovies.h"
#include "DirectoryNodeRecentlyAddedEpisodes.h"
#include "DirectoryNodeStudio.h"
#include "DirectoryNodeMusicVideosOverview.h"
#include "DirectoryNodeRecentlyAddedMusicVideos.h"
#include "DirectoryNodeTitleMusicVideos.h"
#include "DirectoryNodeMusicVideoAlbum.h"
#include "video/VideoInfoTag.h"
#include "URL.h"
#include "settings/AdvancedSettings.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "guilib/LocalizeStrings.h"

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
  if (!strDirectory.IsEmpty())
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
    return new CDirectoryNodeGenre(strName, pParent);
  case NODE_TYPE_COUNTRY:
    return new CDirectoryNodeCountry(strName, pParent);
  case NODE_TYPE_SETS:
    return new CDirectoryNodeSets(strName, pParent);
  case NODE_TYPE_YEAR:
    return new CDirectoryNodeYear(strName, pParent);
  case NODE_TYPE_ACTOR:
    return new CDirectoryNodeActor(strName, pParent);
  case NODE_TYPE_DIRECTOR:
    return new CDirectoryNodeDirector(strName, pParent);
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
  case NODE_TYPE_STUDIO:
    return new CDirectoryNodeStudio(strName,pParent);
  case NODE_TYPE_MUSICVIDEOS_OVERVIEW:
    return new CDirectoryNodeMusicVideosOverview(strName,pParent);
  case NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS:
    return new CDirectoryNodeRecentlyAddedMusicVideos(strName,pParent);
  case NODE_TYPE_TITLE_MUSICVIDEOS:
    return new CDirectoryNodeTitleMusicVideos(strName,pParent);
  case NODE_TYPE_MUSICVIDEOS_ALBUM:
    return new CDirectoryNodeMusicVideoAlbum(strName,pParent);
  default:
    break;
  }

  return NULL;
}

//  Current node name
const CStdString& CDirectoryNode::GetName()
{
  return m_strName;
}

//  Current node type
NODE_TYPE CDirectoryNode::GetType()
{
  return m_Type;
}

//  Return the parent directory node or NULL, if there is no
CDirectoryNode* CDirectoryNode::GetParent()
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
bool CDirectoryNode::GetContent(CFileItemList& items)
{
  return false;
}

//  Creates a videodb url
CStdString CDirectoryNode::BuildPath()
{
  CStdStringArray array;

  if (!m_strName.IsEmpty())
    array.insert(array.begin(), m_strName);

  CDirectoryNode* pParent=m_pParent;
  while (pParent!=NULL)
  {
    const CStdString& strNodeName=pParent->GetName();
    if (!strNodeName.IsEmpty())
      array.insert(array.begin(), strNodeName);

    pParent=pParent->GetParent();
  }

  CStdString strPath="videodb://";
  for (int i=0; i<(int)array.size(); ++i)
    strPath+=array[i]+"/";

  return strPath;
}

//  Collects Query params from this and all parent nodes. If a NODE_TYPE can
//  be used as a database parameter, it will be added to the
//  params object.
void CDirectoryNode::CollectQueryParams(CQueryParams& params)
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
NODE_TYPE CDirectoryNode::GetChildType()
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
void CDirectoryNode::AddQueuingFolder(CFileItemList& items)
{
  CFileItemPtr pItem;

  // always hide "all" items
  if (g_advancedSettings.m_bVideoLibraryHideAllItems)
    return;

  // no need for "all" item when only one item
  if (items.GetObjectCount() <= 1)
    return;

  // hack - as the season node might return episodes
  auto_ptr<CDirectoryNode> pNode(ParseURL(items.m_strPath));

  switch (pNode->GetChildType())
  {
    case NODE_TYPE_SEASONS:
      {
        CStdString strLabel = g_localizeStrings.Get(20366);
        pItem.reset(new CFileItem(strLabel));  // "All Seasons"
        pItem->m_strPath = BuildPath() + "-1/";
        // set the number of watched and unwatched items accordingly
        int watched = 0;
        int unwatched = 0;
        for (int i = 0; i < items.Size(); i++)
        {
          CFileItemPtr item = items[i];
          watched += item->GetPropertyInt("watchedepisodes");
          unwatched += item->GetPropertyInt("unwatchedepisodes");
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
        if (XFILE::CFile::Exists(pItem->GetCachedSeasonThumb()))
          pItem->SetThumbnailImage(pItem->GetCachedSeasonThumb());
      }
      break;
    default:
      break;
  }

  if (pItem)
  {
    pItem->m_bIsFolder = true;
    pItem->SetSpecialSort(g_advancedSettings.m_bVideoLibraryAllItemsOnBottom ? SORT_ON_BOTTOM : SORT_ON_TOP);
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }
}

bool CDirectoryNode::CanCache()
{
  //  Only cache the directorys in the root
  //NODE_TYPE childnode=GetChildType();
  //NODE_TYPE node=GetType();

  // something should probably be cached
  return true;//(childnode==NODE_TYPE_TITLE_MOVIES || childnode==NODE_TYPE_EPISODES || childnode == NODE_TYPE_SEASONS || childnode == NODE_TYPE_TITLE_TVSHOWS);
}

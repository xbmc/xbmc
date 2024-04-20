/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNode.h"

#include "DirectoryNodeEpisodes.h"
#include "DirectoryNodeGrouped.h"
#include "DirectoryNodeInProgressTvShows.h"
#include "DirectoryNodeMoviesOverview.h"
#include "DirectoryNodeMusicVideosOverview.h"
#include "DirectoryNodeOverview.h"
#include "DirectoryNodeRecentlyAddedEpisodes.h"
#include "DirectoryNodeRecentlyAddedMovies.h"
#include "DirectoryNodeRecentlyAddedMusicVideos.h"
#include "DirectoryNodeRoot.h"
#include "DirectoryNodeSeasons.h"
#include "DirectoryNodeTitleMovies.h"
#include "DirectoryNodeTitleMusicVideos.h"
#include "DirectoryNodeTitleTvShows.h"
#include "DirectoryNodeTvShowsOverview.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "QueryParams.h"
#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

//  Constructor is protected use ParseURL()
CDirectoryNode::CDirectoryNode(NODE_TYPE Type, const std::string& strName, CDirectoryNode* pParent)
{
  m_Type = Type;
  m_strName = strName;
  m_pParent = pParent;
}

CDirectoryNode::~CDirectoryNode()
{
  delete m_pParent, m_pParent = nullptr;
}

//  Parses a given path and returns the current node of the path
CDirectoryNode* CDirectoryNode::ParseURL(const std::string& strPath)
{
  CURL url(strPath);

  std::string strDirectory = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(strDirectory);

  std::vector<std::string> Path = StringUtils::Tokenize(strDirectory, '/');
  // we always have a root node, it is special and has a path of ""
  Path.insert(Path.begin(), "");

  CDirectoryNode *pNode = nullptr;
  CDirectoryNode *pParent = nullptr;
  NODE_TYPE NodeType = NODE_TYPE_ROOT;
  // loop down the dir path, creating a node with a parent.
  // if we hit a child type of NODE_TYPE_NONE, then we are done.
  for (size_t i = 0; i < Path.size() && NodeType != NODE_TYPE_NONE; ++i)
  {
    pNode = CDirectoryNode::CreateNode(NodeType, Path[i], pParent);
    NodeType = pNode ? pNode->GetChildType() : NODE_TYPE_NONE;
    pParent = pNode;
  }

  // Add all the additional URL options to the last node
  if (pNode)
    pNode->AddOptions(url.GetOptions());

  return pNode;
}

//  returns the database ids of the path,
void CDirectoryNode::GetDatabaseInfo(const std::string& strPath, CQueryParams& params)
{
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));

  if (!pNode)
    return;

  pNode->CollectQueryParams(params);
}

//  Create a node object
CDirectoryNode* CDirectoryNode::CreateNode(NODE_TYPE Type, const std::string& strName, CDirectoryNode* pParent)
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
  case NODE_TYPE_VIDEOVERSIONS:
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
  case NODE_TYPE_INPROGRESS_TVSHOWS:
    return new CDirectoryNodeInProgressTvShows(strName,pParent);
  case NODE_TYPE_TITLE_MUSICVIDEOS:
    return new CDirectoryNodeTitleMusicVideos(strName,pParent);
  default:
    break;
  }

  return nullptr;
}

//  Current node name
const std::string& CDirectoryNode::GetName() const
{
  return m_strName;
}

int CDirectoryNode::GetID() const
{
  return atoi(m_strName.c_str());
}

std::string CDirectoryNode::GetLocalizedName() const
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
  m_pParent = nullptr;
}

//  should be overloaded by a derived class
//  to get the content of a node. Will be called
//  by GetChilds() of a parent node
bool CDirectoryNode::GetContent(CFileItemList& items) const
{
  return false;
}

//  Creates a videodb url
std::string CDirectoryNode::BuildPath() const
{
  std::vector<std::string> array;

  if (!m_strName.empty())
    array.insert(array.begin(), m_strName);

  CDirectoryNode* pParent=m_pParent;
  while (pParent != nullptr)
  {
    const std::string& strNodeName=pParent->GetName();
    if (!strNodeName.empty())
      array.insert(array.begin(), strNodeName);

    pParent = pParent->GetParent();
  }

  std::string strPath="videodb://";
  for (int i = 0; i < static_cast<int>(array.size()); ++i)
    strPath += array[i]+"/";

  std::string options = m_options.GetOptionsString();
  if (!options.empty())
    strPath += "?" + options;

  return strPath;
}

void CDirectoryNode::AddOptions(const std::string &options)
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
  while (pParent != nullptr)
  {
    params.SetQueryParam(pParent->GetType(), pParent->GetName());
    pParent = pParent->GetParent();
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

  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::CreateNode(GetChildType(), "", this));

  bool bSuccess=false;
  if (pNode)
  {
    pNode->m_options = m_options;
    bSuccess = pNode->GetContent(items);
    if (bSuccess)
    {
      if (CanCache())
        items.SetCacheToDisc(CFileItemList::CACHE_ALWAYS);
    }
    else
      items.Clear();

    pNode->RemoveParent();
  }

  return bSuccess;
}

bool CDirectoryNode::CanCache() const
{
  // no caching is required - the list is cached in CGUIMediaWindow::GetDirectory
  // if it was slow to fetch anyway.
  return false;
}

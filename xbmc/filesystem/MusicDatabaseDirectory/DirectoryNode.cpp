/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNode.h"

#include "DirectoryNodeAlbum.h"
#include "DirectoryNodeAlbumRecentlyAdded.h"
#include "DirectoryNodeAlbumRecentlyAddedSong.h"
#include "DirectoryNodeAlbumRecentlyPlayed.h"
#include "DirectoryNodeAlbumRecentlyPlayedSong.h"
#include "DirectoryNodeAlbumTop100.h"
#include "DirectoryNodeAlbumTop100Song.h"
#include "DirectoryNodeArtist.h"
#include "DirectoryNodeDiscs.h"
#include "DirectoryNodeGrouped.h"
#include "DirectoryNodeOverview.h"
#include "DirectoryNodeRoot.h"
#include "DirectoryNodeSingles.h"
#include "DirectoryNodeSong.h"
#include "DirectoryNodeSongTop100.h"
#include "DirectoryNodeTop100.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "QueryParams.h"
#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace XFILE::MUSICDATABASEDIRECTORY;

//  Constructor is protected use ParseURL()
CDirectoryNode::CDirectoryNode(NODE_TYPE Type, const std::string& strName, CDirectoryNode* pParent)
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
CDirectoryNode* CDirectoryNode::ParseURL(const std::string& strPath)
{
  CURL url(strPath);

  std::string strDirectory=url.GetFileName();
  URIUtils::RemoveSlashAtEnd(strDirectory);

  std::vector<std::string> Path = StringUtils::Split(strDirectory, '/');
  Path.insert(Path.begin(), "");

  CDirectoryNode* pNode = nullptr;
  CDirectoryNode* pParent = nullptr;
  NODE_TYPE NodeType = NODE_TYPE_ROOT;

  for (int i=0; i < static_cast<int>(Path.size()); ++i)
  {
    pNode = CreateNode(NodeType, Path[i], pParent);
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

bool CDirectoryNode::GetNodeInfo(const std::string& strPath,
                                 NODE_TYPE& type,
                                 NODE_TYPE& childtype,
                                 CQueryParams& params)
{
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));
  if (!pNode)
    return false;

  type = pNode->GetType();
  childtype = pNode->GetChildType();
  pNode->CollectQueryParams(params);

  return true;
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
  case NODE_TYPE_SOURCE:
  case NODE_TYPE_ROLE:
  case NODE_TYPE_YEAR:
    return new CDirectoryNodeGrouped(Type, strName, pParent);
  case NODE_TYPE_DISC:
    return new CDirectoryNodeDiscs(strName, pParent);
  case NODE_TYPE_ARTIST:
    return new CDirectoryNodeArtist(strName, pParent);
  case NODE_TYPE_ALBUM:
    return new CDirectoryNodeAlbum(strName, pParent);
  case NODE_TYPE_SONG:
    return new CDirectoryNodeSong(strName, pParent);
  case NODE_TYPE_SINGLES:
    return new CDirectoryNodeSingles(strName, pParent);
  case NODE_TYPE_TOP100:
    return new CDirectoryNodeTop100(strName, pParent);
  case NODE_TYPE_ALBUM_TOP100:
    return new CDirectoryNodeAlbumTop100(strName, pParent);
  case NODE_TYPE_ALBUM_TOP100_SONGS:
    return new CDirectoryNodeAlbumTop100Song(strName, pParent);
  case NODE_TYPE_SONG_TOP100:
    return new CDirectoryNodeSongTop100(strName, pParent);
  case NODE_TYPE_ALBUM_RECENTLY_ADDED:
    return new CDirectoryNodeAlbumRecentlyAdded(strName, pParent);
  case NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS:
    return new CDirectoryNodeAlbumRecentlyAddedSong(strName, pParent);
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
    return new CDirectoryNodeAlbumRecentlyPlayed(strName, pParent);
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS:
    return new CDirectoryNodeAlbumRecentlyPlayedSong(strName, pParent);
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

//  Creates a musicdb url
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

    pParent=pParent->GetParent();
  }

  std::string strPath="musicdb://";
  for (int i = 0; i < static_cast<int>(array.size()); ++i)
    strPath+=array[i]+"/";

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

  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::CreateNode(GetChildType(), "", this));

  bool bSuccess=false;
  if (pNode)
  {
    pNode->m_options = m_options;
    bSuccess=pNode->GetContent(items);
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
  // JM: No need to cache these views, as caching is added in the mediawindow baseclass for anything that takes
  //     longer than a second
  return false;
}

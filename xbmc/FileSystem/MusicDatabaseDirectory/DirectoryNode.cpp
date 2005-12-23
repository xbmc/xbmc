#include "../../stdafx.h"
#include "DirectoryNode.h"
#include "../../util.h"
#include "QueryParams.h"
#include "DirectoryNodeRoot.h"
#include "DirectoryNodeOverview.h"
#include "DirectoryNodeGenre.h"
#include "DirectoryNodeArtist.h"
#include "DirectoryNodeAlbum.h"
#include "DirectoryNodeSong.h"
#include "DirectoryNodeAlbumRecentlyAdded.h"
#include "DirectoryNodeAlbumRecentlyAddedSong.h"
#include "DirectoryNodeAlbumRecentlyPlayed.h"
#include "DirectoryNodeAlbumRecentlyPlayedSong.h"
#include "DirectoryNodeTop100.h"
#include "DirectoryNodeSongTop100.h"
#include "DirectoryNodeAlbumTop100.h"
#include "DirectoryNodeAlbumTop100Song.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

//  Constructor is protected use ParseURL()
CDirectoryNode::CDirectoryNode(NODE_TYPE Type, const CStdString& strName, CDirectoryNode* pParent)
{
  m_Type=Type;
  m_strName=strName;
  m_pParent=pParent;
}

CDirectoryNode::~CDirectoryNode()
{
  if (m_pParent)
    delete m_pParent;
}

//  Parses a given path and returns the current node of the path
CDirectoryNode* CDirectoryNode::ParseURL(const CStdString& strPath)
{
  CURL url(strPath);

  CStdString strDirectory=url.GetFileName();
  if (CUtil::HasSlashAtEnd(strDirectory))
    strDirectory.Delete(strDirectory.size()-1);

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
    NodeType=pNode->GetChildType();
    pParent=pNode;
  }

  return pNode;
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
  case NODE_TYPE_ARTIST:
    return new CDirectoryNodeArtist(strName, pParent);
  case NODE_TYPE_ALBUM:
    return new CDirectoryNodeAlbum(strName, pParent);
  case NODE_TYPE_SONG:
    return new CDirectoryNodeSong(strName, pParent);
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

//  returns the database id of the node,
//  can be used in combination with GetType()
//  to get the real object from the musicdatabase
long CDirectoryNode::GetDatabaseId()
{
  return atol(GetName().c_str());
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

//  Creates a musicdb url
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

  CStdString strPath="musicdb://";
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
  auto_ptr<CDirectoryNode> pNode(CDirectoryNode::CreateNode(GetChildType(), "", this));

  bool bSuccess=false;
  if (pNode.get())
  {
    bSuccess=pNode->GetContent(items);
    pNode->RemoveParent();
  }

  return bSuccess;
}

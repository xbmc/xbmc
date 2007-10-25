#include "stdafx.h"
#include "DirectoryNode.h"
#include "../../Util.h"
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
#include "DirectoryNodeAlbumCompilations.h"
#include "DirectoryNodeAlbumCompilationsSongs.h"
#include "DirectoryNodeYear.h"
#include "DirectoryNodeYearAlbum.h"
#include "DirectoryNodeYearSong.h"

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
  case NODE_TYPE_ALBUM_COMPILATIONS:
    return new CDirectoryNodeAlbumCompilations(strName, pParent);
  case NODE_TYPE_ALBUM_COMPILATIONS_SONGS:
    return new CDirectoryNodeAlbumCompilationsSongs(strName, pParent);
  case NODE_TYPE_YEAR:
    return new CDirectoryNodeYear(strName, pParent);
  case NODE_TYPE_YEAR_ALBUM:
    return new CDirectoryNodeYearAlbum(strName, pParent);
  case NODE_TYPE_YEAR_SONG:
    return new CDirectoryNodeYearSong(strName, pParent);
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
        items.SetCacheToDisc(true);
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
  CFileItem* pItem=NULL;

  // always hide "all" items
  if (g_advancedSettings.m_bMusicLibraryHideAllItems)
    return;

  // no need for "all" item when only one item
  if (items.Size() == 1 || items.Size() == 2 && items[0]->IsParentFolder())
    return;

  switch (GetChildType())
  {
    //  Have no queuing folder
  case NODE_TYPE_ROOT:
  case NODE_TYPE_OVERVIEW:
  case NODE_TYPE_TOP100:
    break;

  /* no need for all genres
  case NODE_TYPE_GENRE:
    pItem = new CFileItem(g_localizeStrings.Get(15105));  // "All Genres"
    pItem->m_strPath = BuildPath() + "-1/";
    break;
  */

  case NODE_TYPE_ARTIST:
    if (GetType() == NODE_TYPE_OVERVIEW) return;
    pItem = new CFileItem(g_localizeStrings.Get(15103));  // "All Artists"
    pItem->m_strPath = BuildPath() + "-1/";
    break;

    //  All album related nodes
  case NODE_TYPE_ALBUM:
    if (GetType() == NODE_TYPE_OVERVIEW) return;
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
  case NODE_TYPE_ALBUM_RECENTLY_ADDED:
  case NODE_TYPE_ALBUM_COMPILATIONS:
  case NODE_TYPE_ALBUM_TOP100:
  case NODE_TYPE_YEAR_ALBUM:
    pItem = new CFileItem(g_localizeStrings.Get(15102));  // "All Albums"
    pItem->m_strPath = BuildPath() + "-1/";
    break;

    //  All song related nodes
/*  case NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS:
  case NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS:
  case NODE_TYPE_ALBUM_COMPILATIONS_SONGS:
  case NODE_TYPE_ALBUM_TOP100_SONGS:
  case NODE_TYPE_SONG_TOP100:
  case NODE_TYPE_SONG:
    pItem = new CFileItem(g_localizeStrings.Get(15104));  // "All Songs"
    pItem->m_strPath = BuildPath() + "-1/";
    break;*/
  default:
    break;
  }

  if (pItem)
  {
    pItem->m_bIsFolder = true;
    CStdString strFake;
    //  HACK: This item will stay on top of a list
    strFake.Format("%c", 0x01);
    if (g_advancedSettings.m_bMusicLibraryAllItemsOnBottom)
      //  HACK: This item will stay on bottom of a list
      strFake.Format("%c", 0xff);
    pItem->GetMusicInfoTag()->SetAlbum(strFake);
    pItem->GetMusicInfoTag()->SetArtist(strFake);
    pItem->GetMusicInfoTag()->SetTitle(strFake);
    pItem->GetMusicInfoTag()->SetGenre(strFake);
    pItem->SetCanQueue(false);
    pItem->SetLabelPreformated(true);
    if (g_advancedSettings.m_bMusicLibraryAllItemsOnBottom)
      items.Add(pItem);
    else
      items.AddFront(pItem, (items.Size() > 0 && items[0]->IsParentFolder()) ? 1 : 0);
  }
}

bool CDirectoryNode::CanCache()
{
  // JM: No need to cache these views, as caching is added in the mediawindow baseclass for anything that takes
  //     longer than a second
  return false;
}

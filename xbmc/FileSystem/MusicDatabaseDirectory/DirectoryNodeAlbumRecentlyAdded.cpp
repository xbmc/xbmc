#include "../../stdafx.h"
#include "DirectoryNodeAlbumRecentlyAdded.h"
#include "../../MusicDatabase.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CDirectoryNodeAlbumRecentlyAdded::CDirectoryNodeAlbumRecentlyAdded(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_ALBUM_RECENTLY_ADDED, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeAlbumRecentlyAdded::GetChildType()
{
  if (GetName()=="-1")
    return NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS;

  return NODE_TYPE_SONG;
}

bool CDirectoryNodeAlbumRecentlyAdded::GetContent(CFileItemList& items)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  VECALBUMS albums;
  if (!musicdatabase.GetRecentlyAddedAlbums(albums))
  {
    musicdatabase.Close();
    return false;
  }

  // add "All Albums"
  CFileItem* pItem = new CFileItem(g_localizeStrings.Get(15102));
  pItem->m_strPath=BuildPath() + "-1/";
  pItem->m_bIsFolder = true;
  pItem->SetCanQueue(false);
  items.Add(pItem);

  for (int i=0; i<(int)albums.size(); ++i)
  {
    CAlbum& album=albums[i];
    CFileItem* pItem=new CFileItem(album);
    pItem->m_strPath=BuildPath();
    CStdString strDir;
    strDir.Format("%ld/", album.idAlbum);
    pItem->m_strPath+=strDir;
    items.Add(pItem);
  }

  musicdatabase.Close();
  return true;
}

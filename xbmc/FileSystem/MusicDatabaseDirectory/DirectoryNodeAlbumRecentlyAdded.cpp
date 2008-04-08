#include "stdafx.h"
#include "DirectoryNodeAlbumRecentlyAdded.h"
#include "MusicDatabase.h"
#include "FileItem.h"

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

  for (int i=0; i<(int)albums.size(); ++i)
  {
    CAlbum& album=albums[i];
    CStdString strDir;
    strDir.Format("%s%ld/", BuildPath().c_str(), album.idAlbum);
    CFileItem* pItem=new CFileItem(strDir, album);
    items.Add(pItem);
  }

  musicdatabase.Close();
  return true;
}

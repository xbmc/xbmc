#include "stdafx.h"
#include "DirectoryNodeAlbumTop100Song.h"
#include "../../MusicDatabase.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CDirectoryNodeAlbumTop100Song::CDirectoryNodeAlbumTop100Song(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_ALBUM_TOP100_SONGS, strName, pParent)
{

}

bool CDirectoryNodeAlbumTop100Song::GetContent(CFileItemList& items)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CStdString strBaseDir=BuildPath();
  bool bSuccess=musicdatabase.GetTop100AlbumSongs(strBaseDir, items);

  musicdatabase.Close();

  return bSuccess;
}

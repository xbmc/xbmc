#include "stdafx.h"
#include "DirectoryNodeAlbumCompilations.h"
#include "QueryParams.h"
#include "MusicDatabase.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CDirectoryNodeAlbumCompilations::CDirectoryNodeAlbumCompilations(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_ALBUM_COMPILATIONS, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeAlbumCompilations::GetChildType()
{
  if (GetName()=="-1")
    return NODE_TYPE_ALBUM_COMPILATIONS_SONGS;

  return NODE_TYPE_SONG;
}

bool CDirectoryNodeAlbumCompilations::GetContent(CFileItemList& items)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess=musicdatabase.GetVariousArtistsAlbums(BuildPath(), items);

  musicdatabase.Close();

  return bSuccess;
}

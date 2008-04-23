#include "stdafx.h"
#include "DirectoryNodeAlbumCompilationsSongs.h"
#include "QueryParams.h"
#include "MusicDatabase.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CDirectoryNodeAlbumCompilationsSongs::CDirectoryNodeAlbumCompilationsSongs(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_ALBUM_COMPILATIONS_SONGS, strName, pParent)
{

}


bool CDirectoryNodeAlbumCompilationsSongs::GetContent(CFileItemList& items)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess=musicdatabase.GetVariousArtistsAlbumsSongs(BuildPath(), items);

  musicdatabase.Close();

  return bSuccess;
}

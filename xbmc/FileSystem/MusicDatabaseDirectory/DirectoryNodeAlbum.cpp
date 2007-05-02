#include "stdafx.h"
#include "DirectoryNodeAlbum.h"
#include "QueryParams.h"
#include "../../MusicDatabase.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CDirectoryNodeAlbum::CDirectoryNodeAlbum(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_ALBUM, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeAlbum::GetChildType()
{
  return NODE_TYPE_SONG;
}

bool CDirectoryNodeAlbum::GetContent(CFileItemList& items)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess=musicdatabase.GetAlbumsNav(BuildPath(), items, params.GetGenreId(), params.GetArtistId());

  musicdatabase.Close();

  return bSuccess;
}

#include "stdafx.h"
#include "DirectoryNodeSong.h"
#include "QueryParams.h"
#include "MusicDatabase.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CDirectoryNodeSong::CDirectoryNodeSong(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_SONG, strName, pParent)
{

}

bool CDirectoryNodeSong::GetContent(CFileItemList& items)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  CStdString strBaseDir=BuildPath();
  bool bSuccess=musicdatabase.GetSongsNav(strBaseDir, items, params.GetGenreId(), params.GetArtistId(), params.GetAlbumId());

  musicdatabase.Close();

  return bSuccess;
}

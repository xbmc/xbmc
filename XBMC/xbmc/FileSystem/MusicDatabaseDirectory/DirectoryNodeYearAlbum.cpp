#include "stdafx.h"
#include "DirectoryNodeYearAlbum.h"
#include "QueryParams.h"
#include "../../MusicDatabase.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CDirectoryNodeYearAlbum::CDirectoryNodeYearAlbum(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_YEAR_ALBUM, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeYearAlbum::GetChildType()
{
  if (GetName()=="-1")
    return NODE_TYPE_YEAR_SONG;

  return NODE_TYPE_SONG;
}

bool CDirectoryNodeYearAlbum::GetContent(CFileItemList& items)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess=musicdatabase.GetAlbumsByYear(BuildPath(), items, params.GetYear());

  musicdatabase.Close();

  return bSuccess;
}

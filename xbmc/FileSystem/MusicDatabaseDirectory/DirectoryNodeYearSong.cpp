#include "stdafx.h"
#include "DirectoryNodeYearSong.h"
#include "QueryParams.h"
#include "../../MusicDatabase.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CDirectoryNodeYearSong::CDirectoryNodeYearSong(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_YEAR_SONG, strName, pParent)
{

}

bool CDirectoryNodeYearSong::GetContent(CFileItemList& items)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  CStdString strBaseDir=BuildPath();
  bool bSuccess=musicdatabase.GetSongsByYear(strBaseDir, items, params.GetYear());

  musicdatabase.Close();

  return bSuccess;
}

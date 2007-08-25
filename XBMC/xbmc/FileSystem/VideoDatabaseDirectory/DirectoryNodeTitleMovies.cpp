#include "stdafx.h"
#include "DirectoryNodeTitleMovies.h"
#include "QueryParams.h"
#include "../../VideoDatabase.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeTitleMovies::CDirectoryNodeTitleMovies(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_TITLE_MOVIES, strName, pParent)
{

}

bool CDirectoryNodeTitleMovies::GetContent(CFileItemList& items)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  CStdString strBaseDir=BuildPath();
  bool bSuccess=videodatabase.GetTitlesNav(strBaseDir, items, params.GetGenreId(), params.GetYear(), params.GetActorId(), params.GetDirectorId(),params.GetStudioId());

  videodatabase.Close();

  return bSuccess;
}

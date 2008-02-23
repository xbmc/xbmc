#include "stdafx.h"
#include "DirectoryNodeTitleTvShows.h"
#include "QueryParams.h"
#include "../../VideoDatabase.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeTitleTvShows::CDirectoryNodeTitleTvShows(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_TITLE_TVSHOWS, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeTitleTvShows::GetChildType()
{
  if (GetParent()->GetType() == NODE_TYPE_OVERVIEW)
    return NODE_TYPE_TITLE_TVSHOWS;
  else
    return NODE_TYPE_SEASONS;
}

bool CDirectoryNodeTitleTvShows::GetContent(CFileItemList& items)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess=videodatabase.GetTvShowsNav(BuildPath(), items, params.GetGenreId(), params.GetYear(), params.GetActorId(), params.GetDirectorId());

  videodatabase.Close();

  return bSuccess;
}

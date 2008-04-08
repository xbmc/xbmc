#include "stdafx.h"
#include "DirectoryNodeEpisodes.h"
#include "QueryParams.h"
#include "VideoDatabase.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeEpisodes::CDirectoryNodeEpisodes(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_EPISODES, strName, pParent)
{

}

bool CDirectoryNodeEpisodes::GetContent(CFileItemList& items)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  CStdString strBaseDir=BuildPath();
  bool bSuccess=videodatabase.GetEpisodesNav(strBaseDir, items, params.GetGenreId(), params.GetYear(), params.GetActorId(), params.GetDirectorId(), params.GetTvShowId(), params.GetSeason());

  videodatabase.Close();

  return bSuccess;
}

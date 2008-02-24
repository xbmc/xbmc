#include "stdafx.h"
#include "DirectoryNodeTitleMusicVideos.h"
#include "QueryParams.h"
#include "../../VideoDatabase.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeTitleMusicVideos::CDirectoryNodeTitleMusicVideos(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_TITLE_MUSICVIDEOS, strName, pParent)
{

}

bool CDirectoryNodeTitleMusicVideos::GetContent(CFileItemList& items)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  CStdString strBaseDir=BuildPath();
  bool bSuccess=videodatabase.GetMusicVideosNav(strBaseDir, items, params.GetGenreId(), params.GetYear(), params.GetActorId(), params.GetDirectorId(),params.GetStudioId());

  videodatabase.Close();

  return bSuccess;
}

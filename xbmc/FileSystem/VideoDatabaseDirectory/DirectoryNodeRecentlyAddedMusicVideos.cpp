#include "stdafx.h"
#include "DirectoryNodeRecentlyAddedMusicVideos.h"
#include "QueryParams.h"
#include "../../VideoDatabase.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeRecentlyAddedMusicVideos::CDirectoryNodeRecentlyAddedMusicVideos(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeRecentlyAddedMusicVideos::GetChildType()
{
  return NODE_TYPE_TITLE_MUSICVIDEOS;
}

bool CDirectoryNodeRecentlyAddedMusicVideos::GetContent(CFileItemList& items)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CStdString strBaseDir=BuildPath();
  bool bSuccess=videodatabase.GetRecentlyAddedMusicVideosNav(strBaseDir, items);

  videodatabase.Close();

  return bSuccess;
}


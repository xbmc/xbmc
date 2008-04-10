#include "stdafx.h"
#include "DirectoryNodeRecentlyAddedMovies.h"
#include "QueryParams.h"
#include "VideoDatabase.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeRecentlyAddedMovies::CDirectoryNodeRecentlyAddedMovies(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_RECENTLY_ADDED_MOVIES, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeRecentlyAddedMovies::GetChildType()
{
  return NODE_TYPE_TITLE_MOVIES;
}

bool CDirectoryNodeRecentlyAddedMovies::GetContent(CFileItemList& items)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CStdString strBaseDir=BuildPath();
  bool bSuccess=videodatabase.GetRecentlyAddedMoviesNav(strBaseDir, items);

  videodatabase.Close();

  return bSuccess;
}

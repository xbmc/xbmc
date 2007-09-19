#include "stdafx.h"
#include "DirectoryNodeGenre.h"
#include "QueryParams.h"
#include "../../VideoDatabase.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeGenre::CDirectoryNodeGenre(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_GENRE, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeGenre::GetChildType()
{
  CQueryParams params;
  CollectQueryParams(params);
  if (params.GetContentType() == VIDEODB_CONTENT_MOVIES)
    return NODE_TYPE_TITLE_MOVIES;
  if (params.GetContentType() == VIDEODB_CONTENT_MUSICVIDEOS)
    return NODE_TYPE_TITLE_MUSICVIDEOS;

  return NODE_TYPE_TITLE_TVSHOWS;
}

bool CDirectoryNodeGenre::GetContent(CFileItemList& items)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess=videodatabase.GetGenresNav(BuildPath(), items, params.GetContentType());

  videodatabase.Close();

  return bSuccess;
}

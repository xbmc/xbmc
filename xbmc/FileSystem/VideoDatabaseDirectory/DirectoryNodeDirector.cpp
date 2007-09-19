#include "stdafx.h"
#include "DirectoryNodeDirector.h"
#include "QueryParams.h"
#include "../../VideoDatabase.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeDirector::CDirectoryNodeDirector(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_DIRECTOR, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeDirector::GetChildType()
{
  CQueryParams params;
  CollectQueryParams(params);
  if (params.GetContentType() == VIDEODB_CONTENT_MOVIES)
    return NODE_TYPE_TITLE_MOVIES;
  if (params.GetContentType() == VIDEODB_CONTENT_MUSICVIDEOS)
    return NODE_TYPE_TITLE_MUSICVIDEOS;

  return NODE_TYPE_TITLE_TVSHOWS;
}

bool CDirectoryNodeDirector::GetContent(CFileItemList& items)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess=videodatabase.GetDirectorsNav(BuildPath(), items, params.GetContentType());

  videodatabase.Close();

  return bSuccess;
}


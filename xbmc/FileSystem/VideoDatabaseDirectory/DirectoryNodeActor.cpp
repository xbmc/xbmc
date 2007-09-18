#include "stdafx.h"
#include "DirectoryNodeActor.h"
#include "QueryParams.h"
#include "../../VideoDatabase.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeActor::CDirectoryNodeActor(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_ACTOR, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeActor::GetChildType()
{
  CQueryParams params;
  CollectQueryParams(params);
  if (params.GetContentType() == VIDEODB_CONTENT_MOVIES)
    return NODE_TYPE_TITLE_MOVIES;
  if (params.GetContentType() == VIDEODB_CONTENT_MUSICVIDEOS)
    return NODE_TYPE_TITLE_MUSICVIDEOS;

  return NODE_TYPE_TITLE_TVSHOWS;
}

bool CDirectoryNodeActor::GetContent(CFileItemList& items)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess=videodatabase.GetActorsNav(BuildPath(), items, params.GetContentType());

  videodatabase.Close();

  return bSuccess;
}

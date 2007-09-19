#include "stdafx.h"
#include "DirectoryNodeStudio.h"
#include "QueryParams.h"
#include "../../VideoDatabase.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeStudio::CDirectoryNodeStudio(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_STUDIO, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeStudio::GetChildType()
{
  CQueryParams params;
  CollectQueryParams(params);
  if (params.GetContentType() == VIDEODB_CONTENT_MOVIES)
    return NODE_TYPE_TITLE_MOVIES;
  if (params.GetContentType() == VIDEODB_CONTENT_MUSICVIDEOS)
    return NODE_TYPE_TITLE_MUSICVIDEOS;

  return NODE_TYPE_TITLE_TVSHOWS;
}

bool CDirectoryNodeStudio::GetContent(CFileItemList& items)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess=videodatabase.GetStudiosNav(BuildPath(), items, params.GetContentType());

  videodatabase.Close();

  return bSuccess;
}

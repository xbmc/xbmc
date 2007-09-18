#include "stdafx.h"
#include "DirectoryNodeYear.h"
#include "QueryParams.h"
#include "../../VideoDatabase.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeYear::CDirectoryNodeYear(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_YEAR, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeYear::GetChildType()
{
  CQueryParams params;
  CollectQueryParams(params);
  if (params.GetContentType() == VIDEODB_CONTENT_MOVIES)
    return NODE_TYPE_TITLE_MOVIES;
  if (params.GetContentType() == VIDEODB_CONTENT_MUSICVIDEOS)
    return NODE_TYPE_TITLE_MUSICVIDEOS;

  return NODE_TYPE_TITLE_TVSHOWS;
}

bool CDirectoryNodeYear::GetContent(CFileItemList& items)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  CStdString strBaseDir=BuildPath();
  bool bSuccess=videodatabase.GetYearsNav(strBaseDir, items, params.GetContentType());

  videodatabase.Close();

  return bSuccess;
}

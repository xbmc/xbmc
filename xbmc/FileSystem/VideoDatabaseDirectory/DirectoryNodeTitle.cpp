#include "../../stdafx.h"
#include "DirectoryNodeTitle.h"
#include "QueryParams.h"
#include "../../VideoDatabase.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeTitle::CDirectoryNodeTitle(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_TITLE, strName, pParent)
{

}

bool CDirectoryNodeTitle::GetContent(CFileItemList& items)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  CStdString strBaseDir=BuildPath();
  bool bSuccess=videodatabase.GetTitlesNav(strBaseDir, items, params.GetGenreId(), params.GetYear(), params.GetActorId());

  videodatabase.Close();

  return bSuccess;
}

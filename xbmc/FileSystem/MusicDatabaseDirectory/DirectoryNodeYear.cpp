#include "stdafx.h"
#include "DirectoryNodeYear.h"
#include "QueryParams.h"
#include "MusicDatabase.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CDirectoryNodeYear::CDirectoryNodeYear(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_YEAR, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeYear::GetChildType()
{
  return NODE_TYPE_YEAR_ALBUM;
}

bool CDirectoryNodeYear::GetContent(CFileItemList& items)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess=musicdatabase.GetYearsNav(BuildPath(), items);

  musicdatabase.Close();

  return bSuccess;
}

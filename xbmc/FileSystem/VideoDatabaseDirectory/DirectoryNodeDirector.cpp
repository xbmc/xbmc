#include "../../stdafx.h"
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
  /* all items should still go to the next filter level
  if (GetName()=="-1")
    return NODE_TYPE_SONG;
    */

  return NODE_TYPE_TITLE;
}

bool CDirectoryNodeDirector::GetContent(CFileItemList& items)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess=videodatabase.GetDirectorsNav(BuildPath(), items);

  videodatabase.Close();

  return bSuccess;
}
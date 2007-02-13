#include "../../stdafx.h"
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
  /* all items should still go to the next filter level
  if (GetName()=="-1")
    return NODE_TYPE_SONG;
    */

  return NODE_TYPE_TITLE;
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
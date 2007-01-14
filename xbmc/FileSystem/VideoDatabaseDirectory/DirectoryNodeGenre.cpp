#include "../../stdafx.h"
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
  /* all items should still go to the next filter level
  if (GetName()=="-1")
    return NODE_TYPE_SONG;
    */

  return NODE_TYPE_TITLE;
}

bool CDirectoryNodeGenre::GetContent(CFileItemList& items)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess=videodatabase.GetGenresNav(BuildPath(), items);

  videodatabase.Close();

  return bSuccess;
}

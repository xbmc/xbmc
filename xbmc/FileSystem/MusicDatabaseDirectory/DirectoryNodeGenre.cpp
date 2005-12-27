#include "../../stdafx.h"
#include "DirectoryNodeGenre.h"
#include "QueryParams.h"
#include "../../MusicDatabase.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CDirectoryNodeGenre::CDirectoryNodeGenre(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_GENRE, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeGenre::GetChildType()
{
  if (GetName()=="-1")
    return NODE_TYPE_SONG;

  return NODE_TYPE_ARTIST;
}

bool CDirectoryNodeGenre::GetContent(CFileItemList& items)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess=musicdatabase.GetGenresNav(BuildPath(), items);

  musicdatabase.Close();

  return bSuccess;
}

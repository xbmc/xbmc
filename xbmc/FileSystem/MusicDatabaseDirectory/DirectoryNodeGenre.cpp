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
  if (!musicdatabase.GetGenresNav(BuildPath(), items))
  {
    musicdatabase.Close();
    return false;
  }

  // add "All Genres"
  CFileItem* pItem = new CFileItem(g_localizeStrings.Get(15105));
  pItem->m_strPath=BuildPath() + "-1/";
  pItem->m_bIsFolder = true;
  pItem->SetCanQueue(false);
  items.Add(pItem);

  musicdatabase.Close();
  return true;
}

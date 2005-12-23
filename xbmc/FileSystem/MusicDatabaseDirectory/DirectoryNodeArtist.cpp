#include "../../stdafx.h"
#include "DirectoryNodeArtist.h"
#include "QueryParams.h"
#include "../../MusicDatabase.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CDirectoryNodeArtist::CDirectoryNodeArtist(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_ARTIST, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeArtist::GetChildType()
{
  if (GetName()=="-1")
    return NODE_TYPE_SONG;

  return NODE_TYPE_ALBUM;
}

bool CDirectoryNodeArtist::GetContent(CFileItemList& items)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);
  if (!musicdatabase.GetArtistsNav(BuildPath(), items, params.GetGenreId()))
  {
    musicdatabase.Close();
    return false;
  }

  // add "All Artists"
  CFileItem* pItem = new CFileItem(g_localizeStrings.Get(15103));
  pItem->m_strPath=BuildPath() + "-1/";
  pItem->m_bIsFolder = true;
  pItem->SetCanQueue(false);
  items.Add(pItem);

  musicdatabase.Close();
  return true;
}

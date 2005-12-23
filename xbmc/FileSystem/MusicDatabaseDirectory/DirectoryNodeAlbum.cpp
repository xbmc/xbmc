#include "../../stdafx.h"
#include "DirectoryNodeAlbum.h"
#include "QueryParams.h"
#include "../../MusicDatabase.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CDirectoryNodeAlbum::CDirectoryNodeAlbum(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_ALBUM, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeAlbum::GetChildType()
{
  return NODE_TYPE_SONG;
}

bool CDirectoryNodeAlbum::GetContent(CFileItemList& items)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);
  if (!musicdatabase.GetAlbumsNav(BuildPath(), items, params.GetGenreId(), params.GetArtistId()))
  {
    musicdatabase.Close();
    return false;
  }

  // add "All Albums"
  CFileItem* pItem = new CFileItem(g_localizeStrings.Get(15102));
  pItem->m_strPath=BuildPath() + "-1/";
  //  HACK: This item will stay at the top of a list
  pItem->m_musicInfoTag.SetAlbum("*");
  pItem->m_musicInfoTag.SetArtist("*");
  pItem->m_bIsFolder = true;
  pItem->SetCanQueue(false);
  items.Add(pItem);

  musicdatabase.Close();
  return true;
}

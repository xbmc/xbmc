#include "stdafx.h"
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
  return NODE_TYPE_ALBUM;
}

bool CDirectoryNodeArtist::GetContent(CFileItemList& items)
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess = musicdatabase.GetArtistsNav(BuildPath(), items, params.GetGenreId(), g_advancedSettings.m_bMusicLibraryHideCompilationArtists);

  musicdatabase.Close();

  return bSuccess;
}

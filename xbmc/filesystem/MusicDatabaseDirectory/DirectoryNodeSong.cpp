/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeSong.h"

#include "QueryParams.h"
#include "music/MusicDatabase.h"

using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeSong::CDirectoryNodeSong(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_SONG, strName, pParent)
{

}

bool CDirectoryNodeSong::GetContent(CFileItemList& items) const
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  std::string strBaseDir=BuildPath();
  bool bSuccess=musicdatabase.GetSongsNav(strBaseDir, items, params.GetGenreId(), params.GetArtistId(), params.GetAlbumId());

  musicdatabase.Close();

  return bSuccess;
}

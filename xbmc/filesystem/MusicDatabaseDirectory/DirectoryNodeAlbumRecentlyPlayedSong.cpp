/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeAlbumRecentlyPlayedSong.h"

#include "music/MusicDatabase.h"

using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeAlbumRecentlyPlayedSong::CDirectoryNodeAlbumRecentlyPlayedSong(
    const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::ALBUM_RECENTLY_PLAYED_SONGS, strName, pParent)
{

}

bool CDirectoryNodeAlbumRecentlyPlayedSong::GetContent(CFileItemList& items) const
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  std::string strBaseDir=BuildPath();
  bool bSuccess=musicdatabase.GetRecentlyPlayedAlbumSongs(strBaseDir, items);

  musicdatabase.Close();

  return bSuccess;
}

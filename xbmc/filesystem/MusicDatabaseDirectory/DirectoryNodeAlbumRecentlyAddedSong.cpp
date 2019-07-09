/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeAlbumRecentlyAddedSong.h"

#include "music/MusicDatabase.h"

using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeAlbumRecentlyAddedSong::CDirectoryNodeAlbumRecentlyAddedSong(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS, strName, pParent)
{

}

bool CDirectoryNodeAlbumRecentlyAddedSong::GetContent(CFileItemList& items) const
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  std::string strBaseDir=BuildPath();
  bool bSuccess=musicdatabase.GetRecentlyAddedAlbumSongs(strBaseDir, items);

  musicdatabase.Close();

  return bSuccess;
}

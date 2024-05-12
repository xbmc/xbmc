/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlaylistDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "playlists/PlayList.h"

using namespace KODI;
using namespace XFILE;

CPlaylistDirectory::CPlaylistDirectory() = default;

CPlaylistDirectory::~CPlaylistDirectory() = default;

bool CPlaylistDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  PLAYLIST::Id playlistId = PLAYLIST::Id::TYPE_NONE;
  if (url.IsProtocol("playlistmusic"))
    playlistId = PLAYLIST::Id::TYPE_MUSIC;
  else if (url.IsProtocol("playlistvideo"))
    playlistId = PLAYLIST::Id::TYPE_VIDEO;

  if (playlistId == PLAYLIST::Id::TYPE_NONE)
    return false;

  const PLAYLIST::CPlayList& playlist = CServiceBroker::GetPlaylistPlayer().GetPlaylist(playlistId);
  items.Reserve(playlist.size());

  for (int i = 0; i < playlist.size(); ++i)
  {
    CFileItemPtr item = playlist[i];
    item->SetProperty("playlistposition", i);
    item->SetProperty("playlisttype", static_cast<int>(playlistId));
    //item->m_iprogramCount = i; // the programCount is set as items are added!
    items.Add(item);
  }

  return true;
}

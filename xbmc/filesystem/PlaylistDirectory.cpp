/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlaylistDirectory.h"

#include "PlayListPlayer.h"
#include "URL.h"
#include "playlists/PlayList.h"

using namespace PLAYLIST;
using namespace XFILE;

CPlaylistDirectory::CPlaylistDirectory() = default;

CPlaylistDirectory::~CPlaylistDirectory() = default;

bool CPlaylistDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  int playlistTyp=PLAYLIST_NONE;
  if (url.IsProtocol("playlistmusic"))
    playlistTyp=PLAYLIST_MUSIC;
  else if (url.IsProtocol("playlistvideo"))
    playlistTyp=PLAYLIST_VIDEO;

  if (playlistTyp==PLAYLIST_NONE)
    return false;

  CPlayList& playlist = CServiceBroker::GetPlaylistPlayer().GetPlaylist(playlistTyp);
  items.Reserve(playlist.size());

  for (int i = 0; i < playlist.size(); ++i)
  {
    CFileItemPtr item = playlist[i];
    item->SetProperty("playlistposition", i);
    item->SetProperty("playlisttype", playlistTyp);
    //item->m_iprogramCount = i; // the programCount is set as items are added!
    items.Add(item);
  }

  return true;
}

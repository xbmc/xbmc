/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "PlaylistDirectory.h"
#include "PlayListPlayer.h"
#include "URL.h"
#include "PlayList.h"

using namespace PLAYLIST;
using namespace XFILE;

CPlaylistDirectory::CPlaylistDirectory()
{

}

CPlaylistDirectory::~CPlaylistDirectory()
{

}

bool CPlaylistDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURL url(strPath);

  int playlistTyp=PLAYLIST_NONE;
  if (url.GetProtocol()=="playlistmusic")
    playlistTyp=PLAYLIST_MUSIC;
  else if (url.GetProtocol()=="playlistvideo")
    playlistTyp=PLAYLIST_VIDEO;

  if (playlistTyp==PLAYLIST_NONE)
    return false;

  CPlayList& playlist = g_playlistPlayer.GetPlaylist(playlistTyp);
  items.Reserve(playlist.size());

  for (int i = 0; i < playlist.size(); ++i)
  {
    CFileItemPtr item = playlist[i];
    //item->m_iprogramCount = i; // the programCount is set as items are added!
    items.Add(item);
  }

  return true;
}

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIInfoHelper.h"

#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "playlists/PlayList.h"
#include "utils/StringUtils.h"

#include "guiinfo/GUIInfoLabels.h"

namespace GUIINFO
{

std::string CGUIInfoHelper::GetPlaylistLabel(int item, int playlistid /* = PLAYLIST_NONE */)
{
  if (playlistid < PLAYLIST_NONE)
    return std::string();

  PLAYLIST::CPlayListPlayer& player = CServiceBroker::GetPlaylistPlayer();

  int iPlaylist = playlistid == PLAYLIST_NONE ? player.GetCurrentPlaylist() : playlistid;
  switch (item)
  {
    case PLAYLIST_LENGTH:
    {
      return StringUtils::Format("%i", player.GetPlaylist(iPlaylist).size());
    }
    case PLAYLIST_POSITION:
    {
      int currentSong = player.GetCurrentSong();
      if (currentSong > -1)
        return StringUtils::Format("%i", currentSong + 1);
      break;
    }
    case PLAYLIST_RANDOM:
    {
      if (player.IsShuffled(iPlaylist))
        return g_localizeStrings.Get(16041); // 16041: On
      else
        return g_localizeStrings.Get(591); // 591: Off
    }
    case PLAYLIST_REPEAT:
    {
      PLAYLIST::REPEAT_STATE state = player.GetRepeat(iPlaylist);
      if (state == PLAYLIST::REPEAT_ONE)
        return g_localizeStrings.Get(592); // 592: One
      else if (state == PLAYLIST::REPEAT_ALL)
        return g_localizeStrings.Get(593); // 593: All
      else
        return g_localizeStrings.Get(594); // 594: Off
    }
  }
  return std::string();
}

} // namespace GUIINFO

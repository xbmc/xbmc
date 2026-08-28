/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayerIds.h"

using namespace KODI;

namespace JSONRPC
{

PLAYLIST::Id PlayerIdOf(PlayerType player, const PlayerState& state)
{
  PLAYLIST::Id playlistId = state.currentPlaylist;
  if (playlistId == PLAYLIST::Id::TYPE_NONE) // No active playlist, try guessing
    playlistId = state.preferredPlaylist;

  switch (player)
  {
    case Video:
      return playlistId == PLAYLIST::Id::TYPE_NONE ? PLAYLIST::Id::TYPE_VIDEO : playlistId;

    case Audio:
      return playlistId == PLAYLIST::Id::TYPE_NONE ? PLAYLIST::Id::TYPE_MUSIC : playlistId;

    case Picture:
      return PLAYLIST::Id::TYPE_PICTURE;

    default:
      return playlistId;
  }
}

PlayerType PlayerForId(PLAYLIST::Id playerid, const PlayerState& state)
{
  PlayerType player;

  switch (playerid)
  {
    case PLAYLIST::Id::TYPE_VIDEO:
      player = Video;
      break;

    case PLAYLIST::Id::TYPE_MUSIC:
      player = Audio;
      break;

    case PLAYLIST::Id::TYPE_PICTURE:
      player = Picture;
      break;

    default:
      player = None;
      break;
  }

  return PlayerIdOf(player, state) == playerid ? player : None;
}

} // namespace JSONRPC

/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayerIds.h"

#include "utils/Variant.h"

using namespace KODI;

namespace JSONRPC
{

PLAYLIST::Id PlayerIdOf(PlayerType player)
{
  switch (player)
  {
    case Video:
      return PLAYLIST::Id::TYPE_VIDEO;

    case Audio:
      return PLAYLIST::Id::TYPE_MUSIC;

    case Picture:
      return PLAYLIST::Id::TYPE_PICTURE;

    default:
      return PLAYLIST::Id::TYPE_NONE;
  }
}

PlayerType PlayerForId(PLAYLIST::Id playerid)
{
  switch (playerid)
  {
    case PLAYLIST::Id::TYPE_VIDEO:
      return Video;

    case PLAYLIST::Id::TYPE_MUSIC:
      return Audio;

    case PLAYLIST::Id::TYPE_PICTURE:
      return Picture;

    default:
      return None;
  }
}

JSONRPC_STATUS ResolvePlayer(PLAYLIST::Id playerid,
                             PLAYLIST::Id playlistid,
                             const PlayerState& state,
                             PlayerType& player)
{
  const bool byPlayer = playerid != PLAYLIST::Id::TYPE_NONE;
  const bool byPlaylist = playlistid != PLAYLIST::Id::TYPE_NONE;
  if (byPlayer == byPlaylist)
    return InvalidParams;

  if (byPlayer)
  {
    player = PlayerForId(playerid);
    if (player == None)
      return InvalidParams;

    return (state.players & player) != 0 ? OK : Unavailable;
  }

  for (const PlayerType candidate : {Video, Audio, Picture})
  {
    if ((state.players & candidate) != 0 && PlaylistOf(candidate, state) == playlistid)
    {
      player = candidate;
      return OK;
    }
  }

  return Unavailable;
}

void DescribePlayer(CVariant& player, PlayerType type, PLAYLIST::Id playlist)
{
  player["playerid"] = static_cast<int>(PlayerIdOf(type));
  player["playlistid"] =
      static_cast<int>(playlist == PLAYLIST::Id::TYPE_NONE ? PlayerIdOf(type) : playlist);
}

PLAYLIST::Id PlaylistOf(PlayerType player, const PlayerState& state)
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

} // namespace JSONRPC

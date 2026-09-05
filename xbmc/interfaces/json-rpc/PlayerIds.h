/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "JSONRPCUtils.h"
#include "playlists/PlayListTypes.h"

class CVariant;

namespace JSONRPC
{

enum PlayerType
{
  None = 0,
  Video = 0x1,
  Audio = 0x2,
  Picture = 0x4,
  External = 0x8,
  Remote = 0x10
};

/*! \brief Everything a playerid is resolved against.
 */
struct PlayerState
{
  //! The players running, as PlayerType flags.
  int players{None};

  //! The playlist the playlist player is working through, TYPE_NONE when it has none.
  KODI::PLAYLIST::Id currentPlaylist{KODI::PLAYLIST::Id::TYPE_NONE};

  //! The playlist the running player would belong to, TYPE_NONE when nothing plays.
  KODI::PLAYLIST::Id preferredPlaylist{KODI::PLAYLIST::Id::TYPE_NONE};
};

/*! \brief The playerid a client addresses a player by.

 Published by Player.GetActivePlayers and accepted by every other Player method. Shares a
 range with the playlist ids but is independent of the playlist the player is working through.

 \param player the player being addressed
 \return the playerid
 */
KODI::PLAYLIST::Id PlayerIdOf(PlayerType player);

/*! \brief The player a playerid names, running or not.
 \param playerid the playerid as the client gave it
 \return the player, or None when the id names none
 */
PlayerType PlayerForId(KODI::PLAYLIST::Id playerid);

/*! \brief The player a request addresses, by playerid or by playlistid.
 \param playerid the playerid given, TYPE_NONE when the request gave none
 \param playlistid the playlistid given, TYPE_NONE when the request gave none
 \param state the players running and the playlists in force
 \param player receives the player
 \return OK; InvalidParams for both or neither id; Unavailable for a player that is not running,
         or a playlist no running player is working through
 */
JSONRPC_STATUS ResolvePlayer(KODI::PLAYLIST::Id playerid,
                             KODI::PLAYLIST::Id playlistid,
                             const PlayerState& state,
                             PlayerType& player);

/*! \brief Fill the "player" member of a Player notification.
 \param player the member to fill
 \param type the player the notification is about
 \param playlist the playlist it is working through, TYPE_NONE for the player's own
 */
void DescribePlayer(CVariant& player, PlayerType type, KODI::PLAYLIST::Id playlist);

/*! \brief The playlist a player is working through.

 What GoTo, SetShuffle, SetRepeat and the playlistid and position properties ask about,
 and unrelated to the id the player is addressed by.

 \param player the player
 \param state the players running and the playlists in force
 \return the playlist
 */
KODI::PLAYLIST::Id PlaylistOf(PlayerType player, const PlayerState& state);

} // namespace JSONRPC

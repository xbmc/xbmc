/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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

 Shares a numeric range with the playlist ids but is unrelated to them.

 \param player the player being addressed
 \return the playerid
 */
KODI::PLAYLIST::Id PlayerIdOf(PlayerType player);

/*! \brief The player a playerid names, running or not.
 \param playerid the playerid as the client gave it
 \return the player, or None when the id names none
 */
PlayerType PlayerForId(KODI::PLAYLIST::Id playerid);

/*! \brief The player a playerid names, when that player is running.
 \param playerid the playerid as the client gave it
 \param state the players running and the playlists in force
 \return the player, or None when the id names none or its player is not running
 */
PlayerType RunningPlayerForId(KODI::PLAYLIST::Id playerid, const PlayerState& state);

/*! \brief Fill the "player" member of a Player notification with the player's own id.
 \param player the member to fill
 \param type the player the notification is about
 */
void DescribePlayer(CVariant& player, PlayerType type);

/*! \brief The playlist a player is working through.

 \param player the player
 \param state the players running and the playlists in force
 \return the playlist
 */
KODI::PLAYLIST::Id PlaylistOf(PlayerType player, const PlayerState& state);

} // namespace JSONRPC

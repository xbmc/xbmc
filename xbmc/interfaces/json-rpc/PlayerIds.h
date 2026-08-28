/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "playlists/PlayListTypes.h"

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
 \param player the player being addressed
 \param state the players running and the playlists in force
 \return the playerid
 */
KODI::PLAYLIST::Id PlayerIdOf(PlayerType player, const PlayerState& state);

/*! \brief The player a client's playerid names.
 \param playerid the playerid as the client gave it
 \param state the players running and the playlists in force
 \return the player, or None when the id names none
 */
PlayerType PlayerForId(KODI::PLAYLIST::Id playerid, const PlayerState& state);

} // namespace JSONRPC

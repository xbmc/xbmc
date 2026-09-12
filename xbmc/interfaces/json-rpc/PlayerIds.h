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
 \param activePlayers the players running, as PlayerType flags
 \return the player, or None when the id names none or its player is not running
 */
PlayerType RunningPlayerForId(KODI::PLAYLIST::Id playerid, int activePlayers);

/*! \brief Fill the "player" member of a Player notification with the player's own id.
 \param player the member to fill
 \param type the player the notification is about
 */
void DescribePlayer(CVariant& player, PlayerType type);

} // namespace JSONRPC

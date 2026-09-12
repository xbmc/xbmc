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

PlayerType RunningPlayerForId(PLAYLIST::Id playerid, int activePlayers)
{
  const PlayerType player = PlayerForId(playerid);
  return (activePlayers & player) != 0 ? player : None;
}

void DescribePlayer(CVariant& player, PlayerType type)
{
  player["playerid"] = static_cast<int>(PlayerIdOf(type));
}

} // namespace JSONRPC

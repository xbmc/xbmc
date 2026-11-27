/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/IAnnouncer.h"
#include "playlists/PlayListTypes.h"
#include "utils/Variant.h"

#include <optional>
#include <string>
#include <vector>

// Temporary: rebuilds the JSON-RPC v14 notifications from the events; removed with JSON-RPC v15.
namespace ANNOUNCEMENT::V14
{

//! \brief The ids JSON-RPC v14 gives its playlists, and the players playing them.
enum class PlaylistId
{
  TYPE_NONE = -1,
  TYPE_MUSIC = 0,
  TYPE_VIDEO = 1,
  TYPE_PICTURE = 2
};

//! \brief The playlist a v14 id names; none for the picture playlist or an unknown id.
std::optional<KODI::PLAYLIST::Type> PlayListFromId(PlaylistId id);

//! \brief The v14 id of a playlist; TYPE_NONE for none.
PlaylistId IdFromPlayList(std::optional<KODI::PLAYLIST::Type> type);

//! \brief An announcement as JSON-RPC v14 clients and Python add-ons are given it.
struct Notification
{
  AnnouncementFlag flag;
  std::string sender;
  std::string message;
  CVariant data;
};

/*!
 * \brief The notifications an announcement is given as; none for an event v14 does not know.
 * Python add-ons read the data as it is, so nothing in it may move, be renamed, be removed or
 * change type.
 */
std::vector<Notification> NotificationsOf(const Announcement& announcement);

} // namespace ANNOUNCEMENT::V14

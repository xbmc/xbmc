/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace PLAYLIST
{

using Id = int;

constexpr Id TYPE_NONE = -1; //! Playlist id of type none
constexpr Id TYPE_MUSIC = 0; //! Playlist id of type music
constexpr Id TYPE_VIDEO = 1; //! Playlist id of type video
constexpr Id TYPE_PICTURE = 2; //! Playlist id of type picture

/*!
 * \brief Manages playlist playing.
 */
enum class RepeatState
{
  NONE,
  ONE,
  ALL
};

} // namespace PLAYLIST

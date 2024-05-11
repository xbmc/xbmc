/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI::PLAYLIST
{

enum class Id : int
{
  TYPE_NONE = -1,
  TYPE_MUSIC = 0,
  TYPE_VIDEO = 1,
  TYPE_PICTURE = 2
};

/*!
 * \brief Manages playlist playing.
 */
enum class RepeatState
{
  NONE,
  ONE,
  ALL
};

} // namespace KODI::PLAYLIST

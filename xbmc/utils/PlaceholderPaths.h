/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/StringUtils.h"

#include <string_view>

//! \brief The paths of list entries that stand for an action rather than an item, such as creating
//! a playlist.
namespace KODI::PLACEHOLDER
{

//! The entry that adds a source
inline constexpr char ADD_SOURCE[] = "add";

inline constexpr char NEW_PLAYLIST[] = "newplaylist://";
inline constexpr char NEW_SMART_PLAYLIST[] = "newsmartplaylist://";
inline constexpr char NEW_TAG[] = "newtag://";
inline constexpr char MUSIC_SEARCH[] = "musicsearch://";

//! \brief Whether \p path is the entry that creates a playlist or a smart playlist.
inline bool IsNewPlaylist(std::string_view path)
{
  return StringUtils::StartsWithNoCase(path, NEW_PLAYLIST) ||
         StringUtils::StartsWithNoCase(path, NEW_SMART_PLAYLIST);
}

//! \brief Whether \p path is an entry that creates a playlist, a smart playlist or a tag.
inline bool IsNewItem(std::string_view path)
{
  return IsNewPlaylist(path) || StringUtils::StartsWithNoCase(path, NEW_TAG);
}

} // namespace KODI::PLACEHOLDER

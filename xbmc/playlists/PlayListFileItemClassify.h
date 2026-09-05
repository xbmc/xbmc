/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "playlists/PlayListTypes.h"

class CFileItem;

namespace KODI::PLAYLIST
{

//! \brief Check whether an item is a playlist.
bool IsPlayList(const CFileItem& item);

//! \brief Check whether an item is a smart playlist.
bool IsSmartPlayList(const CFileItem& item);

/*! \brief The playlist an item belongs to, decided from what the item is.

 PVR items answer from their tags, because their streams cannot: a radio channel or recording
 may carry a video stream for a logo or slideshow.

 \param item the item
 \return the playlist, or TYPE_NONE when the item does not say which
 */
Id PlaylistIdOf(const CFileItem& item);

} // namespace KODI::PLAYLIST

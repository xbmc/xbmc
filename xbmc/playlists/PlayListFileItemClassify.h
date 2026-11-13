/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "media/MediaStreams.h"
#include "playlists/PlayListTypes.h"

#include <optional>

class CFileItem;
class CFileItemList;

namespace KODI::PLAYLIST
{

//! \brief Check whether an item is a playlist.
bool IsPlayList(const CFileItem& item);

//! \brief Check whether an item is a smart playlist.
bool IsSmartPlayList(const CFileItem& item);

/*!
 * \brief The playlist for items nobody named one for: Video if any of them is video, else Audio if
 * any is audio, else Video.
 */
Type TypeFor(const CFileItemList& items);

/*!
 * \brief The playlist for the entries read from \p source: as for any items, except that when no
 * entry says what it is, \p source decides.
 */
Type TypeFor(const CFileItemList& entries, const CFileItem& source);

/*!
 * \brief The playlist for an item nobody named one for: Audio if it holds only audio, else Video.
 */
Type TypeFor(const CFileItem& item);

/*!
 * \brief What an item holds: a channel's is whether it is radio or TV, a picture holds video only,
 * a video or a game video and audio, and audio only audio; nothing when it is none of these.
 */
std::optional<MEDIA::Streams> StreamsOf(const CFileItem& item);

//! \brief Whether an item neither is nor contains a playlist entry: the parent item or an archive.
bool YieldsNoEntries(const CFileItem& item);

/*!
 * \brief Whether an item is read for the entries it holds: a smart playlist, or a playlist file
 * that is not a game's list of discs.
 */
bool HoldsEntries(const CFileItem& item);

/*!
 * \brief Whether an item can become a playlist entry: not a folder, an NFO, or anything that
 * yields no entries.
 */
bool CanBeEntry(const CFileItem& item);

} // namespace KODI::PLAYLIST

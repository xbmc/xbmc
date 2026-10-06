/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

//! \brief The musicdb:// paths of the music library's nodes and filtered album views.
namespace KODI::MUSIC::DB_PATH
{

inline constexpr char ROOT[] = "musicdb://";

inline constexpr char GENRES[] = "musicdb://genres/";
inline constexpr char ARTISTS[] = "musicdb://artists/";
inline constexpr char ALBUMS[] = "musicdb://albums/";
inline constexpr char BOX_SETS[] = "musicdb://albums/?boxset=true";
inline constexpr char SINGLES[] = "musicdb://singles/";
inline constexpr char SONGS[] = "musicdb://songs/";
inline constexpr char YEARS[] = "musicdb://years/";
inline constexpr char ORIGINAL_YEARS[] = "musicdb://originalyears/";
inline constexpr char TOP100[] = "musicdb://top100/";
inline constexpr char TOP100_ALBUMS[] = "musicdb://top100/albums/";
inline constexpr char TOP100_SONGS[] = "musicdb://top100/songs/";
inline constexpr char RECENTLY_ADDED_ALBUMS[] = "musicdb://recentlyaddedalbums/";
inline constexpr char RECENTLY_PLAYED_ALBUMS[] = "musicdb://recentlyplayedalbums/";
inline constexpr char COMPILATIONS[] = "musicdb://albums/?compilation=true";
inline constexpr char ROLES[] = "musicdb://roles/";
inline constexpr char SOURCES[] = "musicdb://sources/";
inline constexpr char DISCS[] = "musicdb://discs/";

} // namespace KODI::MUSIC::DB_PATH

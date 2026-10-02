/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

//! \brief The library:// paths of the library nodes, as XFILE::CLibraryDirectory resolves them.
namespace KODI::LIBRARY
{

inline constexpr char ROOT[] = "library://";

inline constexpr char VIDEO[] = "library://video/";
inline constexpr char VIDEO_FLAT[] = "library://video_flat/";
inline constexpr char VIDEO_FILES[] = "library://video/files.xml/";
inline constexpr char MOVIE_TITLES[] = "library://video/movies/titles.xml/";
inline constexpr char TVSHOW_TITLES[] = "library://video/tvshows/titles.xml/";
inline constexpr char MUSICVIDEOS[] = "library://video/musicvideos/";
inline constexpr char MUSICVIDEO_TITLES[] = "library://video/musicvideos/titles.xml/";

inline constexpr char MUSIC[] = "library://music/";
inline constexpr char MUSIC_FILES[] = "library://music/files.xml/";
inline constexpr char MUSIC_PLAYLISTS[] = "library://music/playlists.xml/";

} // namespace KODI::LIBRARY

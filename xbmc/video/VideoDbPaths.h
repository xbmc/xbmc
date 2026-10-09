/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

//! \brief The videodb:// paths of the video library's nodes, as XFILE::CVideoDatabaseDirectory
//! parses them.
namespace KODI::VIDEO::DB_PATH
{

inline constexpr char ROOT[] = "videodb://";

inline constexpr char MOVIES[] = "videodb://movies/";
inline constexpr char MOVIE_GENRES[] = "videodb://movies/genres/";
inline constexpr char MOVIE_TITLES[] = "videodb://movies/titles/";
inline constexpr char MOVIE_YEARS[] = "videodb://movies/years/";
inline constexpr char MOVIE_ACTORS[] = "videodb://movies/actors/";
inline constexpr char MOVIE_DIRECTORS[] = "videodb://movies/directors/";
inline constexpr char MOVIE_STUDIOS[] = "videodb://movies/studios/";
inline constexpr char MOVIE_SETS[] = "videodb://movies/sets/";
inline constexpr char MOVIE_COUNTRIES[] = "videodb://movies/countries/";
inline constexpr char MOVIE_TAGS[] = "videodb://movies/tags/";

inline constexpr char TVSHOWS[] = "videodb://tvshows/";
inline constexpr char TVSHOW_GENRES[] = "videodb://tvshows/genres/";
inline constexpr char TVSHOW_TITLES[] = "videodb://tvshows/titles/";
inline constexpr char TVSHOW_YEARS[] = "videodb://tvshows/years/";
inline constexpr char TVSHOW_ACTORS[] = "videodb://tvshows/actors/";
inline constexpr char TVSHOW_STUDIOS[] = "videodb://tvshows/studios/";
inline constexpr char TVSHOW_TAGS[] = "videodb://tvshows/tags/";

inline constexpr char MUSICVIDEOS[] = "videodb://musicvideos/";
inline constexpr char MUSICVIDEO_GENRES[] = "videodb://musicvideos/genres/";
inline constexpr char MUSICVIDEO_TITLES[] = "videodb://musicvideos/titles/";
inline constexpr char MUSICVIDEO_YEARS[] = "videodb://musicvideos/years/";
inline constexpr char MUSICVIDEO_ARTISTS[] = "videodb://musicvideos/artists/";
inline constexpr char MUSICVIDEO_ALBUMS[] = "videodb://musicvideos/albums/";
inline constexpr char MUSICVIDEO_DIRECTORS[] = "videodb://musicvideos/directors/";
inline constexpr char MUSICVIDEO_STUDIOS[] = "videodb://musicvideos/studios/";
inline constexpr char MUSICVIDEO_TAGS[] = "videodb://musicvideos/tags/";

inline constexpr char RECENTLY_ADDED_MOVIES[] = "videodb://recentlyaddedmovies/";
inline constexpr char RECENTLY_ADDED_EPISODES[] = "videodb://recentlyaddedepisodes/";
inline constexpr char RECENTLY_ADDED_MUSICVIDEOS[] = "videodb://recentlyaddedmusicvideos/";
inline constexpr char INPROGRESS_TVSHOWS[] = "videodb://inprogresstvshows/";

} // namespace KODI::VIDEO::DB_PATH

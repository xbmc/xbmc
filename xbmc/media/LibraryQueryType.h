/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "media/MediaSection.h"
#include "media/MediaType.h"
#include "playlists/PlayListTypes.h"

#include <array>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

//! \brief What a library query, the model behind a smart playlist and a library filter, lists.
namespace KODI::MEDIA::LIBRARY_QUERY
{

enum class TYPE
{
  SONGS,
  ALBUMS,
  ARTISTS,
  MOVIES,
  TV_SHOWS,
  EPISODES,
  MUSIC_VIDEOS,
  //! Songs and music videos
  MIXED,
};

inline constexpr std::array<TYPE, 8> TYPES{
    TYPE::SONGS,    TYPE::ALBUMS,   TYPE::ARTISTS,      TYPE::MOVIES,
    TYPE::TV_SHOWS, TYPE::EPISODES, TYPE::MUSIC_VIDEOS, TYPE::MIXED,
};

/*!
 * \brief The name a type has in .xsp files and JSON filters, which is also the content name of a
 * list of its items: "songs", "albums", "artists", "movies", "tvshows", "episodes", "musicvideos"
 * or "mixed".
 */
std::string_view NameOf(TYPE type);

//! \brief The type \p name names exactly. Nullopt for any other text, the empty one included.
std::optional<TYPE> TypeFromName(std::string_view name);

//! \brief The id of the string that labels the type.
int LabelOf(TYPE type);

//! \brief A database a type lists from, and the kind of item it lists from it.
struct Source
{
  MediaSection section;
  MEDIA::TYPE itemType;

  bool operator==(const Source&) const = default;
};

//! \brief Where a type lists from. One source, or for MIXED the songs and then the music videos.
std::span<const Source> SourcesOf(TYPE type);

//! \brief Whether any of a type's items come from this section's database.
bool ListsFrom(TYPE type, MediaSection section);

//! \brief The kind of item a type lists from this section's database. Nullopt if it lists none.
std::optional<MEDIA::TYPE> ItemTypeIn(TYPE type, MediaSection section);

//! \brief The type that lists these items alone. Nullopt for an item type no query lists.
std::optional<TYPE> TypeOf(MEDIA::TYPE itemType);

//! \brief The folder of the playlists path a type's playlists are saved in.
std::string_view SaveFolderOf(TYPE type);

/*!
 * \brief Whether a playlist of \p type may take the rules of one of \p referenced: a playlist of
 * its own type, and mixed with songs or music videos either way round.
 */
bool CanReference(TYPE type, TYPE referenced);

/*!
 * \brief The folders of the playlists path that hold the playlists a playlist of \p type may
 * take the rules of: the save folder of every type it can reference.
 */
std::vector<std::string_view> ReferenceFoldersOf(TYPE type);

//! \brief The playlist a type's items play on: Audio for songs, albums and artists, else Video.
PLAYLIST::Type PlayListOf(TYPE type);

//! \brief Whether party mode can feed from a type: songs, music videos and mixed.
bool CanFeedPartyMode(TYPE type);

/*!
 * \brief What the values of a field are picked from when they are browsed. Each library lists what
 * it has of a kind beside its database.
 */
enum class BROWSE
{
  NOTHING,
  GENRES,
  SOURCES,
  ROLES,
  COUNTRIES,
  ARTISTS,
  ALBUM_ARTISTS,
  ALBUMS,
  ALBUM_TYPES,
  MUSIC_LABELS,
  ACTORS,
  YEARS,
  ORIGINAL_YEARS,
  DIRECTORS,
  STUDIOS,
  WRITERS,
  TVSHOWS,
  //! The titles of the items themselves
  TITLES,
  SETS,
  TAGS,
  //! Other smart playlists
  PLAYLISTS,
  //! The folders of the sources
  PATHS,
};

//! \brief What a query lists in place of the items it matches: their genres, their years and so on.
enum class GROUP
{
  //! No group is named: the items are listed, and the library groups them as it does any listing
  DEFAULT,
  //! The items are listed and nothing groups them
  NONE,
  SET,
  GENRE,
  YEAR,
  //! The artists of music and music videos, the actors of movies and TV shows
  PERFORMER,
  DIRECTOR,
  WRITER,
  STUDIO,
  COUNTRY,
  ALBUM,
  TAG,
  ORIGINAL_YEAR,
  VIDEO_VERSION,
};

/*!
 * \brief The group \p name names in an .xsp file, JSON or the group.by property, in any case:
 * "", "none", "sets", "genres", "years", "actors" or "artists" (both the performers), "directors",
 * "writers", "studios", "countries", "albums", "tags", "originalyears" or "videoversions". Nullopt
 * for any other text.
 */
std::optional<GROUP> GroupFromName(std::string_view name);

/*!
 * \brief The name a group of a query of \p type is written as, which is also the content name of
 * the list of its groups. The performers are "actors" for movies, TV shows and episodes and
 * "artists" for everything else.
 */
std::string_view NameOf(GROUP group, TYPE type);

//! \brief The id of the string that labels a group of a query of \p type.
int LabelOf(GROUP group, TYPE type);

//! \brief Whether a query grouped this way can list the items no group holds beside the groups.
bool CanMix(GROUP group);

} // namespace KODI::MEDIA::LIBRARY_QUERY

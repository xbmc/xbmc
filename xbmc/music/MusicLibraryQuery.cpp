/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicLibraryQuery.h"

#include "FileItemList.h"
#include "media/LibraryQueryRule.h"
#include "media/MediaType.h"
#include "music/MusicDatabase.h"
#include "music/MusicDbPaths.h"
#include "utils/ContentNames.h"
#include "utils/DatabaseUtils.h"
#include "utils/SortUtils.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <string_view>

using enum CDatabaseQueryRule::SearchOperator;

namespace KODI::MUSIC::LIBRARY_QUERY
{

namespace
{
std::string Column(Field field, MEDIA::TYPE type)
{
  return DatabaseUtils::GetField(field, type, DatabaseQueryPart::WHERE);
}

std::string YearClause(const MEDIA::CLibraryQueryRule& rule,
                       const std::string& field,
                       const std::string& param,
                       const std::string& parameter)
{
  std::string query;
  if (rule.m_operator == OPERATOR_EQUALS && param == "0")
    query = "(TRIM(" + field + ") = '' OR " + field + " IS NULL)";
  else if (rule.m_operator == OPERATOR_DOES_NOT_EQUAL && param == "0")
    query = "(TRIM(" + field + ") <> '' AND " + field + " IS NOT NULL)";
  else
  { // Get year from ISO8601 date string, cast as INTEGER
    query = "CAST(" + field + " as INTEGER)" + parameter;
    if (rule.m_operator == OPERATOR_LESS_THAN)
      query = "(TRIM(" + field + ") = '' OR " + field + " IS NULL OR " + query + ")";
  }
  return query;
}
} // namespace

std::string RuleClause(const MEDIA::CLibraryQueryRule& rule,
                       MEDIA::TYPE type,
                       const std::string& negate,
                       const std::string& param,
                       const std::string& parameter,
                       bool useOriginalDate)
{
  const Field field{rule.m_field};
  const Field yearField{useOriginalDate ? Field::ORIG_YEAR : field};
  std::string query;
  if (type == MEDIA::TYPE::SONG)
  {
    if (field == Field::GENRE)
      query = negate + " EXISTS (SELECT 1 FROM song_genre, genre WHERE song_genre.idSong = " +
              Column(Field::ID, type) +
              " AND song_genre.idGenre = genre.idGenre AND genre.strGenre" + parameter + ")";
    else if (field == Field::ARTIST)
      query = negate + " EXISTS (SELECT 1 FROM song_artist, artist WHERE song_artist.idSong = " +
              Column(Field::ID, type) +
              " AND song_artist.idArtist = artist.idArtist AND artist.strArtist" + parameter + ")";
    else if (field == Field::ALBUM_ARTIST)
      query = negate +
              " EXISTS (SELECT 1 FROM album_artist, artist WHERE album_artist.idAlbum = "
              "songview.idAlbum AND album_artist.idArtist = artist.idArtist AND artist.strArtist" +
              parameter + ")";
    else if (field == Field::LAST_PLAYED)
      query = MEDIA::CLibraryQueryRule::FormatNullableDate(Column(field, type), rule.m_operator,
                                                           parameter);
    else if (field == Field::SOURCE)
      query = negate +
              " EXISTS (SELECT 1 FROM album_source, source WHERE album_source.idAlbum = "
              "songview.idAlbum AND album_source.idSource = source.idSource AND source.strName" +
              parameter + ")";
    else if (field == Field::YEAR || field == Field::ORIG_YEAR)
    {
      query = YearClause(rule, Column(yearField, type), param, parameter);
    }
  }
  else if (type == MEDIA::TYPE::ALBUM)
  {
    if (field == Field::GENRE)
      query = negate + " EXISTS (SELECT 1 FROM song, song_genre, genre WHERE song.idAlbum = " +
              Column(Field::ID, type) +
              " AND song.idSong = song_genre.idSong AND song_genre.idGenre = genre.idGenre AND "
              "genre.strGenre" +
              parameter + ")";
    else if (field == Field::ARTIST)
      query = negate + " EXISTS (SELECT 1 FROM song, song_artist, artist WHERE song.idAlbum = " +
              Column(Field::ID, type) +
              " AND song.idSong = song_artist.idSong AND song_artist.idArtist = artist.idArtist "
              "AND artist.strArtist" +
              parameter + ")";
    else if (field == Field::ALBUM_ARTIST)
      query = negate + " EXISTS (SELECT 1 FROM album_artist, artist WHERE album_artist.idAlbum = " +
              Column(Field::ID, type) +
              " AND album_artist.idArtist = artist.idArtist AND artist.strArtist" + parameter + ")";
    else if (field == Field::PATH)
      query = negate +
              " EXISTS (SELECT 1 FROM song JOIN path on song.idpath = path.idpath WHERE "
              "song.idAlbum = " +
              Column(Field::ID, type) + " AND path.strPath" + parameter + ")";
    else if (field == Field::LAST_PLAYED)
      query = MEDIA::CLibraryQueryRule::FormatNullableDate(Column(field, type), rule.m_operator,
                                                           parameter);
    else if (field == Field::SOURCE)
      query = negate + " EXISTS (SELECT 1 FROM album_source, source WHERE album_source.idAlbum = " +
              Column(Field::ID, type) +
              " AND album_source.idSource = source.idSource AND source.strName" + parameter + ")";
    else if (field == Field::DISC_TITLE)
      query = negate +
              " EXISTS (SELECT 1 FROM song WHERE song.idAlbum = " + Column(Field::ID, type) +
              " AND song.strDiscSubtitle" + parameter + ")";
    else if (field == Field::YEAR || field == Field::ORIG_YEAR)
    {
      query = YearClause(rule, Column(yearField, type), param, parameter);
    }
  }
  else if (type == MEDIA::TYPE::ARTIST)
  {
    if (field == Field::GENRE)
    {
      query = negate +
              " (EXISTS (SELECT DISTINCT song_artist.idArtist FROM song_artist, song_genre, genre "
              "WHERE song_artist.idArtist = " +
              Column(Field::ID, type) +
              " AND song_artist.idSong = song_genre.idSong AND song_genre.idGenre = genre.idGenre "
              "AND genre.strGenre" +
              parameter + ")";
      query += " OR ";
      query += "EXISTS (SELECT DISTINCT album_artist.idArtist FROM album_artist, song, song_genre, "
               "genre WHERE album_artist.idArtist = " +
               Column(Field::ID, type) +
               " AND song.idAlbum = album_artist.idAlbum AND song.idSong = song_genre.idSong AND "
               "song_genre.idGenre = genre.idGenre AND genre.strGenre" +
               parameter + "))";
    }
    else if (field == Field::ROLE)
    {
      query = negate +
              " (EXISTS (SELECT DISTINCT song_artist.idArtist FROM song_artist, role WHERE "
              "song_artist.idArtist = " +
              Column(Field::ID, type) + " AND song_artist.idRole = role.idRole AND role.strRole" +
              parameter + "))";
    }
    else if (field == Field::PATH)
    {
      query = negate +
              " (EXISTS (SELECT DISTINCT song_artist.idArtist FROM song_artist JOIN song ON "
              "song.idSong = song_artist.idSong JOIN path ON song.idpath = path.idpath ";
      query += "WHERE song_artist.idArtist = " + Column(Field::ID, type) + " AND path.strPath" +
               parameter + "))";
    }
    else if (field == Field::SOURCE)
    {
      query = negate +
              " (EXISTS(SELECT 1 FROM song_artist, song, album_source, source WHERE "
              "song_artist.idArtist = " +
              Column(Field::ID, type) +
              " AND song.idSong = song_artist.idSong AND song_artist.idRole = 1 AND "
              "album_source.idAlbum = song.idAlbum AND album_source.idSource = source.idSource AND "
              "source.strName" +
              parameter + ")";
      query += " OR ";
      query += " EXISTS (SELECT 1 FROM album_artist, album_source, source WHERE "
               "album_artist.idArtist = " +
               Column(Field::ID, type) +
               " AND album_source.idAlbum = album_artist.idAlbum AND album_source.idSource = "
               "source.idSource AND source.strName" +
               parameter + "))";
    }
  }

  if (field == Field::USER_RATING)
  {
    query = MEDIA::CLibraryQueryRule::FormatNullableNumber(Column(field, type), rule.m_operator,
                                                           param, parameter);
  }
  return query;
}

std::string BooleanClause(const MEDIA::CLibraryQueryRule& rule,
                          MEDIA::TYPE type,
                          const std::string& negate)
{
  const Field field{rule.m_field};
  if (type == MEDIA::TYPE::ALBUM)
  {
    if (field == Field::COMPILATION)
      return negate + Column(field, type);
    if (field == Field::IS_BOXSET)
      return negate + "albumview.bBoxedSet = 1";
  }
  return "";
}

namespace
{
using MEDIA::LIBRARY_QUERY::GROUP;

struct ItemListing
{
  MEDIA::TYPE itemType;
  std::string_view path;
};

constexpr std::array<ItemListing, 3> ITEM_LISTINGS{{
    {MEDIA::TYPE::SONG, DB_PATH::SONGS},
    {MEDIA::TYPE::ALBUM, DB_PATH::ALBUMS},
    {MEDIA::TYPE::ARTIST, DB_PATH::ARTISTS},
}};

struct GroupListing
{
  GROUP group;
  std::string_view path;
  std::string_view content;
};

// Both kinds of year are listed by the years node.
constexpr std::array<GroupListing, 5> GROUP_LISTINGS{{
    {GROUP::GENRE, DB_PATH::GENRES, MEDIA::CONTENT::GENRES},
    {GROUP::YEAR, DB_PATH::YEARS, MEDIA::CONTENT::YEARS},
    {GROUP::ORIGINAL_YEAR, DB_PATH::ORIGINAL_YEARS, MEDIA::CONTENT::YEARS},
    {GROUP::PERFORMER, DB_PATH::ARTISTS, MEDIA::CONTENT::ARTISTS},
    {GROUP::ALBUM, DB_PATH::ALBUMS, MEDIA::CONTENT::ALBUMS},
}};
} // namespace

std::optional<std::string_view> ListingPath(MEDIA::TYPE itemType, MEDIA::LIBRARY_QUERY::GROUP group)
{
  if (group == GROUP::DEFAULT || group == GROUP::NONE)
  {
    const auto it = std::ranges::find(ITEM_LISTINGS, itemType, &ItemListing::itemType);
    if (it == ITEM_LISTINGS.end())
    {
      return std::nullopt;
    }
    return it->path;
  }

  const auto it = std::ranges::find(GROUP_LISTINGS, group, &GroupListing::group);
  if (it == GROUP_LISTINGS.end() ||
      std::ranges::find(ITEM_LISTINGS, itemType, &ItemListing::itemType) == ITEM_LISTINGS.end())
  {
    return std::nullopt;
  }
  return it->path;
}

std::string_view GroupContent(MEDIA::LIBRARY_QUERY::GROUP group)
{
  const auto it = std::ranges::find(GROUP_LISTINGS, group, &GroupListing::group);
  return it == GROUP_LISTINGS.end() ? std::string_view{} : it->content;
}

bool BrowseValues(CMusicDatabase& db,
                  MEDIA::LIBRARY_QUERY::BROWSE browse,
                  MEDIA::TYPE itemType,
                  const std::string& baseDir,
                  const CDatabase::Filter& filter,
                  bool countOnly,
                  CFileItemList& items)
{
  using enum MEDIA::LIBRARY_QUERY::BROWSE;
  const auto dir = [&baseDir](const char* whole)
  { return baseDir.empty() ? std::string{whole} : baseDir; };

  switch (browse)
  {
    case GENRES:
      return db.GetGenresNav(dir(DB_PATH::GENRES), items, filter, countOnly);
    case SOURCES:
      return db.GetSourcesNav(dir(DB_PATH::SOURCES), items, filter, countOnly);
    case ROLES:
      return db.GetRolesNav(dir(DB_PATH::SONGS), items, filter);
    case ARTISTS:
    case ALBUM_ARTISTS:
      return db.GetArtistsNav(dir(DB_PATH::ARTISTS), items, SortDescription(),
                              browse == ALBUM_ARTISTS, -1, -1, -1, filter, countOnly);
    case ALBUMS:
      return db.GetAlbumsNav(dir(DB_PATH::ALBUMS), items, SortDescription(), -1, -1, filter,
                             countOnly);
    case ALBUM_TYPES:
      return db.GetAlbumTypesNav(dir(DB_PATH::ALBUMS), items, filter, countOnly);
    case MUSIC_LABELS:
      return db.GetMusicLabelsNav(dir(DB_PATH::ALBUMS), items, filter, countOnly);
    case YEARS:
      return db.GetYearsNav(dir(DB_PATH::YEARS), items, filter);
    case ORIGINAL_YEARS:
      return db.GetYearsNav(dir(DB_PATH::ORIGINAL_YEARS), items, filter);
    case TITLES:
      return itemType == MEDIA::TYPE::SONG &&
             db.GetSongsNav(dir(DB_PATH::SONGS), items, SortDescription(), -1, -1, -1);
    default:
      return false;
  }
}

std::optional<MEDIA::LIBRARY_QUERY::RangeColumn> RangeColumnOf(Field field, MEDIA::TYPE itemType)
{
  using MEDIA::LIBRARY_QUERY::RangeColumn;
  if (field == Field::YEAR && (itemType == MEDIA::TYPE::ALBUM || itemType == MEDIA::TYPE::SONG))
  {
    return RangeColumn{itemType == MEDIA::TYPE::ALBUM ? "albumview" : "songview",
                       DatabaseUtils::GetField(Field::YEAR, itemType, DatabaseQueryPart::SELECT),
                       DatabaseUtils::GetField(Field::YEAR, itemType, DatabaseQueryPart::WHERE) +
                           " > 0"};
  }
  if (itemType == MEDIA::TYPE::SONG && field == Field::TIME)
  {
    return RangeColumn{"songview", "iDuration", ""};
  }
  if (itemType == MEDIA::TYPE::SONG && field == Field::PLAYCOUNT)
  {
    return RangeColumn{"songview", "iTimesPlayed", ""};
  }
  return std::nullopt;
}

bool ItemsById(CMusicDatabase& db,
               MEDIA::TYPE itemType,
               const std::string& ids,
               CFileItemList& items)
{
  if (itemType != MEDIA::TYPE::SONG)
  {
    return false;
  }
  CFileItemList songs;
  db.GetSongsFullByWhere(MUSIC::DB_PATH::SONGS, songs, SortDescription{},
                         CDatabase::Filter("songview.idSong IN (" + ids + ")"), true);
  for (const auto& item : songs)
  {
    db.SetPropertiesForFileItem(*item);
  }
  items.Append(songs);
  return true;
}

} // namespace KODI::MUSIC::LIBRARY_QUERY

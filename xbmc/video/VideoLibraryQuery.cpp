/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoLibraryQuery.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "dbwrappers/Database.h"
#include "media/LibraryQueryRule.h"
#include "media/MediaType.h"
#include "utils/ContentNames.h"
#include "utils/DatabaseUtils.h"
#include "utils/LabelFormatter.h"
#include "utils/StreamDetails.h"
#include "utils/StringUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoDbPaths.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

using enum CDatabaseQueryRule::SearchOperator;

namespace KODI::VIDEO::LIBRARY_QUERY
{

namespace
{
std::string Column(Field field, MEDIA::TYPE type)
{
  return DatabaseUtils::GetField(field, type, DatabaseQueryPart::WHERE);
}

std::string StreamDetailsExists(const std::string& negate,
                                const std::string& table,
                                const std::string& condition)
{
  return negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table +
         ".idFile AND " + condition + ")";
}

std::string VideoResolutionClause(const MEDIA::CLibraryQueryRule& rule,
                                  const std::string& parameter)
{
  std::string retVal(" IN (SELECT DISTINCT idFile FROM streamdetails WHERE iVideoWidth ");
  int iRes = static_cast<int>(std::strtol(parameter.c_str(), nullptr, 10));

  int min, max;
  if (iRes >= 2160)
  {
    min = 1921;
    max = INT_MAX;
  }
  else if (iRes >= 1080)
  {
    min = 1281;
    max = 1920;
  }
  else if (iRes >= 720)
  {
    min = 961;
    max = 1280;
  }
  else if (iRes >= 540)
  {
    min = 721;
    max = 960;
  }
  else
  {
    min = 0;
    max = 720;
  }

  switch (rule.m_operator)
  {
    case OPERATOR_EQUALS:
      retVal += StringUtils::Format(">= {} AND iVideoWidth <= {}", min, max);
      break;
    case OPERATOR_DOES_NOT_EQUAL:
      retVal += StringUtils::Format("< {} OR iVideoWidth > {}", min, max);
      break;
    case OPERATOR_LESS_THAN:
      retVal += StringUtils::Format("< {}", min);
      break;
    case OPERATOR_GREATER_THAN:
      retVal += StringUtils::Format("> {}", max);
      break;
    default:
      break;
  }

  retVal += ")";
  return retVal;
}

std::string LinkClause(const char* field,
                       const char* table,
                       MEDIA::TYPE mediaType,
                       const std::string& mediaField,
                       const std::string& parameter)
{
  // NOTE: no need for a PrepareSQL here, as the parameter has already been formatted
  return StringUtils::Format(
      " EXISTS (SELECT 1 FROM {}_link"
      "         JOIN {} ON {}.{}_id={}_link.{}_id"
      "         WHERE {}_link.media_id={} AND {}.name {} AND {}_link.media_type = '{}')",
      field, table, table, table, field, table, field, mediaField, table, parameter, field,
      MEDIA::NameOf(mediaType));
}
} // namespace

std::string RuleClause(const MEDIA::CLibraryQueryRule& rule,
                       MEDIA::TYPE type,
                       const std::string& negate,
                       const std::string& param,
                       const std::string& parameter,
                       const CDatabase& db)
{
  const Field field{rule.m_field};
  std::string query;
  std::string table;
  if (type == MEDIA::TYPE::MOVIE)
  {
    table = "movie_view";

    if (field == Field::GENRE)
      query = negate +
              LinkClause("genre", "genre", MEDIA::TYPE::MOVIE, Column(Field::ID, type), parameter);
    else if (field == Field::DIRECTOR)
      query = negate + LinkClause("director", "actor", MEDIA::TYPE::MOVIE, Column(Field::ID, type),
                                  parameter);
    else if (field == Field::ACTOR)
      query = negate +
              LinkClause("actor", "actor", MEDIA::TYPE::MOVIE, Column(Field::ID, type), parameter);
    else if (field == Field::WRITER)
      query = negate +
              LinkClause("writer", "actor", MEDIA::TYPE::MOVIE, Column(Field::ID, type), parameter);
    else if (field == Field::STUDIO)
      query = negate + LinkClause("studio", "studio", MEDIA::TYPE::MOVIE, Column(Field::ID, type),
                                  parameter);
    else if (field == Field::COUNTRY)
      query = negate + LinkClause("country", "country", MEDIA::TYPE::MOVIE, Column(Field::ID, type),
                                  parameter);
    else if (field == Field::LAST_PLAYED || field == Field::DATE_ADDED)
      query = MEDIA::CLibraryQueryRule::FormatNullableDate(Column(field, type), rule.m_operator,
                                                           parameter);
    else if (field == Field::TAG)
      query =
          negate + LinkClause("tag", "tag", MEDIA::TYPE::MOVIE, Column(Field::ID, type), parameter);
  }
  else if (type == MEDIA::TYPE::MUSIC_VIDEO)
  {
    table = "musicvideo_view";

    if (field == Field::GENRE)
      query = negate + LinkClause("genre", "genre", MEDIA::TYPE::MUSIC_VIDEO,
                                  Column(Field::ID, type), parameter);
    else if (field == Field::ARTIST || field == Field::ALBUM_ARTIST)
      query = negate + LinkClause("actor", "actor", MEDIA::TYPE::MUSIC_VIDEO,
                                  Column(Field::ID, type), parameter);
    else if (field == Field::STUDIO)
      query = negate + LinkClause("studio", "studio", MEDIA::TYPE::MUSIC_VIDEO,
                                  Column(Field::ID, type), parameter);
    else if (field == Field::DIRECTOR)
      query = negate + LinkClause("director", "actor", MEDIA::TYPE::MUSIC_VIDEO,
                                  Column(Field::ID, type), parameter);
    else if (field == Field::LAST_PLAYED || field == Field::DATE_ADDED)
      query = MEDIA::CLibraryQueryRule::FormatNullableDate(Column(field, type), rule.m_operator,
                                                           parameter);
    else if (field == Field::TAG)
      query = negate + LinkClause("tag", "tag", MEDIA::TYPE::MUSIC_VIDEO, Column(Field::ID, type),
                                  parameter);
  }
  else if (type == MEDIA::TYPE::TV_SHOW)
  {
    table = "tvshow_view";

    if (field == Field::GENRE)
      query = negate + LinkClause("genre", "genre", MEDIA::TYPE::TV_SHOW, Column(Field::ID, type),
                                  parameter);
    else if (field == Field::DIRECTOR)
      query = negate + LinkClause("director", "actor", MEDIA::TYPE::TV_SHOW,
                                  Column(Field::ID, type), parameter);
    else if (field == Field::ACTOR)
      query = negate + LinkClause("actor", "actor", MEDIA::TYPE::TV_SHOW, Column(Field::ID, type),
                                  parameter);
    else if (field == Field::STUDIO)
      query = negate + LinkClause("studio", "studio", MEDIA::TYPE::TV_SHOW, Column(Field::ID, type),
                                  parameter);
    else if (field == Field::MPAA)
      query = negate + " (" + Column(field, type) + parameter + ")";
    else if (field == Field::LAST_PLAYED || field == Field::DATE_ADDED)
      query = MEDIA::CLibraryQueryRule::FormatNullableDate(Column(field, type), rule.m_operator,
                                                           parameter);
    else if (field == Field::PLAYCOUNT)
      query = "CASE WHEN COALESCE(" + Column(Field::NUMBER_OF_EPISODES, type) + " - " +
              Column(Field::NUMBER_OF_WATCHED_EPISODES, type) + ", 0) > 0 THEN 0 ELSE 1 END " +
              parameter;
    else if (field == Field::TAG)
      query = negate +
              LinkClause("tag", "tag", MEDIA::TYPE::TV_SHOW, Column(Field::ID, type), parameter);
  }
  else if (type == MEDIA::TYPE::EPISODE)
  {
    table = "episode_view";

    if (field == Field::GENRE)
      query = negate + LinkClause("genre", "genre", MEDIA::TYPE::TV_SHOW,
                                  (table + ".idShow").c_str(), parameter);
    else if (field == Field::TAG)
      query = negate + LinkClause("tag", "tag", MEDIA::TYPE::TV_SHOW, (table + ".idShow").c_str(),
                                  parameter);
    else if (field == Field::DIRECTOR)
      query = negate + LinkClause("director", "actor", MEDIA::TYPE::EPISODE,
                                  Column(Field::ID, type), parameter);
    else if (field == Field::ACTOR)
      query = negate + LinkClause("actor", "actor", MEDIA::TYPE::EPISODE, Column(Field::ID, type),
                                  parameter);
    else if (field == Field::WRITER)
      query = negate + LinkClause("writer", "actor", MEDIA::TYPE::EPISODE, Column(Field::ID, type),
                                  parameter);
    else if (field == Field::LAST_PLAYED || field == Field::DATE_ADDED)
      query = MEDIA::CLibraryQueryRule::FormatNullableDate(Column(field, type), rule.m_operator,
                                                           parameter);
    else if (field == Field::STUDIO)
      query = negate + LinkClause("studio", "studio", MEDIA::TYPE::TV_SHOW,
                                  (table + ".idShow").c_str(), parameter);
    else if (field == Field::MPAA)
      query = negate + " (" + Column(field, type) + parameter + ")";
  }
  if (field == Field::VIDEO_RESOLUTION)
  {
    query = table + ".idFile" + negate + VideoResolutionClause(rule, param);
  }
  else if (field == Field::AUDIO_CHANNELS)
  {
    query = StreamDetailsExists(negate, table, "iAudioChannels " + parameter);
  }
  else if (field == Field::VIDEO_CODEC)
  {
    query = StreamDetailsExists(negate, table, "strVideoCodec " + parameter);
  }
  else if (field == Field::AUDIO_CODEC)
  {
    query = StreamDetailsExists(negate, table, "strAudioCodec " + parameter);
  }
  else if (field == Field::AUDIO_LANGUAGE)
  {
    query = StreamDetailsExists(negate, table, "strAudioLanguage " + parameter);
  }
  else if (field == Field::SUBTITLE_LANGUAGE)
  {
    query = StreamDetailsExists(negate, table, "strSubtitleLanguage " + parameter);
  }
  else if (field == Field::VIDEO_ASPECT_RATIO)
  {
    query = StreamDetailsExists(negate, table, "fVideoAspect " + parameter);
  }
  else if (field == Field::AUDIO_COUNT)
  {
    query = db.PrepareSQL(
        StreamDetailsExists(negate, table,
                            "streamdetails.iStreamtype = %i GROUP BY streamdetails.idFile HAVING "
                            "COUNT(streamdetails.iStreamType) " +
                                parameter),
        CStreamDetail::AUDIO);
  }
  else if (field == Field::SUBTITLE_COUNT)
  {
    query = db.PrepareSQL(
        StreamDetailsExists(negate, table,
                            "streamdetails.iStreamType = %i GROUP BY streamdetails.idFile HAVING "
                            "COUNT(streamdetails.iStreamType) " +
                                parameter),
        CStreamDetail::SUBTITLE);
  }
  else if (field == Field::HDR_TYPE)
  {
    query = StreamDetailsExists(negate, table, "strHdrType " + parameter);
  }
  else if (field == Field::HDR_DETAIL)
  {
    query = StreamDetailsExists(negate, table, "strHdrDetail " + parameter);
  }

  if ((field == Field::PLAYCOUNT && type != MEDIA::TYPE::TV_SHOW) || field == Field::USER_RATING)
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
  if (type == MEDIA::TYPE::MOVIE)
  {
    if (field == Field::IN_PROGRESS)
      return "movie_view.idFile " + negate +
             " IN (SELECT DISTINCT idFile FROM bookmark WHERE type = 1)";
    else if (field == Field::TRAILER)
      return negate + Column(field, type) + "!= ''";
    else if (field == Field::HAS_VIDEO_VERSIONS || field == Field::HAS_VIDEO_EXTRAS)
      return negate + Column(field, type);
  }
  else if (type == MEDIA::TYPE::EPISODE)
  {
    if (field == Field::IN_PROGRESS)
      return "episode_view.idFile " + negate +
             " IN (SELECT DISTINCT idFile FROM bookmark WHERE type = 1)";
  }
  else if (type == MEDIA::TYPE::TV_SHOW)
  {
    if (field == Field::IN_PROGRESS)
      return negate +
             " ("
             "(tvshow_view.watchedcount > 0 AND tvshow_view.watchedcount < tvshow_view.totalCount) "
             "OR "
             "(tvshow_view.watchedcount = 0 AND EXISTS "
             "(SELECT 1 FROM episode_view WHERE episode_view.idShow = " +
             Column(Field::ID, type) +
             " AND episode_view.resumeTimeInSeconds > 0)"
             ")"
             ")";
    else if (field == Field::TRAILER)
      return negate + Column(field, type) + "!= ''";
  }
  return "";
}

CDatabaseQueryRule::SearchOperator OperatorOf(const MEDIA::CLibraryQueryRule& rule,
                                              MEDIA::TYPE type)
{
  const Field field{rule.m_field};
  CDatabaseQueryRule::SearchOperator op{rule.m_operator};
  if ((type == MEDIA::TYPE::TV_SHOW || type == MEDIA::TYPE::EPISODE) && field == Field::YEAR)
  { // special case for premiered which is a date rather than a year
    //! @todo SMARTPLAYLISTS do we really need this, or should we just make this field the premiered date and request a date?
    if (op == OPERATOR_EQUALS)
      op = OPERATOR_CONTAINS;
    else if (op == OPERATOR_DOES_NOT_EQUAL)
      op = OPERATOR_DOES_NOT_CONTAIN;
  }
  return op;
}

namespace
{
using MEDIA::LIBRARY_QUERY::GROUP;

struct Listing
{
  MEDIA::TYPE itemType;
  GROUP group;
  std::string_view path;
};

constexpr std::array<Listing, 25> LISTINGS{{
    {MEDIA::TYPE::MOVIE, GROUP::DEFAULT, DB_PATH::MOVIE_TITLES},
    {MEDIA::TYPE::MOVIE, GROUP::SET, DB_PATH::MOVIE_SETS},
    {MEDIA::TYPE::MOVIE, GROUP::GENRE, DB_PATH::MOVIE_GENRES},
    {MEDIA::TYPE::MOVIE, GROUP::YEAR, DB_PATH::MOVIE_YEARS},
    {MEDIA::TYPE::MOVIE, GROUP::PERFORMER, DB_PATH::MOVIE_ACTORS},
    {MEDIA::TYPE::MOVIE, GROUP::DIRECTOR, DB_PATH::MOVIE_DIRECTORS},
    {MEDIA::TYPE::MOVIE, GROUP::STUDIO, DB_PATH::MOVIE_STUDIOS},
    {MEDIA::TYPE::MOVIE, GROUP::COUNTRY, DB_PATH::MOVIE_COUNTRIES},
    {MEDIA::TYPE::MOVIE, GROUP::TAG, DB_PATH::MOVIE_TAGS},
    {MEDIA::TYPE::MOVIE, GROUP::VIDEO_VERSION, DB_PATH::MOVIE_VIDEO_VERSIONS},
    {MEDIA::TYPE::TV_SHOW, GROUP::DEFAULT, DB_PATH::TVSHOW_TITLES},
    {MEDIA::TYPE::TV_SHOW, GROUP::GENRE, DB_PATH::TVSHOW_GENRES},
    {MEDIA::TYPE::TV_SHOW, GROUP::YEAR, DB_PATH::TVSHOW_YEARS},
    {MEDIA::TYPE::TV_SHOW, GROUP::PERFORMER, DB_PATH::TVSHOW_ACTORS},
    {MEDIA::TYPE::TV_SHOW, GROUP::STUDIO, DB_PATH::TVSHOW_STUDIOS},
    {MEDIA::TYPE::TV_SHOW, GROUP::TAG, DB_PATH::TVSHOW_TAGS},
    {MEDIA::TYPE::EPISODE, GROUP::DEFAULT, DB_PATH::ALL_EPISODES},
    {MEDIA::TYPE::MUSIC_VIDEO, GROUP::DEFAULT, DB_PATH::MUSICVIDEO_TITLES},
    {MEDIA::TYPE::MUSIC_VIDEO, GROUP::PERFORMER, DB_PATH::MUSICVIDEO_ARTISTS},
    {MEDIA::TYPE::MUSIC_VIDEO, GROUP::ALBUM, DB_PATH::MUSICVIDEO_ALBUMS},
    {MEDIA::TYPE::MUSIC_VIDEO, GROUP::GENRE, DB_PATH::MUSICVIDEO_GENRES},
    {MEDIA::TYPE::MUSIC_VIDEO, GROUP::YEAR, DB_PATH::MUSICVIDEO_YEARS},
    {MEDIA::TYPE::MUSIC_VIDEO, GROUP::DIRECTOR, DB_PATH::MUSICVIDEO_DIRECTORS},
    {MEDIA::TYPE::MUSIC_VIDEO, GROUP::STUDIO, DB_PATH::MUSICVIDEO_STUDIOS},
    {MEDIA::TYPE::MUSIC_VIDEO, GROUP::TAG, DB_PATH::MUSICVIDEO_TAGS},
}};

struct GroupContentName
{
  GROUP group;
  std::string_view content;
};

// The performers of every kind of video are in the actor tables.
constexpr std::array<GroupContentName, 10> GROUP_CONTENTS{{
    {GROUP::SET, MEDIA::CONTENT::SETS},
    {GROUP::GENRE, MEDIA::CONTENT::GENRES},
    {GROUP::YEAR, MEDIA::CONTENT::YEARS},
    {GROUP::PERFORMER, MEDIA::CONTENT::ACTORS},
    {GROUP::DIRECTOR, MEDIA::CONTENT::DIRECTORS},
    {GROUP::STUDIO, MEDIA::CONTENT::STUDIOS},
    {GROUP::COUNTRY, MEDIA::CONTENT::COUNTRIES},
    {GROUP::ALBUM, MEDIA::CONTENT::ALBUMS},
    {GROUP::TAG, MEDIA::CONTENT::TAGS},
    {GROUP::VIDEO_VERSION, MEDIA::CONTENT::VIDEOVERSIONS},
}};
} // namespace

std::optional<std::string_view> ListingPath(MEDIA::TYPE itemType, MEDIA::LIBRARY_QUERY::GROUP group)
{
  const GROUP listed{group == GROUP::NONE ? GROUP::DEFAULT : group};
  const auto it =
      std::ranges::find_if(LISTINGS, [itemType, listed](const Listing& listing)
                           { return listing.itemType == itemType && listing.group == listed; });
  if (it == LISTINGS.end())
  {
    return std::nullopt;
  }
  return it->path;
}

std::string_view GroupContent(MEDIA::LIBRARY_QUERY::GROUP group)
{
  const auto it = std::ranges::find(GROUP_CONTENTS, group, &GroupContentName::group);
  return it == GROUP_CONTENTS.end() ? std::string_view{} : it->content;
}

namespace
{
//! The path the lists of a kind of item hang from
std::string NodesPath(MEDIA::TYPE itemType)
{
  switch (itemType)
  {
    case MEDIA::TYPE::MOVIE:
      return DB_PATH::MOVIES;
    case MEDIA::TYPE::TV_SHOW:
    case MEDIA::TYPE::EPISODE:
      return DB_PATH::TVSHOWS;
    case MEDIA::TYPE::MUSIC_VIDEO:
      return DB_PATH::MUSICVIDEOS;
    default:
      return DB_PATH::ROOT;
  }
}
} // namespace

bool BrowseValues(CVideoDatabase& db,
                  MEDIA::LIBRARY_QUERY::BROWSE browse,
                  MEDIA::TYPE itemType,
                  const std::string& baseDir,
                  const CDatabase::Filter& filter,
                  bool countOnly,
                  CFileItemList& items)
{
  using enum MEDIA::LIBRARY_QUERY::BROWSE;
  const bool whole{baseDir.empty()};
  const std::string nodes{NodesPath(itemType)};
  const auto dir = [&](const char* node) { return whole ? nodes + node : baseDir; };

  // An episode's genres, years, studios and tags are its show's
  const bool ofShow{itemType == MEDIA::TYPE::EPISODE &&
                    (browse == GENRES || browse == YEARS || browse == STUDIOS || browse == TAGS)};
  const VideoDbContentType content{ofShow ? VideoDbContentType::TVSHOWS
                                          : DatabaseUtils::VideoContentTypeFromMediaType(itemType)};

  switch (browse)
  {
    case GENRES:
      return db.GetGenresNav(dir("genres/"), items, content, filter, countOnly);
    case COUNTRIES:
      return db.GetCountriesNav(dir(""), items, content, filter, countOnly);
    case ARTISTS:
    case ALBUM_ARTISTS:
      if (!whole)
      {
        return db.GetActorsNav(baseDir, items, content, filter, countOnly);
      }
      if (itemType != MEDIA::TYPE::MUSIC_VIDEO)
      {
        return false;
      }
      db.GetMusicVideoArtistsByName("", items);
      return true;
    case ALBUMS:
      if (!whole)
      {
        return db.GetMusicVideoAlbumsNav(baseDir, items, filter, countOnly);
      }
      if (itemType != MEDIA::TYPE::MUSIC_VIDEO)
      {
        return false;
      }
      db.GetMusicVideoAlbumsByName("", items);
      return true;
    case ACTORS:
      return db.GetActorsNav(dir("actors/"), items, content, filter, countOnly);
    case YEARS:
      return db.GetYearsNav(dir("years/"), items, content, filter);
    case DIRECTORS:
      return db.GetDirectorsNav(dir("directors/"), items, content, filter, countOnly);
    case STUDIOS:
      return db.GetStudiosNav(dir("studios/"), items, content, filter, countOnly);
    case WRITERS:
      return db.GetWritersNav(dir(""), items, content, filter, countOnly);
    case TVSHOWS:
      return db.GetTvShowsNav(dir("titles/"), items);
    case TITLES:
      switch (itemType)
      {
        case MEDIA::TYPE::MOVIE:
          return db.GetMoviesNav(dir("titles/"), items);
        case MEDIA::TYPE::TV_SHOW:
          return db.GetTvShowsNav(dir("titles/"), items);
        case MEDIA::TYPE::EPISODE:
        {
          const bool listed{db.GetEpisodesNav(dir("titles/-1/-1/"), items)};
          // The title alone, where the library labels an episode with its season and number
          const CLabelFormatter format("%T", "");
          for (const auto& item : items)
          {
            format.FormatLabel(item.get());
          }
          return listed;
        }
        case MEDIA::TYPE::MUSIC_VIDEO:
          return db.GetMusicVideosNav(dir("titles/"), items);
        default:
          return false;
      }
    case SETS:
      return db.GetSetsNav(whole ? std::string{DB_PATH::MOVIE_SETS} : baseDir, items,
                           VideoDbContentType::MOVIES, filter);
    case TAGS:
      return db.GetTagsNav(dir("tags/"), items, content, filter, countOnly);
    default:
      return false;
  }
}

std::optional<MEDIA::LIBRARY_QUERY::RangeColumn> RangeColumnOf(Field field, MEDIA::TYPE itemType)
{
  using MEDIA::LIBRARY_QUERY::RangeColumn;
  if (field == Field::YEAR)
  {
    const std::string year{
        DatabaseUtils::GetField(Field::YEAR, itemType, DatabaseQueryPart::WHERE)};
    switch (itemType)
    {
      case MEDIA::TYPE::MOVIE:
        return RangeColumn{"movie_view", year, year + " > 0"};
      case MEDIA::TYPE::TV_SHOW:
      {
        const std::string firstAired{StringUtils::Format("strftime(\"%%Y\", {})", year)};
        return RangeColumn{"tvshow_view", firstAired, firstAired + " > 0"};
      }
      case MEDIA::TYPE::MUSIC_VIDEO:
        return RangeColumn{"musicvideo_view", year, year + " > 0"};
      default:
        return std::nullopt;
    }
  }
  if (field == Field::AIR_DATE && itemType == MEDIA::TYPE::EPISODE)
  {
    return RangeColumn{"episode_view",
                       StringUtils::Format("CAST(strftime(\"%%s\", {}) AS INTEGER)",
                                           DatabaseUtils::GetField(Field::AIR_DATE, itemType,
                                                                   DatabaseQueryPart::SELECT)),
                       ""};
  }
  return std::nullopt;
}

bool ItemsById(CVideoDatabase& db,
               MEDIA::TYPE itemType,
               const std::string& ids,
               CFileItemList& items)
{
  if (itemType != MEDIA::TYPE::MUSIC_VIDEO)
  {
    return false;
  }
  CFileItemList musicVideos;
  db.GetMusicVideosByWhere(DB_PATH::MUSICVIDEO_TITLES,
                           CDatabase::Filter("idMVideo IN (" + ids + ")"), musicVideos);
  items.Append(musicVideos);
  return true;
}

} // namespace KODI::VIDEO::LIBRARY_QUERY

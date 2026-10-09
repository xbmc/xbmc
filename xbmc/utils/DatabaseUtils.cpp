/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DatabaseUtils.h"

#include "dbwrappers/dataset.h"
#include "music/MusicDatabase.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoDatabaseColumns.h"

#include <algorithm>
#include <array>
#include <sstream>

using KODI::MEDIA::NameOf;

KODI::MEDIA::TYPE DatabaseUtils::MediaTypeFromVideoContentType(VideoDbContentType videoContentType)
{
  switch (videoContentType)
  {
    using enum VideoDbContentType;
    case MOVIES:
      return KODI::MEDIA::TYPE::MOVIE;

    case MOVIE_SETS:
      return KODI::MEDIA::TYPE::VIDEO_COLLECTION;

    case TVSHOWS:
      return KODI::MEDIA::TYPE::TV_SHOW;

    case EPISODES:
      return KODI::MEDIA::TYPE::EPISODE;

    case MUSICVIDEOS:
      return KODI::MEDIA::TYPE::MUSIC_VIDEO;

    default:
      break;
  }

  return KODI::MEDIA::TYPE::NONE;
}

VideoDbContentType DatabaseUtils::VideoContentTypeFromMediaType(KODI::MEDIA::TYPE mediaType)
{
  switch (mediaType)
  {
    using enum VideoDbContentType;
    case KODI::MEDIA::TYPE::MOVIE:
      return MOVIES;
    case KODI::MEDIA::TYPE::VIDEO_COLLECTION:
      return MOVIE_SETS;
    case KODI::MEDIA::TYPE::TV_SHOW:
      return TVSHOWS;
    case KODI::MEDIA::TYPE::EPISODE:
      return EPISODES;
    case KODI::MEDIA::TYPE::MUSIC_VIDEO:
      return MUSICVIDEOS;
    default:
      return UNKNOWN;
  }
}

const DatabaseUtils::View* DatabaseUtils::ViewOf(KODI::MEDIA::TYPE mediaType)
{
  static constexpr auto named = [](Field field, std::string_view name, int index = -1)
  { return Column{field, name, index, index}; };
  // A numbered column's position follows the item's id, and for all but a tv show its file id
  static constexpr auto numbered = [](Field field, int number, int offset)
  { return Column{field, {}, number, number + offset}; };

  // clang-format off
  static constexpr std::array ALBUM_COLUMNS{
      named(Field::ID,             "albumview.idAlbum",            CMusicDatabase::album_idAlbum),
      named(Field::ALBUM,          "albumview.strAlbum",           CMusicDatabase::album_strAlbum),
      named(Field::ARTIST,         "albumview.strArtists",         CMusicDatabase::album_strArtists),
      named(Field::ALBUM_ARTIST,   "albumview.strArtists",         CMusicDatabase::album_strArtists),
      named(Field::GENRE,          "albumview.strGenres",          CMusicDatabase::album_strGenres),
      named(Field::YEAR,           "albumview.strReleaseDate",     CMusicDatabase::album_strReleaseDate),
      named(Field::ORIG_YEAR,      "albumview.strOrigReleaseDate", CMusicDatabase::album_strOrigReleaseDate),
      named(Field::ORIG_DATE,      "albumview.strOrigReleaseDate", CMusicDatabase::album_strOrigReleaseDate),
      named(Field::MOODS,          "albumview.strMoods",           CMusicDatabase::album_strMoods),
      named(Field::STYLES,         "albumview.strStyles",          CMusicDatabase::album_strStyles),
      named(Field::THEMES,         "albumview.strThemes",          CMusicDatabase::album_strThemes),
      named(Field::REVIEW,         "albumview.strReview",          CMusicDatabase::album_strReview),
      named(Field::MUSIC_LABEL,    "albumview.strLabel",           CMusicDatabase::album_strLabel),
      named(Field::ALBUM_TYPE,     "albumview.strType",            CMusicDatabase::album_strType),
      named(Field::COMPILATION,    "albumview.bCompilation"),
      named(Field::RATING,         "albumview.fRating",            CMusicDatabase::album_fRating),
      named(Field::VOTES,          "albumview.iVotes",             CMusicDatabase::album_iVotes),
      named(Field::USER_RATING,    "albumview.iUserrating",        CMusicDatabase::album_iUserrating),
      named(Field::DATE_ADDED,     "albumview.dateAdded",          CMusicDatabase::album_dateAdded),
      named(Field::DATE_NEW,       "albumview.dateNew",            CMusicDatabase::album_dateNew),
      named(Field::DATE_MODIFIED,  "albumview.dateModified",       CMusicDatabase::album_dateModified),
      named(Field::PLAYCOUNT,      "albumview.iTimesPlayed",       CMusicDatabase::album_iTimesPlayed),
      named(Field::LAST_PLAYED,    "albumview.lastPlayed",         CMusicDatabase::album_dtLastPlayed),
      named(Field::TOTAL_DISCS,    "albumview.iDiscTotal",         CMusicDatabase::album_iTotalDiscs),
      named(Field::ALBUM_STATUS,   "albumview.strReleaseStatus",   CMusicDatabase::album_strReleaseStatus),
      named(Field::ALBUM_DURATION, "albumview.iAlbumDuration",     CMusicDatabase::album_iAlbumDuration),
  };

  static constexpr std::array SONG_COLUMNS{
      named(Field::ID,                 "songview.idSong",             CMusicDatabase::song_idSong),
      named(Field::TITLE,              "songview.strTitle",           CMusicDatabase::song_strTitle),
      named(Field::TRACK_NUMBER,       "songview.iTrack",             CMusicDatabase::song_iTrack),
      named(Field::TIME,               "songview.iDuration",          CMusicDatabase::song_iDuration),
      named(Field::YEAR,               "songview.strReleaseDate",     CMusicDatabase::song_strReleaseDate),
      named(Field::ORIG_YEAR,          "songview.strOrigReleaseDate"),
      named(Field::ORIG_DATE,          "songview.strOrigReleaseDate"),
      named(Field::FILENAME,           "songview.strFilename",        CMusicDatabase::song_strFileName),
      named(Field::PLAYCOUNT,          "songview.iTimesPlayed",       CMusicDatabase::song_iTimesPlayed),
      named(Field::START_OFFSET,       "songview.iStartOffset",       CMusicDatabase::song_iStartOffset),
      named(Field::END_OFFSET,         "songview.iEndOffset",         CMusicDatabase::song_iEndOffset),
      named(Field::LAST_PLAYED,        "songview.lastPlayed",         CMusicDatabase::song_lastplayed),
      named(Field::RATING,             "songview.rating",             CMusicDatabase::song_rating),
      named(Field::VOTES,              "songview.votes",              CMusicDatabase::song_votes),
      named(Field::USER_RATING,        "songview.userrating",         CMusicDatabase::song_userrating),
      named(Field::COMMENT,            "songview.comment",            CMusicDatabase::song_comment),
      named(Field::MOODS,              "songview.mood",               CMusicDatabase::song_mood),
      named(Field::ALBUM,              "songview.strAlbum",           CMusicDatabase::song_strAlbum),
      named(Field::PATH,               "songview.strPath",            CMusicDatabase::song_strPath),
      named(Field::ARTIST,             "songview.strArtists",         CMusicDatabase::song_strArtists),
      named(Field::ALBUM_ARTIST,       "songview.strArtists",         CMusicDatabase::song_strArtists),
      named(Field::GENRE,              "songview.strGenres",          CMusicDatabase::song_strGenres),
      named(Field::DATE_ADDED,         "songview.dateAdded",          CMusicDatabase::song_dateAdded),
      named(Field::DATE_NEW,           "songview.dateNew",            CMusicDatabase::song_dateNew),
      named(Field::DATE_MODIFIED,      "songview.dateModified",       CMusicDatabase::song_dateModified),
      named(Field::DISC_TITLE,         "songview.strDiscSubtitle"),
      named(Field::BPM,                "songview.iBPM",               CMusicDatabase::song_iBPM),
      named(Field::MUSIC_BITRATE,      "songview.iBitRate",           CMusicDatabase::song_iBitRate),
      named(Field::SAMPLE_RATE,        "songview.iSampleRate",        CMusicDatabase::song_iSampleRate),
      named(Field::NUMBER_OF_CHANNELS, "songview.iChannels",          CMusicDatabase::song_iChannels),
  };

  static constexpr std::array ARTIST_COLUMNS{
      named(Field::ID,             "artistview.idArtist",          CMusicDatabase::artist_idArtist),
      named(Field::ARTIST_SORT,    "artistview.strSortName",       CMusicDatabase::artist_strSortName),
      named(Field::ARTIST,         "artistview.strArtist",         CMusicDatabase::artist_strArtist),
      named(Field::ARTIST_TYPE,    "artistview.strType",           CMusicDatabase::artist_strType),
      named(Field::GENDER,         "artistview.strGender",         CMusicDatabase::artist_strGender),
      named(Field::DISAMBIGUATION, "artistview.strDisambiguation", CMusicDatabase::artist_strDisambiguation),
      named(Field::GENRE,          "artistview.strGenres",         CMusicDatabase::artist_strGenres),
      named(Field::MOODS,          "artistview.strMoods",          CMusicDatabase::artist_strMoods),
      named(Field::STYLES,         "artistview.strStyles",         CMusicDatabase::artist_strStyles),
      named(Field::INSTRUMENTS,    "artistview.strInstruments",    CMusicDatabase::artist_strInstruments),
      named(Field::BIOGRAPHY,      "artistview.strBiography",      CMusicDatabase::artist_strBiography),
      named(Field::BORN,           "artistview.strBorn",           CMusicDatabase::artist_strBorn),
      named(Field::BAND_FORMED,    "artistview.strFormed",         CMusicDatabase::artist_strFormed),
      named(Field::DISBANDED,      "artistview.strDisbanded",      CMusicDatabase::artist_strDisbanded),
      named(Field::DIED,           "artistview.strDied",           CMusicDatabase::artist_strDied),
      named(Field::DATE_ADDED,     "artistview.dateAdded",         CMusicDatabase::artist_dateAdded),
      named(Field::DATE_NEW,       "artistview.dateNew",           CMusicDatabase::artist_dateNew),
      named(Field::DATE_MODIFIED,  "artistview.dateModified",      CMusicDatabase::artist_dateModified),
  };

  static constexpr std::array MUSIC_VIDEO_COLUMNS{
      named(Field::ID,              "musicvideo_view.idMVideo",    0),
      numbered(Field::TITLE,        VIDEODB_ID_MUSICVIDEO_TITLE,    2),
      numbered(Field::TIME,         VIDEODB_ID_MUSICVIDEO_RUNTIME,  2),
      numbered(Field::DIRECTOR,     VIDEODB_ID_MUSICVIDEO_DIRECTOR, 2),
      numbered(Field::STUDIO,       VIDEODB_ID_MUSICVIDEO_STUDIOS,  2),
      named(Field::YEAR,            "musicvideo_view.premiered",   VIDEODB_DETAILS_MUSICVIDEO_PREMIERED),
      numbered(Field::PLOT,         VIDEODB_ID_MUSICVIDEO_PLOT,     2),
      numbered(Field::ALBUM,        VIDEODB_ID_MUSICVIDEO_ALBUM,    2),
      numbered(Field::ARTIST,       VIDEODB_ID_MUSICVIDEO_ARTIST,   2),
      numbered(Field::GENRE,        VIDEODB_ID_MUSICVIDEO_GENRE,    2),
      numbered(Field::TRACK_NUMBER, VIDEODB_ID_MUSICVIDEO_TRACK,    2),
      named(Field::FILENAME,        "musicvideo_view.strFilename", VIDEODB_DETAILS_MUSICVIDEO_FILE),
      named(Field::PATH,            "musicvideo_view.strPath",     VIDEODB_DETAILS_MUSICVIDEO_PATH),
      named(Field::PLAYCOUNT,       "musicvideo_view.playCount",   VIDEODB_DETAILS_MUSICVIDEO_PLAYCOUNT),
      named(Field::LAST_PLAYED,     "musicvideo_view.lastPlayed",  VIDEODB_DETAILS_MUSICVIDEO_LASTPLAYED),
      named(Field::DATE_ADDED,      "musicvideo_view.dateAdded",   VIDEODB_DETAILS_MUSICVIDEO_DATEADDED),
      named(Field::USER_RATING,     "musicvideo_view.userrating",  VIDEODB_DETAILS_MUSICVIDEO_USER_RATING),
  };

  static constexpr std::array MOVIE_COLUMNS{
      named(Field::ID,                "movie_view.idMovie",         0),
      numbered(Field::TITLE,          VIDEODB_ID_TITLE,             2),
      numbered(Field::PLOT,           VIDEODB_ID_PLOT,              2),
      numbered(Field::PLOT_OUTLINE,   VIDEODB_ID_PLOTOUTLINE,       2),
      numbered(Field::TAGLINE,        VIDEODB_ID_TAGLINE,           2),
      named(Field::VOTES,             "movie_view.votes",           VIDEODB_DETAILS_MOVIE_VOTES),
      named(Field::RATING,            "movie_view.rating",          VIDEODB_DETAILS_MOVIE_RATING),
      numbered(Field::WRITER,         VIDEODB_ID_CREDITS,           2),
      named(Field::YEAR,              "movie_view.premiered",       VIDEODB_DETAILS_MOVIE_PREMIERED),
      numbered(Field::SORT_TITLE,     VIDEODB_ID_SORTTITLE,         2),
      numbered(Field::ORIGINAL_TITLE, VIDEODB_ID_ORIGINALTITLE,     2),
      numbered(Field::TIME,           VIDEODB_ID_RUNTIME,           2),
      numbered(Field::MPAA,           VIDEODB_ID_MPAA,              2),
      numbered(Field::TOP250,         VIDEODB_ID_TOP250,            2),
      named(Field::SET,               "movie_view.strSet",          VIDEODB_DETAILS_MOVIE_SET_NAME),
      numbered(Field::GENRE,          VIDEODB_ID_GENRE,             2),
      numbered(Field::DIRECTOR,       VIDEODB_ID_DIRECTOR,          2),
      numbered(Field::STUDIO,         VIDEODB_ID_STUDIOS,           2),
      numbered(Field::TRAILER,        VIDEODB_ID_TRAILER,           2),
      numbered(Field::COUNTRY,        VIDEODB_ID_COUNTRY,           2),
      named(Field::FILENAME,          "movie_view.strFilename",     VIDEODB_DETAILS_MOVIE_FILE),
      named(Field::PATH,              "movie_view.strPath",         VIDEODB_DETAILS_MOVIE_PATH),
      named(Field::PLAYCOUNT,         "movie_view.playCount",       VIDEODB_DETAILS_MOVIE_PLAYCOUNT),
      named(Field::LAST_PLAYED,       "movie_view.lastPlayed",      VIDEODB_DETAILS_MOVIE_LASTPLAYED),
      named(Field::DATE_ADDED,        "movie_view.dateAdded",       VIDEODB_DETAILS_MOVIE_DATEADDED),
      named(Field::USER_RATING,       "movie_view.userrating",      VIDEODB_DETAILS_MOVIE_USER_RATING),
      named(Field::HAS_VIDEO_VERSIONS, "movie_view.hasVideoVersions"),
      named(Field::HAS_VIDEO_EXTRAS,  "movie_view.hasVideoExtras"),
  };

  static constexpr std::array TV_SHOW_COLUMNS{
      named(Field::ID,                         "tvshow_view.idShow",       0),
      numbered(Field::TITLE,                   VIDEODB_ID_TV_TITLE,         1),
      numbered(Field::PLOT,                    VIDEODB_ID_TV_PLOT,          1),
      numbered(Field::TVSHOW_STATUS,           VIDEODB_ID_TV_STATUS,        1),
      named(Field::VOTES,                      "tvshow_view.votes",        VIDEODB_DETAILS_TVSHOW_VOTES),
      named(Field::RATING,                     "tvshow_view.rating",       VIDEODB_DETAILS_TVSHOW_RATING),
      numbered(Field::YEAR,                    VIDEODB_ID_TV_PREMIERED,     1),
      numbered(Field::GENRE,                   VIDEODB_ID_TV_GENRE,         1),
      numbered(Field::MPAA,                    VIDEODB_ID_TV_MPAA,          1),
      numbered(Field::STUDIO,                  VIDEODB_ID_TV_STUDIOS,       1),
      numbered(Field::TRAILER,                 VIDEODB_ID_TV_TRAILER,       1),
      numbered(Field::SORT_TITLE,              VIDEODB_ID_TV_SORTTITLE,     1),
      numbered(Field::ORIGINAL_TITLE,          VIDEODB_ID_TV_ORIGINALTITLE, 1),
      named(Field::PATH,                       "tvshow_view.strPath",      VIDEODB_DETAILS_TVSHOW_PATH),
      named(Field::DATE_ADDED,                 "tvshow_view.dateAdded",    VIDEODB_DETAILS_TVSHOW_DATEADDED),
      named(Field::LAST_PLAYED,                "tvshow_view.lastPlayed",   VIDEODB_DETAILS_TVSHOW_LASTPLAYED),
      named(Field::SEASON,                     "tvshow_view.totalSeasons", VIDEODB_DETAILS_TVSHOW_NUM_SEASONS),
      named(Field::NUMBER_OF_EPISODES,         "tvshow_view.totalCount",   VIDEODB_DETAILS_TVSHOW_NUM_EPISODES),
      named(Field::NUMBER_OF_WATCHED_EPISODES, "tvshow_view.watchedcount", VIDEODB_DETAILS_TVSHOW_NUM_WATCHED),
      named(Field::USER_RATING,                "tvshow_view.userrating",   VIDEODB_DETAILS_TVSHOW_USER_RATING),
  };

  static constexpr std::array EPISODE_COLUMNS{
      named(Field::ID,                             "episode_view.idEpisode",   0),
      numbered(Field::TITLE,                       VIDEODB_ID_EPISODE_TITLE,       2),
      numbered(Field::PLOT,                        VIDEODB_ID_EPISODE_PLOT,        2),
      named(Field::VOTES,                          "episode_view.votes",       VIDEODB_DETAILS_EPISODE_VOTES),
      named(Field::RATING,                         "episode_view.rating",      VIDEODB_DETAILS_EPISODE_RATING),
      numbered(Field::WRITER,                      VIDEODB_ID_EPISODE_CREDITS,     2),
      numbered(Field::AIR_DATE,                    VIDEODB_ID_EPISODE_AIRED,       2),
      numbered(Field::TIME,                        VIDEODB_ID_EPISODE_RUNTIME,     2),
      numbered(Field::DIRECTOR,                    VIDEODB_ID_EPISODE_DIRECTOR,    2),
      numbered(Field::SEASON,                      VIDEODB_ID_EPISODE_SEASON,      2),
      numbered(Field::EPISODE_NUMBER,              VIDEODB_ID_EPISODE_EPISODE,     2),
      numbered(Field::UNIQUE_ID,                   VIDEODB_ID_EPISODE_IDENT_ID,    2),
      numbered(Field::EPISODE_NUMBER_SPECIAL_SORT, VIDEODB_ID_EPISODE_SORTEPISODE, 2),
      numbered(Field::SEASON_SPECIAL_SORT,         VIDEODB_ID_EPISODE_SORTSEASON,  2),
      named(Field::FILENAME,                       "episode_view.strFilename", VIDEODB_DETAILS_EPISODE_FILE),
      named(Field::PATH,                           "episode_view.strPath",     VIDEODB_DETAILS_EPISODE_PATH),
      named(Field::PLAYCOUNT,                      "episode_view.playCount",   VIDEODB_DETAILS_EPISODE_PLAYCOUNT),
      named(Field::LAST_PLAYED,                    "episode_view.lastPlayed",  VIDEODB_DETAILS_EPISODE_LASTPLAYED),
      named(Field::DATE_ADDED,                     "episode_view.dateAdded",   VIDEODB_DETAILS_EPISODE_DATEADDED),
      named(Field::TVSHOW_TITLE,                   "episode_view.strTitle",    VIDEODB_DETAILS_EPISODE_TVSHOW_NAME),
      named(Field::YEAR,                           "episode_view.premiered",   VIDEODB_DETAILS_EPISODE_TVSHOW_AIRED),
      named(Field::MPAA,                           "episode_view.mpaa",        VIDEODB_DETAILS_EPISODE_TVSHOW_MPAA),
      named(Field::STUDIO,                         "episode_view.strStudio",   VIDEODB_DETAILS_EPISODE_TVSHOW_STUDIO),
      named(Field::USER_RATING,                    "episode_view.userrating",  VIDEODB_DETAILS_EPISODE_USER_RATING),
  };
  // clang-format on

  static constexpr View ALBUM_VIEW{"albumview", ALBUM_COLUMNS};
  static constexpr View SONG_VIEW{"songview", SONG_COLUMNS};
  static constexpr View ARTIST_VIEW{"artistview", ARTIST_COLUMNS};
  static constexpr View MUSIC_VIDEO_VIEW{"musicvideo_view", MUSIC_VIDEO_COLUMNS};
  static constexpr View MOVIE_VIEW{"movie_view", MOVIE_COLUMNS};
  static constexpr View TV_SHOW_VIEW{"tvshow_view", TV_SHOW_COLUMNS};
  static constexpr View EPISODE_VIEW{"episode_view", EPISODE_COLUMNS};

  switch (mediaType)
  {
    case KODI::MEDIA::TYPE::ALBUM:
      return &ALBUM_VIEW;
    case KODI::MEDIA::TYPE::SONG:
      return &SONG_VIEW;
    case KODI::MEDIA::TYPE::ARTIST:
      return &ARTIST_VIEW;
    case KODI::MEDIA::TYPE::MUSIC_VIDEO:
      return &MUSIC_VIDEO_VIEW;
    case KODI::MEDIA::TYPE::MOVIE:
      return &MOVIE_VIEW;
    case KODI::MEDIA::TYPE::TV_SHOW:
      return &TV_SHOW_VIEW;
    case KODI::MEDIA::TYPE::EPISODE:
      return &EPISODE_VIEW;
    default:
      return nullptr;
  }
}

const DatabaseUtils::Column* DatabaseUtils::View::Find(Field field) const
{
  const auto column = std::ranges::find(columns, field, &Column::field);
  return column != columns.end() ? &*column : nullptr;
}

std::string DatabaseUtils::View::NameOf(const Column& column) const
{
  if (column.name.empty())
    return StringUtils::Format("{}.c{:02}", name, column.number);
  return std::string{column.name};
}

const DatabaseUtils::Column* DatabaseUtils::FindColumn(Field field, KODI::MEDIA::TYPE mediaType)
{
  const View* view = ViewOf(mediaType);
  return view ? view->Find(field) : nullptr;
}

std::string DatabaseUtils::GetField(Field field, KODI::MEDIA::TYPE mediaType, DatabaseQueryPart queryPart)
{
  if (field == Field::NONE || mediaType == KODI::MEDIA::TYPE::NONE)
    return "";

  if (const View* view = ViewOf(mediaType))
  {
    if (const Column* column = view->Find(field))
    {
      // A title is ordered by the sort title where one is set
      const Column* sortTitle = view->Find(Field::SORT_TITLE);
      if (field == Field::TITLE && queryPart == DatabaseQueryPart::ORDER_BY && sortTitle)
        return StringUtils::Format("CASE WHEN length({0}) > 0 THEN {0} ELSE {1} END",
                                   view->NameOf(*sortTitle), view->NameOf(*column));
      return view->NameOf(*column);
    }
  }

  if (field == Field::RANDOM && queryPart == DatabaseQueryPart::ORDER_BY)
    return "RANDOM()";

  return "";
}

int DatabaseUtils::GetField(Field field, KODI::MEDIA::TYPE mediaType)
{
  const Column* column = FindColumn(field, mediaType);
  return column ? column->number : -1;
}

int DatabaseUtils::GetFieldIndex(Field field, KODI::MEDIA::TYPE mediaType)
{
  const Column* column = FindColumn(field, mediaType);
  return column ? column->index : -1;
}

bool DatabaseUtils::GetSelectFields(const Fields &fields,
                                    KODI::MEDIA::TYPE mediaType, FieldList &selectFields)
{
  if (mediaType == KODI::MEDIA::TYPE::NONE || fields.empty())
    return false;

  Fields sortFields = fields;

  // add necessary fields to create the label
  switch (mediaType)
  {
    case KODI::MEDIA::TYPE::EPISODE:
      sortFields.insert(Field::TITLE);
      sortFields.insert(Field::SEASON);
    sortFields.insert(Field::EPISODE_NUMBER);
      break;
    case KODI::MEDIA::TYPE::SONG:
      sortFields.insert(Field::TITLE);
      sortFields.insert(Field::TRACK_NUMBER);
      break;
    case KODI::MEDIA::TYPE::VIDEO:
    case KODI::MEDIA::TYPE::VIDEO_COLLECTION:
    case KODI::MEDIA::TYPE::MUSIC_VIDEO:
    case KODI::MEDIA::TYPE::MOVIE:
    case KODI::MEDIA::TYPE::TV_SHOW:
      sortFields.insert(Field::TITLE);
      break;
    case KODI::MEDIA::TYPE::ALBUM:
      sortFields.insert(Field::ALBUM);
      break;
    case KODI::MEDIA::TYPE::ARTIST:
      sortFields.insert(Field::ARTIST);
      break;
    default:
      break;
  }

  selectFields.clear();
  for (const auto& field : sortFields)
  {
    // ignore FieldLabel because it needs special handling (see further up)
    if (field == Field::LABEL)
      continue;

    if (GetField(field, mediaType, DatabaseQueryPart::SELECT).empty())
    {
      CLog::Log(LOGDEBUG, "DatabaseUtils::GetSortFieldList: unknown field {}",
                static_cast<int>(field));
      continue;
    }
    selectFields.emplace_back(field);
  }

  return !selectFields.empty();
}

bool DatabaseUtils::GetFieldValue(const dbiplus::field_value &fieldValue, CVariant &variantValue)
{
  if (fieldValue.get_isNull())
  {
    variantValue = CVariant::ConstNullVariant;
    return true;
  }

  switch (fieldValue.get_fType())
  {
    using enum dbiplus::fType;
    case ft_String:
    case ft_WideString:
    case ft_Object:
      variantValue = fieldValue.get_asString();
      return true;
    case ft_Char:
    case ft_WChar:
      variantValue = fieldValue.get_asChar();
      return true;
    case ft_Boolean:
      variantValue = fieldValue.get_asBool();
      return true;
    case ft_Short:
      variantValue = fieldValue.get_asShort();
      return true;
    case ft_UShort:
      variantValue = fieldValue.get_asUShort();
      return true;
    case ft_Int:
      variantValue = fieldValue.get_asInt();
      return true;
    case ft_UInt:
      variantValue = fieldValue.get_asUInt();
      return true;
    case ft_Float:
      variantValue = fieldValue.get_asFloat();
      return true;
    case ft_Double:
    case ft_LongDouble:
      variantValue = fieldValue.get_asDouble();
      return true;
    case ft_Int64:
      variantValue = fieldValue.get_asInt64();
      return true;
  }

  return false;
}

bool DatabaseUtils::GetDatabaseResults(KODI::MEDIA::TYPE mediaType,
                                       const FieldList& fields,
                                       dbiplus::Dataset& dataset,
                                       DatabaseResults& results)
{
  if (dataset.num_rows() == 0)
    return true;

  const dbiplus::result_set& resultSet = dataset.get_result_set();
  const auto offset = static_cast<unsigned int>(results.size());

  if (fields.empty())
  {
    DatabaseResult result;
    for (unsigned int index = 0; index < resultSet.records.size(); index++)
    {
      result[Field::ROW] = index + offset;
      results.push_back(result);
    }

    return true;
  }

  if (resultSet.record_header.size() < fields.size())
    return false;

  std::vector<int> fieldIndexLookup;
  fieldIndexLookup.reserve(fields.size());
  for (const auto& field : fields)
    fieldIndexLookup.push_back(GetFieldIndex(field, mediaType));

  results.reserve(resultSet.records.size() + offset);
  for (unsigned int index = 0; index < resultSet.records.size(); index++)
  {
    DatabaseResult result;
    result[Field::ROW] = index + offset;

    unsigned int lookupIndex = 0;
    for (const auto& field : fields)
    {
      const int fieldIndex = fieldIndexLookup[lookupIndex];
      if (fieldIndex < 0)
        return false;

      lookupIndex++;

      std::pair<Field, CVariant> value;
      value.first = field;
      if (!GetFieldValue(resultSet.records[index]->at(fieldIndex), value.second))
        CLog::Log(LOGWARNING, "GetDatabaseResults: unable to retrieve value of field {}",
                  resultSet.record_header[fieldIndex].name);

      if (value.first == Field::YEAR &&
          (mediaType == KODI::MEDIA::TYPE::TV_SHOW || mediaType == KODI::MEDIA::TYPE::EPISODE ||
           mediaType == KODI::MEDIA::TYPE::MOVIE))
      {
        CDateTime dateTime;
        dateTime.SetFromDBDate(value.second.asString());
        if (dateTime.IsValid())
        {
          value.second.clear();
          value.second = dateTime.GetYear();
        }
      }

      result.insert(value);
    }

    result[Field::MEDIA_TYPE] = NameOf(mediaType);
    switch (mediaType)
    {
      case KODI::MEDIA::TYPE::MOVIE:
      case KODI::MEDIA::TYPE::VIDEO_COLLECTION:
      case KODI::MEDIA::TYPE::TV_SHOW:
      case KODI::MEDIA::TYPE::MUSIC_VIDEO:
        result[Field::LABEL] = result.at(Field::TITLE).asString();
        break;
      case KODI::MEDIA::TYPE::EPISODE:
      {
        std::ostringstream label;
        label << (result.at(Field::SEASON).asInteger() * 100 +
                  result.at(Field::EPISODE_NUMBER).asInteger());
        label << ". ";
        label << result.at(Field::TITLE).asString();
        result[Field::LABEL] = label.str();
        break;
      }
      case KODI::MEDIA::TYPE::ALBUM:
        result[Field::LABEL] = result.at(Field::ALBUM).asString();
        break;
      case KODI::MEDIA::TYPE::SONG:
      {
        std::ostringstream label;
        label << result.at(Field::TRACK_NUMBER).asInteger();
        label << ". ";
        label << result.at(Field::TITLE).asString();
        result[Field::LABEL] = label.str();
        break;
      }
      case KODI::MEDIA::TYPE::ARTIST:
        result[Field::LABEL] = result.at(Field::ARTIST).asString();
        break;
      default:
        break;
    }

    results.push_back(result);
  }

  return true;
}

std::string DatabaseUtils::BuildLimitClause(int end, int start /* = 0 */)
{
  return " LIMIT " + BuildLimitClauseOnly(end, start);
}

std::string DatabaseUtils::BuildLimitClauseOnly(int end, int start /* = 0 */)
{
  std::ostringstream sql;
  if (start > 0)
  {
    if (end > 0)
    {
      end = end - start;
      if (end < 0)
        end = 0;
    }

    sql << start << "," << end;
  }
  else
    sql << end;

  return sql.str();
}

size_t DatabaseUtils::GetLimitCount(int end, int start)
{
  if (start > 0)
  {
    if (end - start < 0)
      return 0;
    else
      return static_cast<size_t>(end - start);
  }
  else if (end > 0)
    return static_cast<size_t>(end);
  return 0;
}

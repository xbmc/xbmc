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

#include <sstream>

MediaType DatabaseUtils::MediaTypeFromVideoContentType(VideoDbContentType videoContentType)
{
  switch (videoContentType)
  {
    using enum VideoDbContentType;
    case MOVIES:
      return MediaTypeMovie;

    case MOVIE_SETS:
      return MediaTypeVideoCollection;

    case TVSHOWS:
      return MediaTypeTvShow;

    case EPISODES:
      return MediaTypeEpisode;

    case MUSICVIDEOS:
      return MediaTypeMusicVideo;

    default:
      break;
  }

  return MediaTypeNone;
}

std::string DatabaseUtils::GetField(Field field, const MediaType &mediaType, DatabaseQueryPart queryPart)
{
  if (field == Field::NONE || mediaType == MediaTypeNone)
    return "";

  if (mediaType == MediaTypeAlbum)
  {
    if (field == Field::ID)
      return "albumview.idAlbum";
    else if (field == Field::ALBUM)
      return "albumview.strAlbum";
    else if (field == Field::ARTIST || field == Field::ALBUM_ARTIST)
      return "albumview.strArtists";
    else if (field == Field::GENRE)
      return "albumview.strGenres";
    else if (field == Field::YEAR)
      return "albumview.strReleaseDate";
    else if (field == Field::ORIG_YEAR || field == Field::ORIG_DATE)
      return "albumview.strOrigReleaseDate";
    else if (field == Field::MOODS)
      return "albumview.strMoods";
    else if (field == Field::STYLES)
      return "albumview.strStyles";
    else if (field == Field::THEMES)
      return "albumview.strThemes";
    else if (field == Field::REVIEW)
      return "albumview.strReview";
    else if (field == Field::MUSIC_LABEL)
      return "albumview.strLabel";
    else if (field == Field::ALBUM_TYPE)
      return "albumview.strType";
    else if (field == Field::COMPILATION)
      return "albumview.bCompilation";
    else if (field == Field::RATING)
      return "albumview.fRating";
    else if (field == Field::VOTES)
      return "albumview.iVotes";
    else if (field == Field::USER_RATING)
      return "albumview.iUserrating";
    else if (field == Field::DATE_ADDED)
      return "albumview.dateAdded";
    else if (field == Field::DATE_NEW)
      return "albumview.dateNew";
    else if (field == Field::DATE_MODIFIED)
      return "albumview.dateModified";
    else if (field == Field::PLAYCOUNT)
      return "albumview.iTimesPlayed";
    else if (field == Field::LAST_PLAYED)
      return "albumview.lastPlayed";
    else if (field == Field::TOTAL_DISCS)
      return "albumview.iDiscTotal";
    else if (field == Field::ALBUM_STATUS)
      return "albumview.strReleaseStatus";
    else if (field == Field::ALBUM_DURATION)
      return "albumview.iAlbumDuration";
  }
  else if (mediaType == MediaTypeSong)
  {
    if (field == Field::ID)
      return "songview.idSong";
    else if (field == Field::TITLE)
      return "songview.strTitle";
    else if (field == Field::TRACK_NUMBER)
      return "songview.iTrack";
    else if (field == Field::TIME)
      return "songview.iDuration";
    else if (field == Field::YEAR)
      return "songview.strReleaseDate";
    else if (field == Field::ORIG_YEAR || field == Field::ORIG_DATE)
      return "songview.strOrigReleaseDate";
    else if (field == Field::FILENAME)
      return "songview.strFilename";
    else if (field == Field::PLAYCOUNT)
      return "songview.iTimesPlayed";
    else if (field == Field::START_OFFSET)
      return "songview.iStartOffset";
    else if (field == Field::END_OFFSET)
      return "songview.iEndOffset";
    else if (field == Field::LAST_PLAYED)
      return "songview.lastPlayed";
    else if (field == Field::RATING)
      return "songview.rating";
    else if (field == Field::VOTES)
      return "songview.votes";
    else if (field == Field::USER_RATING)
      return "songview.userrating";
    else if (field == Field::COMMENT)
      return "songview.comment";
    else if (field == Field::MOODS)
      return "songview.mood";
    else if (field == Field::ALBUM)
      return "songview.strAlbum";
    else if (field == Field::PATH)
      return "songview.strPath";
    else if (field == Field::ARTIST || field == Field::ALBUM_ARTIST)
      return "songview.strArtists";
    else if (field == Field::GENRE)
      return "songview.strGenres";
    else if (field == Field::DATE_ADDED)
      return "songview.dateAdded";
    else if (field == Field::DATE_NEW)
      return "songview.dateNew";
    else if (field == Field::DATE_MODIFIED)
      return "songview.dateModified";

    else if (field == Field::DISC_TITLE)
      return "songview.strDiscSubtitle";
    else if (field == Field::BPM)
      return "songview.iBPM";
    else if (field == Field::MUSIC_BITRATE)
      return "songview.iBitRate";
    else if (field == Field::SAMPLE_RATE)
      return "songview.iSampleRate";
    else if (field == Field::NUMBER_OF_CHANNELS)
      return "songview.iChannels";
  }
  else if (mediaType == MediaTypeArtist)
  {
    if (field == Field::ID)
      return "artistview.idArtist";
    else if (field == Field::ARTIST_SORT)
      return "artistview.strSortName";
    else if (field == Field::ARTIST)
      return "artistview.strArtist";
    else if (field == Field::ARTIST_TYPE)
      return "artistview.strType";
    else if (field == Field::GENDER)
      return "artistview.strGender";
    else if (field == Field::DISAMBIGUATION)
      return "artistview.strDisambiguation";
    else if (field == Field::GENRE)
      return "artistview.strGenres";
    else if (field == Field::MOODS)
      return "artistview.strMoods";
    else if (field == Field::STYLES)
      return "artistview.strStyles";
    else if (field == Field::INSTRUMENTS)
      return "artistview.strInstruments";
    else if (field == Field::BIOGRAPHY)
      return "artistview.strBiography";
    else if (field == Field::BORN)
      return "artistview.strBorn";
    else if (field == Field::BAND_FORMED)
      return "artistview.strFormed";
    else if (field == Field::DISBANDED)
      return "artistview.strDisbanded";
    else if (field == Field::DIED)
      return "artistview.strDied";
    else if (field == Field::DATE_ADDED)
      return "artistview.dateAdded";
    else if (field == Field::DATE_NEW)
      return "artistview.dateNew";
    else if (field == Field::DATE_MODIFIED)
      return "artistview.dateModified";
  }
  else if (mediaType == MediaTypeMusicVideo)
  {
    std::string result;
    if (field == Field::ID)
      return "musicvideo_view.idMVideo";
    else if (field == Field::TITLE)
      result = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_TITLE);
    else if (field == Field::TIME)
      result = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_RUNTIME);
    else if (field == Field::DIRECTOR)
      result = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_DIRECTOR);
    else if (field == Field::STUDIO)
      result = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_STUDIOS);
    else if (field == Field::YEAR)
      return "musicvideo_view.premiered";
    else if (field == Field::PLOT)
      result = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_PLOT);
    else if (field == Field::ALBUM)
      result = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_ALBUM);
    else if (field == Field::ARTIST)
      result = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_ARTIST);
    else if (field == Field::GENRE)
      result = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_GENRE);
    else if (field == Field::TRACK_NUMBER)
      result = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_TRACK);
    else if (field == Field::FILENAME)
      return "musicvideo_view.strFilename";
    else if (field == Field::PATH)
      return "musicvideo_view.strPath";
    else if (field == Field::PLAYCOUNT)
      return "musicvideo_view.playCount";
    else if (field == Field::LAST_PLAYED)
      return "musicvideo_view.lastPlayed";
    else if (field == Field::DATE_ADDED)
      return "musicvideo_view.dateAdded";
    else if (field == Field::USER_RATING)
      return "musicvideo_view.userrating";

    if (!result.empty())
      return result;
  }
  else if (mediaType == MediaTypeMovie)
  {
    std::string result;
    if (field == Field::ID)
      return "movie_view.idMovie";
    else if (field == Field::TITLE)
    {
      // We need some extra logic to get the title value if sorttitle isn't set
      if (queryPart == DatabaseQueryPart::ORDER_BY)
        result = StringUtils::Format("CASE WHEN length(movie_view.c{:02}) > 0 THEN "
                                     "movie_view.c{:02} ELSE movie_view.c{:02} END",
                                     VIDEODB_ID_SORTTITLE, VIDEODB_ID_SORTTITLE, VIDEODB_ID_TITLE);
      else
        result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TITLE);
    }
    else if (field == Field::PLOT)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_PLOT);
    else if (field == Field::PLOT_OUTLINE)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_PLOTOUTLINE);
    else if (field == Field::TAGLINE)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TAGLINE);
    else if (field == Field::VOTES)
      return "movie_view.votes";
    else if (field == Field::RATING)
      return "movie_view.rating";
    else if (field == Field::WRITER)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_CREDITS);
    else if (field == Field::YEAR)
      return "movie_view.premiered";
    else if (field == Field::SORT_TITLE)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_SORTTITLE);
    else if (field == Field::ORIGINAL_TITLE)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_ORIGINALTITLE);
    else if (field == Field::TIME)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_RUNTIME);
    else if (field == Field::MPAA)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_MPAA);
    else if (field == Field::TOP250)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TOP250);
    else if (field == Field::SET)
      return "movie_view.strSet";
    else if (field == Field::GENRE)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_GENRE);
    else if (field == Field::DIRECTOR)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_DIRECTOR);
    else if (field == Field::STUDIO)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_STUDIOS);
    else if (field == Field::TRAILER)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TRAILER);
    else if (field == Field::COUNTRY)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_COUNTRY);
    else if (field == Field::FILENAME)
      return "movie_view.strFilename";
    else if (field == Field::PATH)
      return "movie_view.strPath";
    else if (field == Field::PLAYCOUNT)
      return "movie_view.playCount";
    else if (field == Field::LAST_PLAYED)
      return "movie_view.lastPlayed";
    else if (field == Field::DATE_ADDED)
      return "movie_view.dateAdded";
    else if (field == Field::USER_RATING)
      return "movie_view.userrating";
    else if (field == Field::HAS_VIDEO_VERSIONS)
      return "movie_view.hasVideoVersions";
    else if (field == Field::HAS_VIDEO_EXTRAS)
      return "movie_view.hasVideoExtras";

    if (!result.empty())
      return result;
  }
  else if (mediaType == MediaTypeTvShow)
  {
    std::string result;
    if (field == Field::ID)
      return "tvshow_view.idShow";
    else if (field == Field::TITLE)
    {
      // We need some extra logic to get the title value if sorttitle isn't set
      if (queryPart == DatabaseQueryPart::ORDER_BY)
        result = StringUtils::Format("CASE WHEN length(tvshow_view.c{:02}) > 0 THEN "
                                     "tvshow_view.c{:02} ELSE tvshow_view.c{:02} END",
                                     VIDEODB_ID_TV_SORTTITLE, VIDEODB_ID_TV_SORTTITLE,
                                     VIDEODB_ID_TV_TITLE);
      else
        result = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_TITLE);
    }
    else if (field == Field::PLOT)
      result = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_PLOT);
    else if (field == Field::TVSHOW_STATUS)
      result = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_STATUS);
    else if (field == Field::VOTES)
      return "tvshow_view.votes";
    else if (field == Field::RATING)
      return "tvshow_view.rating";
    else if (field == Field::YEAR)
      result = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_PREMIERED);
    else if (field == Field::GENRE)
      result = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_GENRE);
    else if (field == Field::MPAA)
      result = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_MPAA);
    else if (field == Field::STUDIO)
      result = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_STUDIOS);
    else if (field == Field::TRAILER)
      result = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_TRAILER);
    else if (field == Field::SORT_TITLE)
      result = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_SORTTITLE);
    else if (field == Field::ORIGINAL_TITLE)
      result = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_ORIGINALTITLE);
    else if (field == Field::PATH)
      return "tvshow_view.strPath";
    else if (field == Field::DATE_ADDED)
      return "tvshow_view.dateAdded";
    else if (field == Field::LAST_PLAYED)
      return "tvshow_view.lastPlayed";
    else if (field == Field::SEASON)
      return "tvshow_view.totalSeasons";
    else if (field == Field::NUMBER_OF_EPISODES)
      return "tvshow_view.totalCount";
    else if (field == Field::NUMBER_OF_WATCHED_EPISODES)
      return "tvshow_view.watchedcount";
    else if (field == Field::USER_RATING)
      return "tvshow_view.userrating";

    if (!result.empty())
      return result;
  }
  else if (mediaType == MediaTypeEpisode)
  {
    std::string result;
    if (field == Field::ID)
      return "episode_view.idEpisode";
    else if (field == Field::TITLE)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_TITLE);
    else if (field == Field::PLOT)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_PLOT);
    else if (field == Field::VOTES)
      return "episode_view.votes";
    else if (field == Field::RATING)
      return "episode_view.rating";
    else if (field == Field::WRITER)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_CREDITS);
    else if (field == Field::AIR_DATE)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_AIRED);
    else if (field == Field::TIME)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_RUNTIME);
    else if (field == Field::DIRECTOR)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_DIRECTOR);
    else if (field == Field::SEASON)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_SEASON);
    else if (field == Field::EPISODE_NUMBER)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_EPISODE);
    else if (field == Field::UNIQUE_ID)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_IDENT_ID);
    else if (field == Field::EPISODE_NUMBER_SPECIAL_SORT)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_SORTEPISODE);
    else if (field == Field::SEASON_SPECIAL_SORT)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_SORTSEASON);
    else if (field == Field::FILENAME)
      return "episode_view.strFilename";
    else if (field == Field::PATH)
      return "episode_view.strPath";
    else if (field == Field::PLAYCOUNT)
      return "episode_view.playCount";
    else if (field == Field::LAST_PLAYED)
      return "episode_view.lastPlayed";
    else if (field == Field::DATE_ADDED)
      return "episode_view.dateAdded";
    else if (field == Field::TVSHOW_TITLE)
      return "episode_view.strTitle";
    else if (field == Field::YEAR)
      return "episode_view.premiered";
    else if (field == Field::MPAA)
      return "episode_view.mpaa";
    else if (field == Field::STUDIO)
      return "episode_view.strStudio";
    else if (field == Field::USER_RATING)
      return "episode_view.userrating";

    if (!result.empty())
      return result;
  }

  if (field == Field::RANDOM && queryPart == DatabaseQueryPart::ORDER_BY)
    return "RANDOM()";

  return "";
}

int DatabaseUtils::GetField(Field field, const MediaType &mediaType)
{
  if (field == Field::NONE || mediaType == MediaTypeNone)
    return -1;

  return GetField(field, mediaType, false);
}

int DatabaseUtils::GetFieldIndex(Field field, const MediaType &mediaType)
{
  if (field == Field::NONE || mediaType == MediaTypeNone)
    return -1;

  return GetField(field, mediaType, true);
}

bool DatabaseUtils::GetSelectFields(const Fields &fields, const MediaType &mediaType, FieldList &selectFields)
{
  if (mediaType == MediaTypeNone || fields.empty())
    return false;

  Fields sortFields = fields;

  // add necessary fields to create the label
  if (mediaType == MediaTypeSong || mediaType == MediaTypeVideo || mediaType == MediaTypeVideoCollection ||
      mediaType == MediaTypeMusicVideo || mediaType == MediaTypeMovie || mediaType == MediaTypeTvShow || mediaType == MediaTypeEpisode)
    sortFields.insert(Field::TITLE);
  if (mediaType == MediaTypeEpisode)
  {
    sortFields.insert(Field::SEASON);
    sortFields.insert(Field::EPISODE_NUMBER);
  }
  else if (mediaType == MediaTypeAlbum)
    sortFields.insert(Field::ALBUM);
  else if (mediaType == MediaTypeSong)
    sortFields.insert(Field::TRACK_NUMBER);
  else if (mediaType == MediaTypeArtist)
    sortFields.insert(Field::ARTIST);

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

bool DatabaseUtils::GetDatabaseResults(const MediaType& mediaType,
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
          (mediaType == MediaTypeTvShow || mediaType == MediaTypeEpisode ||
           mediaType == MediaTypeMovie))
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

    result[Field::MEDIA_TYPE] = mediaType;
    if (mediaType == MediaTypeMovie || mediaType == MediaTypeVideoCollection ||
        mediaType == MediaTypeTvShow || mediaType == MediaTypeMusicVideo)
      result[Field::LABEL] = result.at(Field::TITLE).asString();
    else if (mediaType == MediaTypeEpisode)
    {
      std::ostringstream label;
      label << (result.at(Field::SEASON).asInteger() * 100 +
                result.at(Field::EPISODE_NUMBER).asInteger());
      label << ". ";
      label << result.at(Field::TITLE).asString();
      result[Field::LABEL] = label.str();
    }
    else if (mediaType == MediaTypeAlbum)
      result[Field::LABEL] = result.at(Field::ALBUM).asString();
    else if (mediaType == MediaTypeSong)
    {
      std::ostringstream label;
      label << result.at(Field::TRACK_NUMBER).asInteger();
      label << ". ";
      label << result.at(Field::TITLE).asString();
      result[Field::LABEL] = label.str();
    }
    else if (mediaType == MediaTypeArtist)
      result[Field::LABEL] = result.at(Field::ARTIST).asString();

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

int DatabaseUtils::GetField(Field field, const MediaType &mediaType, bool asIndex)
{
  if (field == Field::NONE || mediaType == MediaTypeNone)
    return -1;

  int index = -1;

  if (mediaType == MediaTypeAlbum)
  {
    if (field == Field::ID)
      return CMusicDatabase::album_idAlbum;
    else if (field == Field::ALBUM)
      return CMusicDatabase::album_strAlbum;
    else if (field == Field::ARTIST || field == Field::ALBUM_ARTIST)
      return CMusicDatabase::album_strArtists;
    else if (field == Field::GENRE)
      return CMusicDatabase::album_strGenres;
    else if (field == Field::YEAR)
      return CMusicDatabase::album_strReleaseDate;
    else if (field == Field::MOODS)
      return CMusicDatabase::album_strMoods;
    else if (field == Field::STYLES)
      return CMusicDatabase::album_strStyles;
    else if (field == Field::THEMES)
      return CMusicDatabase::album_strThemes;
    else if (field == Field::REVIEW)
      return CMusicDatabase::album_strReview;
    else if (field == Field::MUSIC_LABEL)
      return CMusicDatabase::album_strLabel;
    else if (field == Field::ALBUM_TYPE)
      return CMusicDatabase::album_strType;
    else if (field == Field::RATING)
      return CMusicDatabase::album_fRating;
    else if (field == Field::VOTES)
      return CMusicDatabase::album_iVotes;
    else if (field == Field::USER_RATING)
      return CMusicDatabase::album_iUserrating;
    else if (field == Field::PLAYCOUNT)
      return CMusicDatabase::album_iTimesPlayed;
    else if (field == Field::LAST_PLAYED)
      return CMusicDatabase::album_dtLastPlayed;
    else if (field == Field::DATE_ADDED)
      return CMusicDatabase::album_dateAdded;
    else if (field == Field::DATE_NEW)
      return CMusicDatabase::album_dateNew;
    else if (field == Field::DATE_MODIFIED)
      return CMusicDatabase::album_dateModified;
    else if (field == Field::TOTAL_DISCS)
      return CMusicDatabase::album_iTotalDiscs;
    else if (field == Field::ORIG_YEAR || field == Field::ORIG_DATE)
      return CMusicDatabase::album_strOrigReleaseDate;
    else if (field == Field::ALBUM_STATUS)
      return CMusicDatabase::album_strReleaseStatus;
    else if (field == Field::ALBUM_DURATION)
      return CMusicDatabase::album_iAlbumDuration;
  }
  else if (mediaType == MediaTypeSong)
  {
    if (field == Field::ID)
      return CMusicDatabase::song_idSong;
    else if (field == Field::TITLE)
      return CMusicDatabase::song_strTitle;
    else if (field == Field::TRACK_NUMBER)
      return CMusicDatabase::song_iTrack;
    else if (field == Field::TIME)
      return CMusicDatabase::song_iDuration;
    else if (field == Field::YEAR)
      return CMusicDatabase::song_strReleaseDate;
    else if (field == Field::FILENAME)
      return CMusicDatabase::song_strFileName;
    else if (field == Field::PLAYCOUNT)
      return CMusicDatabase::song_iTimesPlayed;
    else if (field == Field::START_OFFSET)
      return CMusicDatabase::song_iStartOffset;
    else if (field == Field::END_OFFSET)
      return CMusicDatabase::song_iEndOffset;
    else if (field == Field::LAST_PLAYED)
      return CMusicDatabase::song_lastplayed;
    else if (field == Field::RATING)
      return CMusicDatabase::song_rating;
    else if (field == Field::USER_RATING)
      return CMusicDatabase::song_userrating;
    else if (field == Field::VOTES)
      return CMusicDatabase::song_votes;
    else if (field == Field::COMMENT)
      return CMusicDatabase::song_comment;
    else if (field == Field::MOODS)
      return CMusicDatabase::song_mood;
    else if (field == Field::ALBUM)
      return CMusicDatabase::song_strAlbum;
    else if (field == Field::PATH)
      return CMusicDatabase::song_strPath;
    else if (field == Field::GENRE)
      return CMusicDatabase::song_strGenres;
    else if (field == Field::ARTIST || field == Field::ALBUM_ARTIST)
      return CMusicDatabase::song_strArtists;
    else if (field == Field::DATE_ADDED)
      return CMusicDatabase::song_dateAdded;
    else if (field == Field::DATE_NEW)
      return CMusicDatabase::song_dateNew;
    else if (field == Field::DATE_MODIFIED)
      return CMusicDatabase::song_dateModified;
    else if (field == Field::BPM)
      return CMusicDatabase::song_iBPM;
    else if (field == Field::MUSIC_BITRATE)
      return CMusicDatabase::song_iBitRate;
    else if (field == Field::SAMPLE_RATE)
      return CMusicDatabase::song_iSampleRate;
    else if (field == Field::NUMBER_OF_CHANNELS)
      return CMusicDatabase::song_iChannels;
  }
  else if (mediaType == MediaTypeArtist)
  {
    if (field == Field::ID)
      return CMusicDatabase::artist_idArtist;
    else if (field == Field::ARTIST)
      return CMusicDatabase::artist_strArtist;
    else if (field == Field::ARTIST_SORT)
      return CMusicDatabase::artist_strSortName;
    else if (field == Field::ARTIST_TYPE)
      return CMusicDatabase::artist_strType;
    else if (field == Field::GENDER)
      return CMusicDatabase::artist_strGender;
    else if (field == Field::DISAMBIGUATION)
      return CMusicDatabase::artist_strDisambiguation;
    else if (field == Field::GENRE)
      return CMusicDatabase::artist_strGenres;
    else if (field == Field::MOODS)
      return CMusicDatabase::artist_strMoods;
    else if (field == Field::STYLES)
      return CMusicDatabase::artist_strStyles;
    else if (field == Field::INSTRUMENTS)
      return CMusicDatabase::artist_strInstruments;
    else if (field == Field::BIOGRAPHY)
      return CMusicDatabase::artist_strBiography;
    else if (field == Field::BORN)
      return CMusicDatabase::artist_strBorn;
    else if (field == Field::BAND_FORMED)
      return CMusicDatabase::artist_strFormed;
    else if (field == Field::DISBANDED)
      return CMusicDatabase::artist_strDisbanded;
    else if (field == Field::DIED)
      return CMusicDatabase::artist_strDied;
    else if (field == Field::DATE_ADDED)
      return CMusicDatabase::artist_dateAdded;
    else if (field == Field::DATE_NEW)
      return CMusicDatabase::artist_dateNew;
    else if (field == Field::DATE_MODIFIED)
      return CMusicDatabase::artist_dateModified;
  }
  else if (mediaType == MediaTypeMusicVideo)
  {
    if (field == Field::ID)
      return 0;
    else if (field == Field::TITLE)
      index = VIDEODB_ID_MUSICVIDEO_TITLE;
    else if (field == Field::TIME)
      index = VIDEODB_ID_MUSICVIDEO_RUNTIME;
    else if (field == Field::DIRECTOR)
      index = VIDEODB_ID_MUSICVIDEO_DIRECTOR;
    else if (field == Field::STUDIO)
      index = VIDEODB_ID_MUSICVIDEO_STUDIOS;
    else if (field == Field::YEAR)
      return VIDEODB_DETAILS_MUSICVIDEO_PREMIERED;
    else if (field == Field::PLOT)
      index = VIDEODB_ID_MUSICVIDEO_PLOT;
    else if (field == Field::ALBUM)
      index = VIDEODB_ID_MUSICVIDEO_ALBUM;
    else if (field == Field::ARTIST)
      index = VIDEODB_ID_MUSICVIDEO_ARTIST;
    else if (field == Field::GENRE)
      index = VIDEODB_ID_MUSICVIDEO_GENRE;
    else if (field == Field::TRACK_NUMBER)
      index = VIDEODB_ID_MUSICVIDEO_TRACK;
    else if (field == Field::FILENAME)
      return VIDEODB_DETAILS_MUSICVIDEO_FILE;
    else if (field == Field::PATH)
      return VIDEODB_DETAILS_MUSICVIDEO_PATH;
    else if (field == Field::PLAYCOUNT)
      return VIDEODB_DETAILS_MUSICVIDEO_PLAYCOUNT;
    else if (field == Field::LAST_PLAYED)
      return VIDEODB_DETAILS_MUSICVIDEO_LASTPLAYED;
    else if (field == Field::DATE_ADDED)
      return VIDEODB_DETAILS_MUSICVIDEO_DATEADDED;
    else if (field == Field::USER_RATING)
      return VIDEODB_DETAILS_MUSICVIDEO_USER_RATING;

    if (index < 0)
      return index;

    if (asIndex)
    {
      // see VideoDatabase.h
      // the first field is the item's ID and the second is the item's file ID
      index += 2;
    }
  }
  else if (mediaType == MediaTypeMovie)
  {
    if (field == Field::ID)
      return 0;
    else if (field == Field::TITLE)
      index = VIDEODB_ID_TITLE;
    else if (field == Field::SORT_TITLE)
      index = VIDEODB_ID_SORTTITLE;
    else if (field == Field::ORIGINAL_TITLE)
      index = VIDEODB_ID_ORIGINALTITLE;
    else if (field == Field::PLOT)
      index = VIDEODB_ID_PLOT;
    else if (field == Field::PLOT_OUTLINE)
      index = VIDEODB_ID_PLOTOUTLINE;
    else if (field == Field::TAGLINE)
      index = VIDEODB_ID_TAGLINE;
    else if (field == Field::VOTES)
      return VIDEODB_DETAILS_MOVIE_VOTES;
    else if (field == Field::RATING)
      return VIDEODB_DETAILS_MOVIE_RATING;
    else if (field == Field::WRITER)
      index = VIDEODB_ID_CREDITS;
    else if (field == Field::YEAR)
      return VIDEODB_DETAILS_MOVIE_PREMIERED;
    else if (field == Field::TIME)
      index = VIDEODB_ID_RUNTIME;
    else if (field == Field::MPAA)
      index = VIDEODB_ID_MPAA;
    else if (field == Field::TOP250)
      index = VIDEODB_ID_TOP250;
    else if (field == Field::SET)
      return VIDEODB_DETAILS_MOVIE_SET_NAME;
    else if (field == Field::GENRE)
      index = VIDEODB_ID_GENRE;
    else if (field == Field::DIRECTOR)
      index = VIDEODB_ID_DIRECTOR;
    else if (field == Field::STUDIO)
      index = VIDEODB_ID_STUDIOS;
    else if (field == Field::TRAILER)
      index = VIDEODB_ID_TRAILER;
    else if (field == Field::COUNTRY)
      index = VIDEODB_ID_COUNTRY;
    else if (field == Field::FILENAME)
      index = VIDEODB_DETAILS_MOVIE_FILE;
    else if (field == Field::PATH)
      return VIDEODB_DETAILS_MOVIE_PATH;
    else if (field == Field::PLAYCOUNT)
      return VIDEODB_DETAILS_MOVIE_PLAYCOUNT;
    else if (field == Field::LAST_PLAYED)
      return VIDEODB_DETAILS_MOVIE_LASTPLAYED;
    else if (field == Field::DATE_ADDED)
      return VIDEODB_DETAILS_MOVIE_DATEADDED;
    else if (field == Field::USER_RATING)
      return VIDEODB_DETAILS_MOVIE_USER_RATING;

    if (index < 0)
      return index;

    if (asIndex)
    {
      // see VideoDatabase.h
      // the first field is the item's ID and the second is the item's file ID
      index += 2;
    }
  }
  else if (mediaType == MediaTypeTvShow)
  {
    // clang-format off
    if (field == Field::ID) return 0;
    else if (field == Field::TITLE) index = VIDEODB_ID_TV_TITLE;
    else if (field == Field::SORT_TITLE) index = VIDEODB_ID_TV_SORTTITLE;
    else if (field == Field::ORIGINAL_TITLE) index = VIDEODB_ID_TV_ORIGINALTITLE;
    else if (field == Field::PLOT) index = VIDEODB_ID_TV_PLOT;
    else if (field == Field::TVSHOW_STATUS) index = VIDEODB_ID_TV_STATUS;
    else if (field == Field::VOTES) return VIDEODB_DETAILS_TVSHOW_VOTES;
    else if (field == Field::RATING) return VIDEODB_DETAILS_TVSHOW_RATING;
    else if (field == Field::YEAR) index = VIDEODB_ID_TV_PREMIERED;
    else if (field == Field::GENRE) index = VIDEODB_ID_TV_GENRE;
    else if (field == Field::MPAA) index = VIDEODB_ID_TV_MPAA;
    else if (field == Field::STUDIO) index = VIDEODB_ID_TV_STUDIOS;
    else if (field == Field::TRAILER) index = VIDEODB_ID_TV_TRAILER;
    else if (field == Field::PATH) return VIDEODB_DETAILS_TVSHOW_PATH;
    else if (field == Field::DATE_ADDED) return VIDEODB_DETAILS_TVSHOW_DATEADDED;
    else if (field == Field::LAST_PLAYED) return VIDEODB_DETAILS_TVSHOW_LASTPLAYED;
    else if (field == Field::NUMBER_OF_EPISODES) return VIDEODB_DETAILS_TVSHOW_NUM_EPISODES;
    else if (field == Field::NUMBER_OF_WATCHED_EPISODES) return VIDEODB_DETAILS_TVSHOW_NUM_WATCHED;
    else if (field == Field::SEASON) return VIDEODB_DETAILS_TVSHOW_NUM_SEASONS;
    else if (field == Field::USER_RATING) return VIDEODB_DETAILS_TVSHOW_USER_RATING;
    // clang-format on
    if (index < 0)
      return index;

    if (asIndex)
    {
      // see VideoDatabase.h
      // the first field is the item's ID
      index += 1;
    }
  }
  else if (mediaType == MediaTypeEpisode)
  {
    if (field == Field::ID)
      return 0;
    else if (field == Field::TITLE)
      index = VIDEODB_ID_EPISODE_TITLE;
    else if (field == Field::PLOT)
      index = VIDEODB_ID_EPISODE_PLOT;
    else if (field == Field::VOTES)
      return VIDEODB_DETAILS_EPISODE_VOTES;
    else if (field == Field::RATING)
      return VIDEODB_DETAILS_EPISODE_RATING;
    else if (field == Field::WRITER)
      index = VIDEODB_ID_EPISODE_CREDITS;
    else if (field == Field::AIR_DATE)
      index = VIDEODB_ID_EPISODE_AIRED;
    else if (field == Field::TIME)
      index = VIDEODB_ID_EPISODE_RUNTIME;
    else if (field == Field::DIRECTOR)
      index = VIDEODB_ID_EPISODE_DIRECTOR;
    else if (field == Field::SEASON)
      index = VIDEODB_ID_EPISODE_SEASON;
    else if (field == Field::EPISODE_NUMBER)
      index = VIDEODB_ID_EPISODE_EPISODE;
    else if (field == Field::UNIQUE_ID)
      index = VIDEODB_ID_EPISODE_IDENT_ID;
    else if (field == Field::EPISODE_NUMBER_SPECIAL_SORT)
      index = VIDEODB_ID_EPISODE_SORTEPISODE;
    else if (field == Field::SEASON_SPECIAL_SORT)
      index = VIDEODB_ID_EPISODE_SORTSEASON;
    else if (field == Field::FILENAME)
      return VIDEODB_DETAILS_EPISODE_FILE;
    else if (field == Field::PATH)
      return VIDEODB_DETAILS_EPISODE_PATH;
    else if (field == Field::PLAYCOUNT)
      return VIDEODB_DETAILS_EPISODE_PLAYCOUNT;
    else if (field == Field::LAST_PLAYED)
      return VIDEODB_DETAILS_EPISODE_LASTPLAYED;
    else if (field == Field::DATE_ADDED)
      return VIDEODB_DETAILS_EPISODE_DATEADDED;
    else if (field == Field::TVSHOW_TITLE)
      return VIDEODB_DETAILS_EPISODE_TVSHOW_NAME;
    else if (field == Field::STUDIO)
      return VIDEODB_DETAILS_EPISODE_TVSHOW_STUDIO;
    else if (field == Field::YEAR)
      return VIDEODB_DETAILS_EPISODE_TVSHOW_AIRED;
    else if (field == Field::MPAA)
      return VIDEODB_DETAILS_EPISODE_TVSHOW_MPAA;
    else if (field == Field::USER_RATING)
      return VIDEODB_DETAILS_EPISODE_USER_RATING;

    if (index < 0)
      return index;

    if (asIndex)
    {
      // see VideoDatabase.h
      // the first field is the item's ID and the second is the item's file ID
      index += 2;
    }
  }

  return index;
}

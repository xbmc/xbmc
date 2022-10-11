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

#include <sstream>

MediaType DatabaseUtils::MediaTypeFromVideoContentType(VideoDbContentType videoContentType)
{
  switch (videoContentType)
  {
    case VideoDbContentType::MOVIES:
      return MediaTypeMovie;

    case VideoDbContentType::MOVIE_SETS:
      return MediaTypeVideoCollection;

    case VideoDbContentType::TVSHOWS:
      return MediaTypeTvShow;

    case VideoDbContentType::EPISODES:
      return MediaTypeEpisode;

    case VideoDbContentType::MUSICVIDEOS:
      return MediaTypeMusicVideo;

    default:
      break;
  }

  return MediaTypeNone;
}

std::string DatabaseUtils::GetField(Field field, const MediaType &mediaType, DatabaseQueryPart queryPart)
{
  if (field == FieldNone || mediaType == MediaTypeNone)
    return "";

  if (mediaType == MediaTypeAlbum)
  {
    if (field == FieldId) return "albumview.idAlbum";
    else if (field == FieldAlbum) return "albumview.strAlbum";
    else if (field == FieldArtist || field == FieldAlbumArtist) return "albumview.strArtists";
    else if (field == FieldGenre)
      return "albumview.strGenres";
    else if (field == FieldYear)
      return "albumview.strReleaseDate";
    else if (field == FieldOrigYear || field == FieldOrigDate)
      return "albumview.strOrigReleaseDate";
    else if (field == FieldMoods) return "albumview.strMoods";
    else if (field == FieldStyles) return "albumview.strStyles";
    else if (field == FieldThemes) return "albumview.strThemes";
    else if (field == FieldReview) return "albumview.strReview";
    else if (field == FieldMusicLabel) return "albumview.strLabel";
    else if (field == FieldAlbumType) return "albumview.strType";
    else if (field == FieldCompilation) return "albumview.bCompilation";
    else if (field == FieldRating) return "albumview.fRating";
    else if (field == FieldVotes) return "albumview.iVotes";
    else if (field == FieldUserRating) return "albumview.iUserrating";
    else if (field == FieldDateAdded) return "albumview.dateAdded";
    else if (field == FieldDateNew) return "albumview.dateNew";
    else if (field == FieldDateModified) return "albumview.dateModified";
    else if (field == FieldPlaycount) return "albumview.iTimesPlayed";
    else if (field == FieldLastPlayed) return "albumview.lastPlayed";
    else if (field == FieldTotalDiscs)
      return "albumview.iDiscTotal";
    else if (field == FieldAlbumStatus)
        return "albumview.strReleaseStatus";
    else if (field == FieldAlbumDuration)
      return "albumview.iAlbumDuration";
  }
  else if (mediaType == MediaTypeSong)
  {
    if (field == FieldId) return "songview.idSong";
    else if (field == FieldTitle) return "songview.strTitle";
    else if (field == FieldTrackNumber) return "songview.iTrack";
    else if (field == FieldTime) return "songview.iDuration";
    else if (field == FieldYear)
      return "songview.strReleaseDate";
    else if (field == FieldOrigYear || field == FieldOrigDate)
      return "songview.strOrigReleaseDate";
    else if (field == FieldFilename) return "songview.strFilename";
    else if (field == FieldPlaycount) return "songview.iTimesPlayed";
    else if (field == FieldStartOffset) return "songview.iStartOffset";
    else if (field == FieldEndOffset) return "songview.iEndOffset";
    else if (field == FieldLastPlayed) return "songview.lastPlayed";
    else if (field == FieldRating) return "songview.rating";
    else if (field == FieldVotes) return "songview.votes";
    else if (field == FieldUserRating) return "songview.userrating";
    else if (field == FieldComment) return "songview.comment";
    else if (field == FieldMoods) return "songview.mood";
    else if (field == FieldAlbum) return "songview.strAlbum";
    else if (field == FieldPath) return "songview.strPath";
    else if (field == FieldArtist || field == FieldAlbumArtist) return "songview.strArtists";
    else if (field == FieldGenre)
      return "songview.strGenres";
    else if (field == FieldDateAdded) return "songview.dateAdded";
    else if (field == FieldDateNew) return "songview.dateNew";
    else if (field == FieldDateModified) return "songview.dateModified";

    else if (field == FieldDiscTitle)
      return "songview.strDiscSubtitle";
    else if (field == FieldBPM)
        return "songview.iBPM";
    else if (field == FieldMusicBitRate)
        return "songview.iBitRate";
    else if (field == FieldSampleRate)
        return "songview.iSampleRate";
    else if (field == FieldNoOfChannels)
        return "songview.iChannels";
  }
  else if (mediaType == MediaTypeArtist)
  {
    if (field == FieldId) return "artistview.idArtist";
    else if (field == FieldArtistSort) return "artistview.strSortName";
    else if (field == FieldArtist) return "artistview.strArtist";
    else if (field == FieldArtistType) return "artistview.strType";
    else if (field == FieldGender) return "artistview.strGender";
    else if (field == FieldDisambiguation) return "artistview.strDisambiguation";
    else if (field == FieldGenre) return "artistview.strGenres";
    else if (field == FieldMoods) return "artistview.strMoods";
    else if (field == FieldStyles) return "artistview.strStyles";
    else if (field == FieldInstruments) return "artistview.strInstruments";
    else if (field == FieldBiography) return "artistview.strBiography";
    else if (field == FieldBorn) return "artistview.strBorn";
    else if (field == FieldBandFormed) return "artistview.strFormed";
    else if (field == FieldDisbanded) return "artistview.strDisbanded";
    else if (field == FieldDied) return "artistview.strDied";
    else if (field == FieldDateAdded) return "artistview.dateAdded";
    else if (field == FieldDateNew) return "artistview.dateNew";
    else if (field == FieldDateModified) return "artistview.dateModified";
  }
  else if (mediaType == MediaTypeMusicVideo)
  {
    std::string result;
    if (field == FieldId) return "musicvideo_view.idMVideo";
    else if (field == FieldTitle)
      result = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_TITLE);
    else if (field == FieldTime)
      result = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_RUNTIME);
    else if (field == FieldDirector)
      result = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_DIRECTOR);
    else if (field == FieldStudio)
      result = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_STUDIOS);
    else if (field == FieldYear) return "musicvideo_view.premiered";
    else if (field == FieldPlot)
      result = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_PLOT);
    else if (field == FieldAlbum)
      result = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_ALBUM);
    else if (field == FieldArtist)
      result = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_ARTIST);
    else if (field == FieldGenre)
      result = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_GENRE);
    else if (field == FieldTrackNumber)
      result = StringUtils::Format("musicvideo_view.c{:02}", VIDEODB_ID_MUSICVIDEO_TRACK);
    else if (field == FieldFilename) return "musicvideo_view.strFilename";
    else if (field == FieldPath) return "musicvideo_view.strPath";
    else if (field == FieldPlaycount) return "musicvideo_view.playCount";
    else if (field == FieldLastPlayed) return "musicvideo_view.lastPlayed";
    else if (field == FieldDateAdded) return "musicvideo_view.dateAdded";
    else if (field == FieldUserRating) return "musicvideo_view.userrating";

    if (!result.empty())
      return result;
  }
  else if (mediaType == MediaTypeMovie)
  {
    std::string result;
    if (field == FieldId) return "movie_view.idMovie";
    else if (field == FieldTitle)
    {
      // We need some extra logic to get the title value if sorttitle isn't set
      if (queryPart == DatabaseQueryPartOrderBy)
        result = StringUtils::Format("CASE WHEN length(movie_view.c{:02}) > 0 THEN "
                                     "movie_view.c{:02} ELSE movie_view.c{:02} END",
                                     VIDEODB_ID_SORTTITLE, VIDEODB_ID_SORTTITLE, VIDEODB_ID_TITLE);
      else
        result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TITLE);
    }
    else if (field == FieldPlot)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_PLOT);
    else if (field == FieldPlotOutline)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_PLOTOUTLINE);
    else if (field == FieldTagline)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TAGLINE);
    else if (field == FieldVotes) return "movie_view.votes";
    else if (field == FieldRating) return "movie_view.rating";
    else if (field == FieldWriter)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_CREDITS);
    else if (field == FieldYear) return "movie_view.premiered";
    else if (field == FieldSortTitle)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_SORTTITLE);
    else if (field == FieldOriginalTitle)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_ORIGINALTITLE);
    else if (field == FieldTime)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_RUNTIME);
    else if (field == FieldMPAA)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_MPAA);
    else if (field == FieldTop250)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TOP250);
    else if (field == FieldSet) return "movie_view.strSet";
    else if (field == FieldGenre)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_GENRE);
    else if (field == FieldDirector)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_DIRECTOR);
    else if (field == FieldStudio)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_STUDIOS);
    else if (field == FieldTrailer)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_TRAILER);
    else if (field == FieldCountry)
      result = StringUtils::Format("movie_view.c{:02}", VIDEODB_ID_COUNTRY);
    else if (field == FieldFilename) return "movie_view.strFilename";
    else if (field == FieldPath) return "movie_view.strPath";
    else if (field == FieldPlaycount) return "movie_view.playCount";
    else if (field == FieldLastPlayed) return "movie_view.lastPlayed";
    else if (field == FieldDateAdded) return "movie_view.dateAdded";
    else if (field == FieldUserRating) return "movie_view.userrating";

    if (!result.empty())
      return result;
  }
  else if (mediaType == MediaTypeTvShow)
  {
    std::string result;
    if (field == FieldId) return "tvshow_view.idShow";
    else if (field == FieldTitle)
    {
      // We need some extra logic to get the title value if sorttitle isn't set
      if (queryPart == DatabaseQueryPartOrderBy)
        result = StringUtils::Format("CASE WHEN length(tvshow_view.c{:02}) > 0 THEN "
                                     "tvshow_view.c{:02} ELSE tvshow_view.c{:02} END",
                                     VIDEODB_ID_TV_SORTTITLE, VIDEODB_ID_TV_SORTTITLE,
                                     VIDEODB_ID_TV_TITLE);
      else
        result = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_TITLE);
    }
    else if (field == FieldPlot)
      result = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_PLOT);
    else if (field == FieldTvShowStatus)
      result = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_STATUS);
    else if (field == FieldVotes) return "tvshow_view.votes";
    else if (field == FieldRating) return "tvshow_view.rating";
    else if (field == FieldYear)
      result = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_PREMIERED);
    else if (field == FieldGenre)
      result = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_GENRE);
    else if (field == FieldMPAA)
      result = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_MPAA);
    else if (field == FieldStudio)
      result = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_STUDIOS);
    else if (field == FieldSortTitle)
      result = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_SORTTITLE);
    else if (field == FieldOriginalTitle)
      result = StringUtils::Format("tvshow_view.c{:02}", VIDEODB_ID_TV_ORIGINALTITLE);
    else if (field == FieldPath) return "tvshow_view.strPath";
    else if (field == FieldDateAdded) return "tvshow_view.dateAdded";
    else if (field == FieldLastPlayed) return "tvshow_view.lastPlayed";
    else if (field == FieldSeason) return "tvshow_view.totalSeasons";
    else if (field == FieldNumberOfEpisodes) return "tvshow_view.totalCount";
    else if (field == FieldNumberOfWatchedEpisodes) return "tvshow_view.watchedcount";
    else if (field == FieldUserRating) return "tvshow_view.userrating";

    if (!result.empty())
      return result;
  }
  else if (mediaType == MediaTypeEpisode)
  {
    std::string result;
    if (field == FieldId) return "episode_view.idEpisode";
    else if (field == FieldTitle)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_TITLE);
    else if (field == FieldPlot)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_PLOT);
    else if (field == FieldVotes) return "episode_view.votes";
    else if (field == FieldRating) return "episode_view.rating";
    else if (field == FieldWriter)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_CREDITS);
    else if (field == FieldAirDate)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_AIRED);
    else if (field == FieldTime)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_RUNTIME);
    else if (field == FieldDirector)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_DIRECTOR);
    else if (field == FieldSeason)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_SEASON);
    else if (field == FieldEpisodeNumber)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_EPISODE);
    else if (field == FieldUniqueId)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_IDENT_ID);
    else if (field == FieldEpisodeNumberSpecialSort)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_SORTEPISODE);
    else if (field == FieldSeasonSpecialSort)
      result = StringUtils::Format("episode_view.c{:02}", VIDEODB_ID_EPISODE_SORTSEASON);
    else if (field == FieldFilename) return "episode_view.strFilename";
    else if (field == FieldPath) return "episode_view.strPath";
    else if (field == FieldPlaycount) return "episode_view.playCount";
    else if (field == FieldLastPlayed) return "episode_view.lastPlayed";
    else if (field == FieldDateAdded) return "episode_view.dateAdded";
    else if (field == FieldTvShowTitle) return "episode_view.strTitle";
    else if (field == FieldYear) return "episode_view.premiered";
    else if (field == FieldMPAA) return "episode_view.mpaa";
    else if (field == FieldStudio) return "episode_view.strStudio";
    else if (field == FieldUserRating) return "episode_view.userrating";

    if (!result.empty())
      return result;
  }

  if (field == FieldRandom && queryPart == DatabaseQueryPartOrderBy)
    return "RANDOM()";

  return "";
}

int DatabaseUtils::GetField(Field field, const MediaType &mediaType)
{
  if (field == FieldNone || mediaType == MediaTypeNone)
    return -1;

  return GetField(field, mediaType, false);
}

int DatabaseUtils::GetFieldIndex(Field field, const MediaType &mediaType)
{
  if (field == FieldNone || mediaType == MediaTypeNone)
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
    sortFields.insert(FieldTitle);
  if (mediaType == MediaTypeEpisode)
  {
    sortFields.insert(FieldSeason);
    sortFields.insert(FieldEpisodeNumber);
  }
  else if (mediaType == MediaTypeAlbum)
    sortFields.insert(FieldAlbum);
  else if (mediaType == MediaTypeSong)
    sortFields.insert(FieldTrackNumber);
  else if (mediaType == MediaTypeArtist)
    sortFields.insert(FieldArtist);

  selectFields.clear();
  for (Fields::const_iterator it = sortFields.begin(); it != sortFields.end(); ++it)
  {
    // ignore FieldLabel because it needs special handling (see further up)
    if (*it == FieldLabel)
      continue;

    if (GetField(*it, mediaType, DatabaseQueryPartSelect).empty())
    {
      CLog::Log(LOGDEBUG, "DatabaseUtils::GetSortFieldList: unknown field {}", *it);
      continue;
    }
    selectFields.push_back(*it);
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
  case dbiplus::ft_String:
  case dbiplus::ft_WideString:
  case dbiplus::ft_Object:
    variantValue = fieldValue.get_asString();
    return true;
  case dbiplus::ft_Char:
  case dbiplus::ft_WChar:
    variantValue = fieldValue.get_asChar();
    return true;
  case dbiplus::ft_Boolean:
    variantValue = fieldValue.get_asBool();
    return true;
  case dbiplus::ft_Short:
    variantValue = fieldValue.get_asShort();
    return true;
  case dbiplus::ft_UShort:
    variantValue = fieldValue.get_asShort();
    return true;
  case dbiplus::ft_Int:
    variantValue = fieldValue.get_asInt();
    return true;
  case dbiplus::ft_UInt:
    variantValue = fieldValue.get_asUInt();
    return true;
  case dbiplus::ft_Float:
    variantValue = fieldValue.get_asFloat();
    return true;
  case dbiplus::ft_Double:
  case dbiplus::ft_LongDouble:
    variantValue = fieldValue.get_asDouble();
    return true;
  case dbiplus::ft_Int64:
    variantValue = fieldValue.get_asInt64();
    return true;
  }

  return false;
}

bool DatabaseUtils::GetDatabaseResults(const MediaType &mediaType, const FieldList &fields, const std::unique_ptr<dbiplus::Dataset> &dataset, DatabaseResults &results)
{
  if (dataset->num_rows() == 0)
    return true;

  const dbiplus::result_set &resultSet = dataset->get_result_set();
  unsigned int offset = results.size();

  if (fields.empty())
  {
    DatabaseResult result;
    for (unsigned int index = 0; index < resultSet.records.size(); index++)
    {
      result[FieldRow] = index + offset;
      results.push_back(result);
    }

    return true;
  }

  if (resultSet.record_header.size() < fields.size())
    return false;

  std::vector<int> fieldIndexLookup;
  fieldIndexLookup.reserve(fields.size());
  for (FieldList::const_iterator it = fields.begin(); it != fields.end(); ++it)
    fieldIndexLookup.push_back(GetFieldIndex(*it, mediaType));

  results.reserve(resultSet.records.size() + offset);
  for (unsigned int index = 0; index < resultSet.records.size(); index++)
  {
    DatabaseResult result;
    result[FieldRow] = index + offset;

    unsigned int lookupIndex = 0;
    for (FieldList::const_iterator it = fields.begin(); it != fields.end(); ++it)
    {
      int fieldIndex = fieldIndexLookup[lookupIndex++];
      if (fieldIndex < 0)
        return false;

      std::pair<Field, CVariant> value;
      value.first = *it;
      if (!GetFieldValue(resultSet.records[index]->at(fieldIndex), value.second))
        CLog::Log(LOGWARNING, "GetDatabaseResults: unable to retrieve value of field {}",
                  resultSet.record_header[fieldIndex].name);

      if (value.first == FieldYear &&
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

    result[FieldMediaType] = mediaType;
    if (mediaType == MediaTypeMovie || mediaType == MediaTypeVideoCollection ||
        mediaType == MediaTypeTvShow || mediaType == MediaTypeMusicVideo)
      result[FieldLabel] = result.at(FieldTitle).asString();
    else if (mediaType == MediaTypeEpisode)
    {
      std::ostringstream label;
      label << (int)(result.at(FieldSeason).asInteger() * 100 + result.at(FieldEpisodeNumber).asInteger());
      label << ". ";
      label << result.at(FieldTitle).asString();
      result[FieldLabel] = label.str();
    }
    else if (mediaType == MediaTypeAlbum)
      result[FieldLabel] = result.at(FieldAlbum).asString();
    else if (mediaType == MediaTypeSong)
    {
      std::ostringstream label;
      label << (int)result.at(FieldTrackNumber).asInteger();
      label << ". ";
      label << result.at(FieldTitle).asString();
      result[FieldLabel] = label.str();
    }
    else if (mediaType == MediaTypeArtist)
      result[FieldLabel] = result.at(FieldArtist).asString();

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
  if (field == FieldNone || mediaType == MediaTypeNone)
    return -1;

  int index = -1;

  if (mediaType == MediaTypeAlbum)
  {
    if (field == FieldId) return CMusicDatabase::album_idAlbum;
    else if (field == FieldAlbum) return CMusicDatabase::album_strAlbum;
    else if (field == FieldArtist || field == FieldAlbumArtist) return CMusicDatabase::album_strArtists;
    else if (field == FieldGenre) return CMusicDatabase::album_strGenres;
    else if (field == FieldYear) return CMusicDatabase::album_strReleaseDate;
    else if (field == FieldMoods) return CMusicDatabase::album_strMoods;
    else if (field == FieldStyles) return CMusicDatabase::album_strStyles;
    else if (field == FieldThemes) return CMusicDatabase::album_strThemes;
    else if (field == FieldReview) return CMusicDatabase::album_strReview;
    else if (field == FieldMusicLabel) return CMusicDatabase::album_strLabel;
    else if (field == FieldAlbumType) return CMusicDatabase::album_strType;
    else if (field == FieldRating) return CMusicDatabase::album_fRating;
    else if (field == FieldVotes) return CMusicDatabase::album_iVotes;
    else if (field == FieldUserRating) return CMusicDatabase::album_iUserrating;
    else if (field == FieldPlaycount) return CMusicDatabase::album_iTimesPlayed;
    else if (field == FieldLastPlayed) return CMusicDatabase::album_dtLastPlayed;
    else if (field == FieldDateAdded) return CMusicDatabase::album_dateAdded;
    else if (field == FieldDateNew) return CMusicDatabase::album_dateNew;
    else if (field == FieldDateModified) return CMusicDatabase::album_dateModified;
    else if (field == FieldTotalDiscs)
      return CMusicDatabase::album_iTotalDiscs;
    else if (field == FieldOrigYear || field == FieldOrigDate)
      return CMusicDatabase::album_strOrigReleaseDate;
    else if (field == FieldAlbumStatus)
      return CMusicDatabase::album_strReleaseStatus;
    else if (field == FieldAlbumDuration)
      return CMusicDatabase::album_iAlbumDuration;
  }
  else if (mediaType == MediaTypeSong)
  {
    if (field == FieldId) return CMusicDatabase::song_idSong;
    else if (field == FieldTitle) return CMusicDatabase::song_strTitle;
    else if (field == FieldTrackNumber) return CMusicDatabase::song_iTrack;
    else if (field == FieldTime) return CMusicDatabase::song_iDuration;
    else if (field == FieldYear) return CMusicDatabase::song_strReleaseDate;
    else if (field == FieldFilename) return CMusicDatabase::song_strFileName;
    else if (field == FieldPlaycount) return CMusicDatabase::song_iTimesPlayed;
    else if (field == FieldStartOffset) return CMusicDatabase::song_iStartOffset;
    else if (field == FieldEndOffset) return CMusicDatabase::song_iEndOffset;
    else if (field == FieldLastPlayed) return CMusicDatabase::song_lastplayed;
    else if (field == FieldRating) return CMusicDatabase::song_rating;
    else if (field == FieldUserRating) return CMusicDatabase::song_userrating;
    else if (field == FieldVotes) return CMusicDatabase::song_votes;
    else if (field == FieldComment) return CMusicDatabase::song_comment;
    else if (field == FieldMoods) return CMusicDatabase::song_mood;
    else if (field == FieldAlbum) return CMusicDatabase::song_strAlbum;
    else if (field == FieldPath) return CMusicDatabase::song_strPath;
    else if (field == FieldGenre) return CMusicDatabase::song_strGenres;
    else if (field == FieldArtist || field == FieldAlbumArtist) return CMusicDatabase::song_strArtists;
    else if (field == FieldDateAdded) return CMusicDatabase::song_dateAdded;
    else if (field == FieldDateNew) return CMusicDatabase::song_dateNew;
    else if (field == FieldDateModified) return CMusicDatabase::song_dateModified;
    else if (field == FieldBPM)
      return CMusicDatabase::song_iBPM;
    else if (field == FieldMusicBitRate)
        return CMusicDatabase::song_iBitRate;
    else if (field == FieldSampleRate)
        return CMusicDatabase::song_iSampleRate;
    else if (field == FieldNoOfChannels)
        return CMusicDatabase::song_iChannels;
  }
  else if (mediaType == MediaTypeArtist)
  {
    if (field == FieldId) return CMusicDatabase::artist_idArtist;
    else if (field == FieldArtist) return CMusicDatabase::artist_strArtist;
    else if (field == FieldArtistSort) return CMusicDatabase::artist_strSortName;
    else if (field == FieldArtistType) return CMusicDatabase::artist_strType;
    else if (field == FieldGender) return CMusicDatabase::artist_strGender;
    else if (field == FieldDisambiguation) return CMusicDatabase::artist_strDisambiguation;
    else if (field == FieldGenre) return CMusicDatabase::artist_strGenres;
    else if (field == FieldMoods) return CMusicDatabase::artist_strMoods;
    else if (field == FieldStyles) return CMusicDatabase::artist_strStyles;
    else if (field == FieldInstruments) return CMusicDatabase::artist_strInstruments;
    else if (field == FieldBiography) return CMusicDatabase::artist_strBiography;
    else if (field == FieldBorn) return CMusicDatabase::artist_strBorn;
    else if (field == FieldBandFormed) return CMusicDatabase::artist_strFormed;
    else if (field == FieldDisbanded) return CMusicDatabase::artist_strDisbanded;
    else if (field == FieldDied) return CMusicDatabase::artist_strDied;
    else if (field == FieldDateAdded) return CMusicDatabase::artist_dateAdded;
    else if (field == FieldDateNew) return CMusicDatabase::artist_dateNew;
    else if (field == FieldDateModified) return CMusicDatabase::artist_dateModified;
  }
  else if (mediaType == MediaTypeMusicVideo)
  {
    if (field == FieldId) return 0;
    else if (field == FieldTitle) index = VIDEODB_ID_MUSICVIDEO_TITLE;
    else if (field == FieldTime) index =  VIDEODB_ID_MUSICVIDEO_RUNTIME;
    else if (field == FieldDirector) index =  VIDEODB_ID_MUSICVIDEO_DIRECTOR;
    else if (field == FieldStudio) index =  VIDEODB_ID_MUSICVIDEO_STUDIOS;
    else if (field == FieldYear) return VIDEODB_DETAILS_MUSICVIDEO_PREMIERED;
    else if (field == FieldPlot) index =  VIDEODB_ID_MUSICVIDEO_PLOT;
    else if (field == FieldAlbum) index = VIDEODB_ID_MUSICVIDEO_ALBUM;
    else if (field == FieldArtist) index =  VIDEODB_ID_MUSICVIDEO_ARTIST;
    else if (field == FieldGenre) index =  VIDEODB_ID_MUSICVIDEO_GENRE;
    else if (field == FieldTrackNumber) index =  VIDEODB_ID_MUSICVIDEO_TRACK;
    else if (field == FieldFilename) return VIDEODB_DETAILS_MUSICVIDEO_FILE;
    else if (field == FieldPath) return VIDEODB_DETAILS_MUSICVIDEO_PATH;
    else if (field == FieldPlaycount) return VIDEODB_DETAILS_MUSICVIDEO_PLAYCOUNT;
    else if (field == FieldLastPlayed) return VIDEODB_DETAILS_MUSICVIDEO_LASTPLAYED;
    else if (field == FieldDateAdded) return VIDEODB_DETAILS_MUSICVIDEO_DATEADDED;
    else if (field == FieldUserRating) return VIDEODB_DETAILS_MUSICVIDEO_USER_RATING;

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
    if (field == FieldId) return 0;
    else if (field == FieldTitle) index = VIDEODB_ID_TITLE;
    else if (field == FieldSortTitle) index = VIDEODB_ID_SORTTITLE;
    else if (field == FieldOriginalTitle) index = VIDEODB_ID_ORIGINALTITLE;
    else if (field == FieldPlot) index = VIDEODB_ID_PLOT;
    else if (field == FieldPlotOutline) index = VIDEODB_ID_PLOTOUTLINE;
    else if (field == FieldTagline) index = VIDEODB_ID_TAGLINE;
    else if (field == FieldVotes) return VIDEODB_DETAILS_MOVIE_VOTES;
    else if (field == FieldRating) return VIDEODB_DETAILS_MOVIE_RATING;
    else if (field == FieldWriter) index = VIDEODB_ID_CREDITS;
    else if (field == FieldYear) return VIDEODB_DETAILS_MOVIE_PREMIERED;
    else if (field == FieldTime) index = VIDEODB_ID_RUNTIME;
    else if (field == FieldMPAA) index = VIDEODB_ID_MPAA;
    else if (field == FieldTop250) index = VIDEODB_ID_TOP250;
    else if (field == FieldSet) return VIDEODB_DETAILS_MOVIE_SET_NAME;
    else if (field == FieldGenre) index = VIDEODB_ID_GENRE;
    else if (field == FieldDirector) index = VIDEODB_ID_DIRECTOR;
    else if (field == FieldStudio) index = VIDEODB_ID_STUDIOS;
    else if (field == FieldTrailer) index = VIDEODB_ID_TRAILER;
    else if (field == FieldCountry) index = VIDEODB_ID_COUNTRY;
    else if (field == FieldFilename) index = VIDEODB_DETAILS_MOVIE_FILE;
    else if (field == FieldPath) return VIDEODB_DETAILS_MOVIE_PATH;
    else if (field == FieldPlaycount) return VIDEODB_DETAILS_MOVIE_PLAYCOUNT;
    else if (field == FieldLastPlayed) return VIDEODB_DETAILS_MOVIE_LASTPLAYED;
    else if (field == FieldDateAdded) return VIDEODB_DETAILS_MOVIE_DATEADDED;
    else if (field == FieldUserRating) return VIDEODB_DETAILS_MOVIE_USER_RATING;

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
    if (field == FieldId) return 0;
    else if (field == FieldTitle) index = VIDEODB_ID_TV_TITLE;
    else if (field == FieldSortTitle) index = VIDEODB_ID_TV_SORTTITLE;
    else if (field == FieldOriginalTitle) index = VIDEODB_ID_TV_ORIGINALTITLE;
    else if (field == FieldPlot) index = VIDEODB_ID_TV_PLOT;
    else if (field == FieldTvShowStatus) index = VIDEODB_ID_TV_STATUS;
    else if (field == FieldVotes) return VIDEODB_DETAILS_TVSHOW_VOTES;
    else if (field == FieldRating) return VIDEODB_DETAILS_TVSHOW_RATING;
    else if (field == FieldYear) index = VIDEODB_ID_TV_PREMIERED;
    else if (field == FieldGenre) index = VIDEODB_ID_TV_GENRE;
    else if (field == FieldMPAA) index = VIDEODB_ID_TV_MPAA;
    else if (field == FieldStudio) index = VIDEODB_ID_TV_STUDIOS;
    else if (field == FieldPath) return VIDEODB_DETAILS_TVSHOW_PATH;
    else if (field == FieldDateAdded) return VIDEODB_DETAILS_TVSHOW_DATEADDED;
    else if (field == FieldLastPlayed) return VIDEODB_DETAILS_TVSHOW_LASTPLAYED;
    else if (field == FieldNumberOfEpisodes) return VIDEODB_DETAILS_TVSHOW_NUM_EPISODES;
    else if (field == FieldNumberOfWatchedEpisodes) return VIDEODB_DETAILS_TVSHOW_NUM_WATCHED;
    else if (field == FieldSeason) return VIDEODB_DETAILS_TVSHOW_NUM_SEASONS;
    else if (field == FieldUserRating) return VIDEODB_DETAILS_TVSHOW_USER_RATING;

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
    if (field == FieldId) return 0;
    else if (field == FieldTitle) index = VIDEODB_ID_EPISODE_TITLE;
    else if (field == FieldPlot) index = VIDEODB_ID_EPISODE_PLOT;
    else if (field == FieldVotes) return VIDEODB_DETAILS_EPISODE_VOTES;
    else if (field == FieldRating) return VIDEODB_DETAILS_EPISODE_RATING;
    else if (field == FieldWriter) index = VIDEODB_ID_EPISODE_CREDITS;
    else if (field == FieldAirDate) index = VIDEODB_ID_EPISODE_AIRED;
    else if (field == FieldTime) index = VIDEODB_ID_EPISODE_RUNTIME;
    else if (field == FieldDirector) index = VIDEODB_ID_EPISODE_DIRECTOR;
    else if (field == FieldSeason) index = VIDEODB_ID_EPISODE_SEASON;
    else if (field == FieldEpisodeNumber) index = VIDEODB_ID_EPISODE_EPISODE;
    else if (field == FieldUniqueId) index = VIDEODB_ID_EPISODE_IDENT_ID;
    else if (field == FieldEpisodeNumberSpecialSort) index = VIDEODB_ID_EPISODE_SORTEPISODE;
    else if (field == FieldSeasonSpecialSort) index = VIDEODB_ID_EPISODE_SORTSEASON;
    else if (field == FieldFilename) return VIDEODB_DETAILS_EPISODE_FILE;
    else if (field == FieldPath) return VIDEODB_DETAILS_EPISODE_PATH;
    else if (field == FieldPlaycount) return VIDEODB_DETAILS_EPISODE_PLAYCOUNT;
    else if (field == FieldLastPlayed) return VIDEODB_DETAILS_EPISODE_LASTPLAYED;
    else if (field == FieldDateAdded) return VIDEODB_DETAILS_EPISODE_DATEADDED;
    else if (field == FieldTvShowTitle) return VIDEODB_DETAILS_EPISODE_TVSHOW_NAME;
    else if (field == FieldStudio) return VIDEODB_DETAILS_EPISODE_TVSHOW_STUDIO;
    else if (field == FieldYear) return VIDEODB_DETAILS_EPISODE_TVSHOW_AIRED;
    else if (field == FieldMPAA) return VIDEODB_DETAILS_EPISODE_TVSHOW_MPAA;
    else if (field == FieldUserRating) return VIDEODB_DETAILS_EPISODE_USER_RATING;

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

/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "media/MediaType.h"

#include <map>
#include <set>
#include <string>
#include <vector>

class CVariant;
enum class VideoDbContentType;

namespace dbiplus
{
  class Dataset;
  class field_value;
}

enum class Field
{
  // special fields used during sorting
  UNKNOWN = -1,
  NONE = 0,
  SORT, // used to store the string to use for sorting
  SORT_SPECIAL, // whether the item needs special handling (0 = no, 1 = sort on top, 2 = sort on bottom)
  LABEL,
  FOLDER,
  MEDIA_TYPE,
  ROW, // the row number in a dataset

  // special fields not retrieved from the database
  SIZE,
  DATE,
  DRIVE_TYPE,
  START_OFFSET,
  END_OFFSET,
  PROGRAM_COUNT,
  BITRATE,
  LISTENERS,
  PLAYLIST,
  VIRTUAL_FOLDER,
  RANDOM,
  DATE_TAKEN,
  AUDIO_COUNT,
  SUBTITLE_COUNT,

  INSTALL_DATE,
  LAST_UPDATED,
  LAST_USED,

  // fields retrievable from the database
  ID,
  GENRE,
  ALBUM,
  DISC_TITLE,
  IS_BOXSET,
  TOTAL_DISCS,
  ORIG_YEAR,
  ORIG_DATE,
  ARTIST,
  ARTIST_SORT,
  ALBUM_ARTIST,
  TITLE,
  SORT_TITLE,
  ORIGINAL_TITLE,
  YEAR,
  TIME,
  TRACK_NUMBER,
  FILENAME,
  PATH,
  PLAYCOUNT,
  LAST_PLAYED,
  IN_PROGRESS,
  RATING,
  COMMENT,
  ROLE,
  DATE_ADDED,
  DATE_MODIFIED,
  DATE_NEW,
  TVSHOW_TITLE,
  PLOT,
  PLOT_OUTLINE,
  TAGLINE,
  TVSHOW_STATUS,
  VOTES,
  DIRECTOR,
  ACTOR,
  STUDIO,
  COUNTRY,
  MPAA,
  TOP250,
  SET,
  NUMBER_OF_EPISODES,
  NUMBER_OF_WATCHED_EPISODES,
  WRITER,
  AIR_DATE,
  EPISODE_NUMBER,
  UNIQUE_ID,
  SEASON,
  EPISODE_NUMBER_SPECIAL_SORT,
  SEASON_SPECIAL_SORT,
  REVIEW,
  THEMES,
  MOODS,
  STYLES,
  ALBUM_TYPE,
  MUSIC_LABEL,
  COMPILATION,
  SOURCE,
  TRAILER,
  VIDEO_RESOLUTION,
  VIDEO_ASPECT_RATIO,
  VIDEO_CODEC,
  AUDIO_CHANNELS,
  AUDIO_CODEC,
  AUDIO_LANGUAGE,
  SUBTITLE_LANGUAGE,
  PRODUCTION_CODE,
  TAG,
  VIDEO_ASSET_TITLE,
  CHANNEL_NAME,
  CHANNEL_NUMBER,
  INSTRUMENTS,
  BIOGRAPHY,
  ARTIST_TYPE,
  GENDER,
  DISAMBIGUATION,
  BORN,
  BAND_FORMED,
  DISBANDED,
  DIED,
  STEREO_MODE,
  USER_RATING,
  RELEVANCE, // Used for actors' appearances
  CLIENT_CHANNEL_ORDER,
  BPM,
  MUSIC_BITRATE,
  SAMPLE_RATE,
  NUMBER_OF_CHANNELS,
  ALBUM_STATUS,
  ALBUM_DURATION,
  HDR_TYPE,
  HDR_DETAIL,
  PROVIDER,
  USER_PREFERENCE,
  HAS_VIDEO_VERSIONS,
  HAS_VIDEO_EXTRAS,
  MAX
};

using Fields = std::set<Field>;
using FieldList = std::vector<Field>;

enum class DatabaseQueryPart
{
  SELECT,
  WHERE,
  ORDER_BY,
};

using DatabaseResult = std::map<Field, CVariant>;
using DatabaseResults = std::vector<DatabaseResult>;

class DatabaseUtils
{
public:
  static MediaType MediaTypeFromVideoContentType(VideoDbContentType videoContentType);

  static std::string GetField(Field field, const MediaType &mediaType, DatabaseQueryPart queryPart);
  static int GetField(Field field, const MediaType &mediaType);
  static int GetFieldIndex(Field field, const MediaType &mediaType);
  static bool GetSelectFields(const Fields &fields, const MediaType &mediaType, FieldList &selectFields);

  static bool GetFieldValue(const dbiplus::field_value &fieldValue, CVariant &variantValue);
  static bool GetDatabaseResults(const MediaType& mediaType,
                                 const FieldList& fields,
                                 dbiplus::Dataset& dataset,
                                 DatabaseResults& results);

  static std::string BuildLimitClause(int end, int start = 0);
  static std::string BuildLimitClauseOnly(int end, int start = 0);
  static size_t GetLimitCount(int end, int start);

private:
  static int GetField(Field field, const MediaType &mediaType, bool asIndex);
};

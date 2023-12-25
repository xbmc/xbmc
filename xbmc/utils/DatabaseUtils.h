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
#include <memory>
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

typedef enum
{
  // special fields used during sorting
  FieldUnknown = -1,
  FieldNone = 0,
  FieldSort, // used to store the string to use for sorting
  FieldSortSpecial, // whether the item needs special handling (0 = no, 1 = sort on top, 2 = sort on bottom)
  FieldLabel,
  FieldFolder,
  FieldMediaType,
  FieldRow, // the row number in a dataset

  // special fields not retrieved from the database
  FieldSize,
  FieldDate,
  FieldDriveType,
  FieldStartOffset,
  FieldEndOffset,
  FieldProgramCount,
  FieldBitrate,
  FieldListeners,
  FieldPlaylist,
  FieldVirtualFolder,
  FieldRandom,
  FieldDateTaken,
  FieldAudioCount,
  FieldSubtitleCount,

  FieldInstallDate,
  FieldLastUpdated,
  FieldLastUsed,

  // fields retrievable from the database
  FieldId,
  FieldGenre,
  FieldAlbum,
  FieldDiscTitle,
  FieldIsBoxset,
  FieldTotalDiscs,
  FieldOrigYear,
  FieldOrigDate,
  FieldArtist,
  FieldArtistSort,
  FieldAlbumArtist,
  FieldTitle,
  FieldSortTitle,
  FieldOriginalTitle,
  FieldYear,
  FieldTime,
  FieldTrackNumber,
  FieldFilename,
  FieldPath,
  FieldPlaycount,
  FieldLastPlayed,
  FieldInProgress,
  FieldRating,
  FieldComment,
  FieldRole,
  FieldDateAdded,
  FieldDateModified,
  FieldDateNew,
  FieldTvShowTitle,
  FieldPlot,
  FieldPlotOutline,
  FieldTagline,
  FieldTvShowStatus,
  FieldVotes,
  FieldDirector,
  FieldActor,
  FieldStudio,
  FieldCountry,
  FieldMPAA,
  FieldTop250,
  FieldSet,
  FieldNumberOfEpisodes,
  FieldNumberOfWatchedEpisodes,
  FieldWriter,
  FieldAirDate,
  FieldEpisodeNumber,
  FieldUniqueId,
  FieldSeason,
  FieldEpisodeNumberSpecialSort,
  FieldSeasonSpecialSort,
  FieldReview,
  FieldThemes,
  FieldMoods,
  FieldStyles,
  FieldAlbumType,
  FieldMusicLabel,
  FieldCompilation,
  FieldSource,
  FieldTrailer,
  FieldVideoResolution,
  FieldVideoAspectRatio,
  FieldVideoCodec,
  FieldAudioChannels,
  FieldAudioCodec,
  FieldAudioLanguage,
  FieldSubtitleLanguage,
  FieldProductionCode,
  FieldTag,
  FieldVideoAssetTitle,
  FieldChannelName,
  FieldChannelNumber,
  FieldInstruments,
  FieldBiography,
  FieldArtistType,
  FieldGender,
  FieldDisambiguation,
  FieldBorn,
  FieldBandFormed,
  FieldDisbanded,
  FieldDied,
  FieldStereoMode,
  FieldUserRating,
  FieldRelevance, // Used for actors' appearances
  FieldClientChannelOrder,
  FieldBPM,
  FieldMusicBitRate,
  FieldSampleRate,
  FieldNoOfChannels,
  FieldAlbumStatus,
  FieldAlbumDuration,
  FieldHdrType,
  FieldProvider,
  FieldUserPreference,
  FieldMax
} Field;

typedef std::set<Field> Fields;
typedef std::vector<Field> FieldList;

typedef enum {
  DatabaseQueryPartSelect,
  DatabaseQueryPartWhere,
  DatabaseQueryPartOrderBy,
} DatabaseQueryPart;

typedef std::map<Field, CVariant> DatabaseResult;
typedef std::vector<DatabaseResult> DatabaseResults;

class DatabaseUtils
{
public:
  static MediaType MediaTypeFromVideoContentType(VideoDbContentType videoContentType);

  static std::string GetField(Field field, const MediaType &mediaType, DatabaseQueryPart queryPart);
  static int GetField(Field field, const MediaType &mediaType);
  static int GetFieldIndex(Field field, const MediaType &mediaType);
  static bool GetSelectFields(const Fields &fields, const MediaType &mediaType, FieldList &selectFields);

  static bool GetFieldValue(const dbiplus::field_value &fieldValue, CVariant &variantValue);
  static bool GetDatabaseResults(const MediaType &mediaType, const FieldList &fields, const std::unique_ptr<dbiplus::Dataset> &dataset, DatabaseResults &results);

  static std::string BuildLimitClause(int end, int start = 0);
  static std::string BuildLimitClauseOnly(int end, int start = 0);
  static size_t GetLimitCount(int end, int start);

private:
  static int GetField(Field field, const MediaType &mediaType, bool asIndex);
};

#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "media/MediaType.h"

class CVariant;

namespace dbiplus
{
  class Dataset;
  class field_value;
}

typedef enum {
  // special fields used during sorting
  FieldUnknown = -1,
  FieldNone = 0,
  FieldSort,        // used to store the string to use for sorting
  FieldSortSpecial, // whether the item needs special handling (0 = no, 1 = sort on top, 2 = sort on bottom)
  FieldLabel,
  FieldFolder,
  FieldMediaType,
  FieldRow,         // the row number in a dataset

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
  static MediaType MediaTypeFromVideoContentType(int videoContentType);

  static std::string GetField(Field field, const MediaType &mediaType, DatabaseQueryPart queryPart);
  static int GetField(Field field, const MediaType &mediaType);
  static int GetFieldIndex(Field field, const MediaType &mediaType);
  static bool GetSelectFields(const Fields &fields, const MediaType &mediaType, FieldList &selectFields);

  static bool GetFieldValue(const dbiplus::field_value &fieldValue, CVariant &variantValue);
  static bool GetDatabaseResults(const MediaType &mediaType, const FieldList &fields, const std::unique_ptr<dbiplus::Dataset> &dataset, DatabaseResults &results);

  static std::string BuildLimitClause(int end, int start = 0);

private:
  static int GetField(Field field, const MediaType &mediaType, bool asIndex);
};

/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartPlayList.h"

#include "ServiceBroker.h"
#include "Util.h"
#include "XBDateTime.h"
#include "dbwrappers/Database.h"
#include "filesystem/File.h"
#include "filesystem/SmartPlaylistDirectory.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/DatabaseUtils.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/StreamDetails.h"
#include "utils/StringUtils.h"
#include "utils/StringValidation.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <cstdlib>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

using enum CDatabaseQueryRule::FieldType;
using enum CDatabaseQueryRule::SearchOperator;
using namespace XFILE;

namespace KODI::PLAYLIST
{

typedef struct
{
  char string[17];
  Field field;
  CDatabaseQueryRule::FieldType type;
  StringValidation::Validator validator;
  bool browseable;
  int localizedString;
} translateField;

// clang-format off
static const translateField fields[] = {
  { "none",              FieldNone,                    TEXT_FIELD,     nullptr,                              false, 231 },
  { "filename",          FieldFilename,                TEXT_FIELD,     nullptr,                              false, 561 },
  { "path",              FieldPath,                    TEXT_FIELD,     nullptr,                              true,  573 },
  { "album",             FieldAlbum,                   TEXT_FIELD,     nullptr,                              true,  558 },
  { "albumartist",       FieldAlbumArtist,             TEXT_FIELD,     nullptr,                              true,  566 },
  { "artist",            FieldArtist,                  TEXT_FIELD,     nullptr,                              true,  557 },
  { "tracknumber",       FieldTrackNumber,             NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 554 },
  { "role",              FieldRole,                    TEXT_FIELD,     nullptr,                              true, 38033 },
  { "comment",           FieldComment,                 TEXT_FIELD,     nullptr,                              false, 569 },
  { "review",            FieldReview,                  TEXT_FIELD,     nullptr,                              false, 183 },
  { "themes",            FieldThemes,                  TEXT_FIELD,     nullptr,                              false, 21895 },
  { "moods",             FieldMoods,                   TEXT_FIELD,     nullptr,                              false, 175 },
  { "styles",            FieldStyles,                  TEXT_FIELD,     nullptr,                              false, 176 },
  { "type",              FieldAlbumType,               TEXT_FIELD,     nullptr,                              false, 564 },
  { "compilation",       FieldCompilation,             BOOLEAN_FIELD,  nullptr,                              false, 204 },
  { "label",             FieldMusicLabel,              TEXT_FIELD,     nullptr,                              false, 21899 },
  { "title",             FieldTitle,                   TEXT_FIELD,     nullptr,                              true,  556 },
  { "sorttitle",         FieldSortTitle,               TEXT_FIELD,     nullptr,                              false, 171 },
  { "originaltitle",     FieldOriginalTitle,           TEXT_FIELD,     nullptr,                              false, 20376 },
  { "year",              FieldYear,                    NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  true,  562 },
  { "time",              FieldTime,                    SECONDS_FIELD,  StringValidation::IsTime,             false, 180 },
  { "playcount",         FieldPlaycount,               NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 567 },
  { "lastplayed",        FieldLastPlayed,              DATE_FIELD,     CSmartPlaylistRule::ValidateDate,     false, 568 },
  { "inprogress",        FieldInProgress,              BOOLEAN_FIELD,  nullptr,                              false, 575 },
  { "rating",            FieldRating,                  REAL_FIELD,     CSmartPlaylistRule::ValidateRating,   false, 563 },
  { "userrating",        FieldUserRating,              REAL_FIELD,     CSmartPlaylistRule::ValidateMyRating, false, 38018 },
  { "votes",             FieldVotes,                   REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 205 },
  { "top250",            FieldTop250,                  NUMERIC_FIELD,  nullptr,                              false, 13409 },
  { "mpaarating",        FieldMPAA,                    TEXT_FIELD,     nullptr,                              false, 20074 },
  { "dateadded",         FieldDateAdded,               DATE_FIELD,     CSmartPlaylistRule::ValidateDate,     false, 570 },
  { "datemodified",      FieldDateModified,            DATE_FIELD,     CSmartPlaylistRule::ValidateDate,     false, 39119 },
  { "datenew",           FieldDateNew,                 DATE_FIELD,     CSmartPlaylistRule::ValidateDate,     false, 21877 },
  { "genre",             FieldGenre,                   TEXT_FIELD,     nullptr,                              true,  515 },
  { "plot",              FieldPlot,                    TEXT_FIELD,     nullptr,                              false, 207 },
  { "plotoutline",       FieldPlotOutline,             TEXT_FIELD,     nullptr,                              false, 203 },
  { "tagline",           FieldTagline,                 TEXT_FIELD,     nullptr,                              false, 202 },
  { "set",               FieldSet,                     TEXT_FIELD,     nullptr,                              true,  20457 },
  { "director",          FieldDirector,                TEXT_FIELD,     nullptr,                              true,  20339 },
  { "actor",             FieldActor,                   TEXT_FIELD,     nullptr,                              true,  20337 },
  { "writers",           FieldWriter,                  TEXT_FIELD,     nullptr,                              true,  20417 },
  { "airdate",           FieldAirDate,                 DATE_FIELD,     CSmartPlaylistRule::ValidateDate,     false, 20416 },
  { "hastrailer",        FieldTrailer,                 BOOLEAN_FIELD,  nullptr,                              false, 20423 },
  { "studio",            FieldStudio,                  TEXT_FIELD,     nullptr,                              true,  572 },
  { "country",           FieldCountry,                 TEXT_FIELD,     nullptr,                              true,  574 },
  { "tvshow",            FieldTvShowTitle,             TEXT_FIELD,     nullptr,                              true,  20364 },
  { "status",            FieldTvShowStatus,            TEXT_FIELD,     nullptr,                              false, 126 },
  { "season",            FieldSeason,                  NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 20373 },
  { "episode",           FieldEpisodeNumber,           NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 20359 },
  { "numepisodes",       FieldNumberOfEpisodes,        REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 20360 },
  { "numwatched",        FieldNumberOfWatchedEpisodes, REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 21457 },
  { "videoresolution",   FieldVideoResolution,         REAL_FIELD,     nullptr,                              false, 21443 },
  { "videocodec",        FieldVideoCodec,              TEXTIN_FIELD,   nullptr,                              false, 21445 },
  { "videoaspect",       FieldVideoAspectRatio,        REAL_FIELD,     nullptr,                              false, 21374 },
  { "audiochannels",     FieldAudioChannels,           REAL_FIELD,     nullptr,                              false, 21444 },
  { "audiocodec",        FieldAudioCodec,              TEXTIN_FIELD,   nullptr,                              false, 21446 },
  { "audiolanguage",     FieldAudioLanguage,           TEXTIN_FIELD,   nullptr,                              false, 21447 },
  { "audiocount",        FieldAudioCount,              REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 21481 },
  { "subtitlecount",     FieldSubtitleCount,           REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 21482 },
  { "subtitlelanguage",  FieldSubtitleLanguage,        TEXTIN_FIELD,   nullptr,                              false, 21448 },
  { "random",            FieldRandom,                  TEXT_FIELD,     nullptr,                              false, 590 },
  { "playlist",          FieldPlaylist,                PLAYLIST_FIELD, nullptr,                              true,  559 },
  { "virtualfolder",     FieldVirtualFolder,           PLAYLIST_FIELD, nullptr,                              true,  614 },
  { "tag",               FieldTag,                     TEXT_FIELD,     nullptr,                              true,  20459 },
  { "instruments",       FieldInstruments,             TEXT_FIELD,     nullptr,                              false, 21892 },
  { "biography",         FieldBiography,               TEXT_FIELD,     nullptr,                              false, 21887 },
  { "born",              FieldBorn,                    TEXT_FIELD,     nullptr,                              false, 21893 },
  { "bandformed",        FieldBandFormed,              TEXT_FIELD,     nullptr,                              false, 21894 },
  { "disbanded",         FieldDisbanded,               TEXT_FIELD,     nullptr,                              false, 21896 },
  { "died",              FieldDied,                    TEXT_FIELD,     nullptr,                              false, 21897 },
  { "artisttype",        FieldArtistType,              TEXT_FIELD,     nullptr,                              false, 564 },
  { "gender",            FieldGender,                  TEXT_FIELD,     nullptr,                              false, 39025 },
  { "disambiguation",    FieldDisambiguation,          TEXT_FIELD,     nullptr,                              false, 39026 },
  { "source",            FieldSource,                  TEXT_FIELD,     nullptr,                              true,  39030 },
  { "disctitle",         FieldDiscTitle,               TEXT_FIELD,     nullptr,                              false, 38076 },
  { "isboxset",          FieldIsBoxset,                BOOLEAN_FIELD,  nullptr,                              false, 38074 },
  { "totaldiscs",        FieldTotalDiscs,              NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 38077 },
  { "originalyear",      FieldOrigYear,                NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  true,  38078 },
  { "bpm",               FieldBPM,                     NUMERIC_FIELD,  nullptr,                              false, 38080 },
  { "samplerate",        FieldSampleRate,              NUMERIC_FIELD,  nullptr,                              false, 613 },
  { "bitrate",           FieldMusicBitRate,            NUMERIC_FIELD,  nullptr,                              false, 623 },
  { "channels",          FieldNoOfChannels,            NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 253 },
  { "albumstatus",       FieldAlbumStatus,             TEXT_FIELD,     nullptr,                              false, 38081 },
  { "albumduration",     FieldAlbumDuration,           SECONDS_FIELD,  StringValidation::IsTime,             false, 180 },
  { "hdrtype",           FieldHdrType,                 TEXTIN_FIELD,   nullptr,                              false, 20474 },
  { "hasversions",       FieldHasVideoVersions,        BOOLEAN_FIELD,  nullptr,                              false, 20475 },
  { "hasextras",         FieldHasVideoExtras,          BOOLEAN_FIELD,  nullptr,                              false, 20476 },
};
// clang-format on

typedef struct
{
  std::string name;
  Field field;
  bool canMix;
  int localizedString;
} group;

// clang-format off
static const group groups[] = { { "",               FieldUnknown,   false,    571 },
                                { "none",           FieldNone,      false,    231 },
                                { "sets",           FieldSet,       true,   20434 },
                                { "genres",         FieldGenre,     false,    135 },
                                { "years",          FieldYear,      false,    652 },
                                { "actors",         FieldActor,     false,    344 },
                                { "directors",      FieldDirector,  false,  20348 },
                                { "writers",        FieldWriter,    false,  20418 },
                                { "studios",        FieldStudio,    false,  20388 },
                                { "countries",      FieldCountry,   false,  20451 },
                                { "artists",        FieldArtist,    false,    133 },
                                { "albums",         FieldAlbum,     false,    132 },
                                { "tags",           FieldTag,       false,  20459 },
                                { "originalyears",  FieldOrigYear,  false,  38078 },
                              };
// clang-format on

#define RULE_VALUE_SEPARATOR  " / "

CSmartPlaylistRule::CSmartPlaylistRule() = default;

int CSmartPlaylistRule::TranslateField(const char *field) const
{
  for (const translateField& f : fields)
    if (StringUtils::EqualsNoCase(field, f.string)) return f.field;
  return FieldNone;
}

std::string CSmartPlaylistRule::TranslateField(int field) const
{
  for (const translateField& f : fields)
    if (field == f.field) return f.string;
  return "none";
}

SortBy CSmartPlaylistRule::TranslateOrder(const char *order)
{
  return SortUtils::SortMethodFromString(order);
}

std::string CSmartPlaylistRule::TranslateOrder(SortBy order)
{
  std::string sortOrder = SortUtils::SortMethodToString(order);
  if (sortOrder.empty())
    return "none";

  return sortOrder;
}

Field CSmartPlaylistRule::TranslateGroup(const char *group)
{
  for (const auto & i : groups)
  {
    if (StringUtils::EqualsNoCase(group, i.name))
      return i.field;
  }

  return FieldUnknown;
}

std::string CSmartPlaylistRule::TranslateGroup(Field group)
{
  for (const auto & i : groups)
  {
    if (group == i.field)
      return i.name;
  }

  return "";
}

std::string CSmartPlaylistRule::GetLocalizedField(int field)
{
  for (const translateField& f : fields)
    if (field == f.field)
      return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(f.localizedString);
  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(16018);
}

CDatabaseQueryRule::FieldType CSmartPlaylistRule::GetFieldType(int field) const
{
  for (const translateField& f : fields)
    if (field == f.field) return f.type;
  return TEXT_FIELD;
}

bool CSmartPlaylistRule::IsFieldBrowseable(int field)
{
  for (const translateField& f : fields)
    if (field == f.field) return f.browseable;

  return false;
}

bool CSmartPlaylistRule::Validate(const std::string &input, void *data)
{
  if (data == NULL)
    return true;

  CSmartPlaylistRule *rule = static_cast<CSmartPlaylistRule*>(data);

  // check if there's a validator for this rule
  StringValidation::Validator validator = NULL;
  for (const translateField& field : fields)
  {
    if (rule->m_field == field.field)
    {
        validator = field.validator;
        break;
    }
  }
  if (validator == NULL)
    return true;

  if (input.empty())
    return validator("", data);

  // Split the input into multiple values and validate every value separately
  const std::vector<std::string> values{StringUtils::Split(input, RULE_VALUE_SEPARATOR)};

  return (
      std::ranges::all_of(values, [data, validator](const auto& s) { return validator(s, data); }));
}

bool CSmartPlaylistRule::ValidateRating(const std::string &input, void *data)
{
  char *end = NULL;
  std::string strRating = input;
  StringUtils::Trim(strRating);

  double rating = std::strtod(strRating.c_str(), &end);
  return (end == NULL || *end == '\0') &&
         rating >= 0.0 && rating <= 10.0;
}

bool CSmartPlaylistRule::ValidateMyRating(const std::string &input, void *data)
{
  std::string strRating = input;
  StringUtils::Trim(strRating);

  int rating = atoi(strRating.c_str());
  return StringValidation::IsPositiveInteger(input, data) && rating <= 10;
}

bool CSmartPlaylistRule::ValidateDate(const std::string& input, void* data)
{
  if (!data)
    return false;

  const auto* rule = static_cast<CSmartPlaylistRule*>(data);

  //! @todo implement a validation for relative dates
  if (rule->m_operator == OPERATOR_IN_THE_LAST || rule->m_operator == OPERATOR_NOT_IN_THE_LAST)
    return true;

  // The date format must be YYYY-MM-DD
  CDateTime dt;
  return dt.SetFromRFC3339FullDate(input);
}

std::vector<Field> CSmartPlaylistRule::GetFields(const std::string &type)
{
  std::vector<Field> fields;
  bool isVideo = false;
  if (type == "mixed")
  {
    fields.push_back(FieldGenre);
    fields.push_back(FieldAlbum);
    fields.push_back(FieldArtist);
    fields.push_back(FieldAlbumArtist);
    fields.push_back(FieldTitle);
    fields.push_back(FieldOriginalTitle);
    fields.push_back(FieldYear);
    fields.push_back(FieldTime);
    fields.push_back(FieldTrackNumber);
    fields.push_back(FieldFilename);
    fields.push_back(FieldPath);
    fields.push_back(FieldPlaycount);
    fields.push_back(FieldLastPlayed);
  }
  else if (type == "songs")
  {
    fields.push_back(FieldGenre);
    fields.push_back(FieldSource);
    fields.push_back(FieldAlbum);
    fields.push_back(FieldDiscTitle);
    fields.push_back(FieldArtist);
    fields.push_back(FieldAlbumArtist);
    fields.push_back(FieldTitle);
    fields.push_back(FieldYear);
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
      CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
      fields.push_back(FieldOrigYear);
    fields.push_back(FieldTime);
    fields.push_back(FieldTrackNumber);
    fields.push_back(FieldFilename);
    fields.push_back(FieldPath);
    fields.push_back(FieldPlaycount);
    fields.push_back(FieldLastPlayed);
    fields.push_back(FieldRating);
    fields.push_back(FieldUserRating);
    fields.push_back(FieldComment);
    fields.push_back(FieldMoods);
    fields.push_back(FieldBPM);
    fields.push_back(FieldSampleRate);
    fields.push_back(FieldMusicBitRate);
    fields.push_back(FieldNoOfChannels);
    fields.push_back(FieldDateAdded);
    fields.push_back(FieldDateModified);
    fields.push_back(FieldDateNew);
  }
  else if (type == "albums")
  {
    fields.push_back(FieldGenre);
    fields.push_back(FieldSource);
    fields.push_back(FieldAlbum);
    fields.push_back(FieldDiscTitle);
    fields.push_back(FieldTotalDiscs);
    fields.push_back(FieldIsBoxset);
    fields.push_back(FieldArtist);        // any artist
    fields.push_back(FieldAlbumArtist);  // album artist
    fields.push_back(FieldYear);
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
      fields.push_back(FieldOrigYear);
    fields.push_back(FieldAlbumDuration);
    fields.push_back(FieldReview);
    fields.push_back(FieldThemes);
    fields.push_back(FieldMoods);
    fields.push_back(FieldStyles);
    fields.push_back(FieldCompilation);
    fields.push_back(FieldAlbumType);
    fields.push_back(FieldMusicLabel);
    fields.push_back(FieldRating);
    fields.push_back(FieldUserRating);
    fields.push_back(FieldPlaycount);
    fields.push_back(FieldLastPlayed);
    fields.push_back(FieldPath);
    fields.push_back(FieldAlbumStatus);
    fields.push_back(FieldDateAdded);
    fields.push_back(FieldDateModified);
    fields.push_back(FieldDateNew);
  }
  else if (type == "artists")
  {
    fields.push_back(FieldArtist);
    fields.push_back(FieldSource);
    fields.push_back(FieldGenre);
    fields.push_back(FieldMoods);
    fields.push_back(FieldStyles);
    fields.push_back(FieldInstruments);
    fields.push_back(FieldBiography);
    fields.push_back(FieldArtistType);
    fields.push_back(FieldGender);
    fields.push_back(FieldDisambiguation);
    fields.push_back(FieldBorn);
    fields.push_back(FieldBandFormed);
    fields.push_back(FieldDisbanded);
    fields.push_back(FieldDied);
    fields.push_back(FieldRole);
    fields.push_back(FieldPath);
    fields.push_back(FieldDateAdded);
    fields.push_back(FieldDateModified);
    fields.push_back(FieldDateNew);
  }
  else if (type == "tvshows")
  {
    fields.push_back(FieldTitle);
    fields.push_back(FieldOriginalTitle);
    fields.push_back(FieldPlot);
    fields.push_back(FieldTagline);
    fields.push_back(FieldTvShowStatus);
    fields.push_back(FieldVotes);
    fields.push_back(FieldRating);
    fields.push_back(FieldUserRating);
    fields.push_back(FieldYear);
    fields.push_back(FieldGenre);
    fields.push_back(FieldDirector);
    fields.push_back(FieldActor);
    fields.push_back(FieldNumberOfEpisodes);
    fields.push_back(FieldNumberOfWatchedEpisodes);
    fields.push_back(FieldPlaycount);
    fields.push_back(FieldPath);
    fields.push_back(FieldStudio);
    fields.push_back(FieldMPAA);
    fields.push_back(FieldDateAdded);
    fields.push_back(FieldLastPlayed);
    fields.push_back(FieldInProgress);
    fields.push_back(FieldTag);
    fields.push_back(FieldTrailer);
  }
  else if (type == "episodes")
  {
    fields.push_back(FieldTitle);
    fields.push_back(FieldTvShowTitle);
    fields.push_back(FieldOriginalTitle);
    fields.push_back(FieldPlot);
    fields.push_back(FieldVotes);
    fields.push_back(FieldRating);
    fields.push_back(FieldUserRating);
    fields.push_back(FieldTime);
    fields.push_back(FieldWriter);
    fields.push_back(FieldAirDate);
    fields.push_back(FieldPlaycount);
    fields.push_back(FieldLastPlayed);
    fields.push_back(FieldInProgress);
    fields.push_back(FieldGenre);
    fields.push_back(FieldYear); // premiered
    fields.push_back(FieldDirector);
    fields.push_back(FieldActor);
    fields.push_back(FieldEpisodeNumber);
    fields.push_back(FieldSeason);
    fields.push_back(FieldFilename);
    fields.push_back(FieldPath);
    fields.push_back(FieldStudio);
    fields.push_back(FieldMPAA);
    fields.push_back(FieldDateAdded);
    fields.push_back(FieldTag);
    isVideo = true;
  }
  else if (type == "movies")
  {
    fields.push_back(FieldTitle);
    fields.push_back(FieldOriginalTitle);
    fields.push_back(FieldPlot);
    fields.push_back(FieldPlotOutline);
    fields.push_back(FieldTagline);
    fields.push_back(FieldVotes);
    fields.push_back(FieldRating);
    fields.push_back(FieldUserRating);
    fields.push_back(FieldTime);
    fields.push_back(FieldWriter);
    fields.push_back(FieldPlaycount);
    fields.push_back(FieldLastPlayed);
    fields.push_back(FieldInProgress);
    fields.push_back(FieldGenre);
    fields.push_back(FieldCountry);
    fields.push_back(FieldYear); // premiered
    fields.push_back(FieldDirector);
    fields.push_back(FieldActor);
    fields.push_back(FieldMPAA);
    fields.push_back(FieldTop250);
    fields.push_back(FieldStudio);
    fields.push_back(FieldTrailer);
    fields.push_back(FieldFilename);
    fields.push_back(FieldPath);
    fields.push_back(FieldSet);
    fields.push_back(FieldTag);
    fields.push_back(FieldDateAdded);
    fields.push_back(FieldHasVideoVersions);
    fields.push_back(FieldHasVideoExtras);
    isVideo = true;
  }
  else if (type == "musicvideos")
  {
    fields.push_back(FieldTitle);
    fields.push_back(FieldGenre);
    fields.push_back(FieldAlbum);
    fields.push_back(FieldYear);
    fields.push_back(FieldArtist);
    fields.push_back(FieldFilename);
    fields.push_back(FieldPath);
    fields.push_back(FieldPlaycount);
    fields.push_back(FieldLastPlayed);
    fields.push_back(FieldRating);
    fields.push_back(FieldUserRating);
    fields.push_back(FieldTime);
    fields.push_back(FieldDirector);
    fields.push_back(FieldStudio);
    fields.push_back(FieldPlot);
    fields.push_back(FieldTag);
    fields.push_back(FieldDateAdded);
    isVideo = true;
  }
  if (isVideo)
  {
    fields.push_back(FieldVideoResolution);
    fields.push_back(FieldAudioChannels);
    fields.push_back(FieldAudioCount);
    fields.push_back(FieldSubtitleCount);
    fields.push_back(FieldVideoCodec);
    fields.push_back(FieldAudioCodec);
    fields.push_back(FieldAudioLanguage);
    fields.push_back(FieldSubtitleLanguage);
    fields.push_back(FieldVideoAspectRatio);
    fields.push_back(FieldHdrType);
  }
  fields.push_back(FieldPlaylist);
  fields.push_back(FieldVirtualFolder);

  return fields;
}

std::vector<SortBy> CSmartPlaylistRule::GetOrders(const std::string &type)
{
  std::vector<SortBy> orders;
  orders.push_back(SortByNone);
  if (type == "mixed")
  {
    orders.push_back(SortByGenre);
    orders.push_back(SortByAlbum);
    orders.push_back(SortByArtist);
    orders.push_back(SortByTitle);
    orders.push_back(SortByYear);
    orders.push_back(SortByTime);
    orders.push_back(SortByTrackNumber);
    orders.push_back(SortByFile);
    orders.push_back(SortByPath);
    orders.push_back(SortByPlaycount);
    orders.push_back(SortByLastPlayed);
  }
  else if (type == "songs")
  {
    orders.push_back(SortByGenre);
    orders.push_back(SortByAlbum);
    orders.push_back(SortByArtist);
    orders.push_back(SortByTitle);
    orders.push_back(SortByYear);
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
      CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
      orders.push_back(SortByOrigDate);
    orders.push_back(SortByTime);
    orders.push_back(SortByTrackNumber);
    orders.push_back(SortByFile);
    orders.push_back(SortByPath);
    orders.push_back(SortByPlaycount);
    orders.push_back(SortByLastPlayed);
    orders.push_back(SortByDateAdded);
    orders.push_back(SortByRating);
    orders.push_back(SortByUserRating);
    orders.push_back(SortByBPM);
  }
  else if (type == "albums")
  {
    orders.push_back(SortByGenre);
    orders.push_back(SortByAlbum);
    orders.push_back(SortByTotalDiscs);
    orders.push_back(SortByArtist);        // any artist
    orders.push_back(SortByYear);
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
      orders.push_back(SortByOrigDate);
    //orders.push_back(SortByThemes);
    //orders.push_back(SortByMoods);
    //orders.push_back(SortByStyles);
    orders.push_back(SortByAlbumType);
    //orders.push_back(SortByMusicLabel);
    orders.push_back(SortByRating);
    orders.push_back(SortByUserRating);
    orders.push_back(SortByPlaycount);
    orders.push_back(SortByLastPlayed);
    orders.push_back(SortByDateAdded);
  }
  else if (type == "artists")
  {
    orders.push_back(SortByArtist);
  }
  else if (type == "tvshows")
  {
    orders.push_back(SortBySortTitle);
    orders.push_back(SortByOriginalTitle);
    orders.push_back(SortByTvShowStatus);
    orders.push_back(SortByVotes);
    orders.push_back(SortByRating);
    orders.push_back(SortByUserRating);
    orders.push_back(SortByYear);
    orders.push_back(SortByGenre);
    orders.push_back(SortByNumberOfEpisodes);
    orders.push_back(SortByNumberOfWatchedEpisodes);
    //orders.push_back(SortByPlaycount);
    orders.push_back(SortByPath);
    orders.push_back(SortByStudio);
    orders.push_back(SortByMPAA);
    orders.push_back(SortByDateAdded);
    orders.push_back(SortByLastPlayed);
  }
  else if (type == "episodes")
  {
    orders.push_back(SortByTitle);
    orders.push_back(SortByOriginalTitle);
    orders.push_back(SortByTvShowTitle);
    orders.push_back(SortByVotes);
    orders.push_back(SortByRating);
    orders.push_back(SortByUserRating);
    orders.push_back(SortByTime);
    orders.push_back(SortByPlaycount);
    orders.push_back(SortByLastPlayed);
    orders.push_back(SortByYear); // premiered/dateaired
    orders.push_back(SortByEpisodeNumber);
    orders.push_back(SortBySeason);
    orders.push_back(SortByFile);
    orders.push_back(SortByPath);
    orders.push_back(SortByStudio);
    orders.push_back(SortByMPAA);
    orders.push_back(SortByDateAdded);
  }
  else if (type == "movies")
  {
    orders.push_back(SortBySortTitle);
    orders.push_back(SortByOriginalTitle);
    orders.push_back(SortByVotes);
    orders.push_back(SortByRating);
    orders.push_back(SortByUserRating);
    orders.push_back(SortByTime);
    orders.push_back(SortByPlaycount);
    orders.push_back(SortByLastPlayed);
    orders.push_back(SortByGenre);
    orders.push_back(SortByCountry);
    orders.push_back(SortByYear); // premiered
    orders.push_back(SortByMPAA);
    orders.push_back(SortByTop250);
    orders.push_back(SortByStudio);
    orders.push_back(SortByFile);
    orders.push_back(SortByPath);
    orders.push_back(SortByDateAdded);
  }
  else if (type == "musicvideos")
  {
    orders.push_back(SortByTitle);
    orders.push_back(SortByGenre);
    orders.push_back(SortByAlbum);
    orders.push_back(SortByYear);
    orders.push_back(SortByArtist);
    orders.push_back(SortByFile);
    orders.push_back(SortByPath);
    orders.push_back(SortByPlaycount);
    orders.push_back(SortByLastPlayed);
    orders.push_back(SortByTime);
    orders.push_back(SortByRating);
    orders.push_back(SortByUserRating);
    orders.push_back(SortByStudio);
    orders.push_back(SortByDateAdded);
  }
  orders.push_back(SortByRandom);

  return orders;
}

std::vector<Field> CSmartPlaylistRule::GetGroups(const std::string &type)
{
  std::vector<Field> groups;
  groups.push_back(FieldUnknown);

  if (type == "artists")
    groups.push_back(FieldGenre);
  else if (type == "albums")
  {
    groups.push_back(FieldYear);
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
      groups.push_back(FieldOrigYear);
  }
  if (type == "movies")
  {
    groups.push_back(FieldNone);
    groups.push_back(FieldSet);
    groups.push_back(FieldGenre);
    groups.push_back(FieldYear);
    groups.push_back(FieldActor);
    groups.push_back(FieldDirector);
    groups.push_back(FieldWriter);
    groups.push_back(FieldStudio);
    groups.push_back(FieldCountry);
    groups.push_back(FieldTag);
  }
  else if (type == "tvshows")
  {
    groups.push_back(FieldGenre);
    groups.push_back(FieldYear);
    groups.push_back(FieldActor);
    groups.push_back(FieldDirector);
    groups.push_back(FieldStudio);
    groups.push_back(FieldTag);
  }
  else if (type == "musicvideos")
  {
    groups.push_back(FieldArtist);
    groups.push_back(FieldAlbum);
    groups.push_back(FieldGenre);
    groups.push_back(FieldYear);
    groups.push_back(FieldDirector);
    groups.push_back(FieldStudio);
    groups.push_back(FieldTag);
  }

  return groups;
}

std::string CSmartPlaylistRule::GetLocalizedGroup(Field group)
{
  for (const auto & i : groups)
  {
    if (group == i.field)
      return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(i.localizedString);
  }

  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
      groups[0].localizedString);
}

bool CSmartPlaylistRule::CanGroupMix(Field group)
{
  for (const auto & i : groups)
  {
    if (group == i.field)
      return i.canMix;
  }

  return false;
}

std::string CSmartPlaylistRule::GetLocalizedRule() const
{
  return StringUtils::Format("{} {} {}", GetLocalizedField(m_field),
                             GetLocalizedOperator(m_operator), GetParameter());
}

std::string CSmartPlaylistRule::GetVideoResolutionQuery(const std::string &parameter) const
{
  std::string retVal(" IN (SELECT DISTINCT idFile FROM streamdetails WHERE iVideoWidth ");
  int iRes = (int)std::strtol(parameter.c_str(), NULL, 10);

  int min, max;
  if (iRes >= 2160)
  {
    min = 1921;
    max = INT_MAX;
  }
  else if (iRes >= 1080) { min = 1281; max = 1920; }
  else if (iRes >= 720) { min =  961; max = 1280; }
  else if (iRes >= 540) { min =  721; max =  960; }
  else                  { min =    0; max =  720; }

  switch (m_operator)
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

std::string CSmartPlaylistRule::GetBooleanQuery(const std::string &negate, const std::string &strType) const
{
  if (strType == "movies")
  {
    if (m_field == FieldInProgress)
      return "movie_view.idFile " + negate + " IN (SELECT DISTINCT idFile FROM bookmark WHERE type = 1)";
    else if (m_field == FieldTrailer)
      return negate + GetField(m_field, strType) + "!= ''";
    else if (m_field == FieldHasVideoVersions || m_field == FieldHasVideoExtras)
      return negate + GetField(m_field, strType);
  }
  else if (strType == "episodes")
  {
    if (m_field == FieldInProgress)
      return "episode_view.idFile " + negate + " IN (SELECT DISTINCT idFile FROM bookmark WHERE type = 1)";
  }
  else if (strType == "tvshows")
  {
    if (m_field == FieldInProgress)
      return negate + " ("
                          "(tvshow_view.watchedcount > 0 AND tvshow_view.watchedcount < tvshow_view.totalCount) OR "
                          "(tvshow_view.watchedcount = 0 AND EXISTS "
                            "(SELECT 1 FROM episode_view WHERE episode_view.idShow = " + GetField(FieldId, strType) + " AND episode_view.resumeTimeInSeconds > 0)"
                          ")"
                       ")";
    else if (m_field == FieldTrailer)
      return negate + GetField(m_field, strType) + "!= ''";
  }
  if (strType == "albums")
  {
    if (m_field == FieldCompilation)
      return negate + GetField(m_field, strType);
    if (m_field == FieldIsBoxset)
      return negate + "albumview.bBoxedSet = 1";
  }
  return "";
}

CDatabaseQueryRule::SearchOperator CSmartPlaylistRule::GetOperator(const std::string& strType) const
{
  SearchOperator op = CDatabaseQueryRule::GetOperator(strType);
  if ((strType == "tvshows" || strType == "episodes") && m_field == FieldYear)
  { // special case for premiered which is a date rather than a year
    //! @todo SMARTPLAYLISTS do we really need this, or should we just make this field the premiered date and request a date?
    if (op == OPERATOR_EQUALS)
      op = OPERATOR_CONTAINS;
    else if (op == OPERATOR_DOES_NOT_EQUAL)
      op = OPERATOR_DOES_NOT_CONTAIN;
  }
  return op;
}

std::string CSmartPlaylistRule::FormatParameter(const std::string &operatorString, const std::string &param, const CDatabase &db, const std::string &strType) const
{
  // special-casing
  if (m_field == FieldTime || m_field == FieldAlbumDuration)
  { // translate time to seconds
    std::string seconds = std::to_string(StringUtils::TimeStringToSeconds(param));
    return db.PrepareSQL(operatorString, seconds.c_str());
  }
  return CDatabaseQueryRule::FormatParameter(operatorString, param, db, strType);
}

std::string CSmartPlaylistRule::FormatLinkQuery(const char *field, const char *table, const MediaType& mediaType, const std::string& mediaField, const std::string& parameter)
{
  // NOTE: no need for a PrepareSQL here, as the parameter has already been formatted
  return StringUtils::Format(
      " EXISTS (SELECT 1 FROM {}_link"
      "         JOIN {} ON {}.{}_id={}_link.{}_id"
      "         WHERE {}_link.media_id={} AND {}.name {} AND {}_link.media_type = '{}')",
      field, table, table, table, field, table, field, mediaField, table, parameter, field,
      mediaType);
}

std::string CSmartPlaylistRule::FormatYearQuery(const std::string& field,
                                                const std::string& param,
                                                const std::string& parameter) const
{
  std::string query;
  if (m_operator == OPERATOR_EQUALS && param == "0")
    query = "(TRIM(" + field + ") = '' OR " + field + " IS NULL)";
  else if (m_operator == OPERATOR_DOES_NOT_EQUAL && param == "0")
    query = "(TRIM(" + field + ") <> '' AND " + field + " IS NOT NULL)";
  else
  { // Get year from ISO8601 date string, cast as INTEGER
    query = "CAST(" + field + " as INTEGER)" + parameter;
    if (m_operator == OPERATOR_LESS_THAN)
      query = "(TRIM(" + field + ") = '' OR " + field + " IS NULL OR " + query + ")";
  }
  return query;
}

namespace
{
std::string FormatNullableDate(const std::string& field,
                               CDatabaseQueryRule::SearchOperator oper,
                               const std::string& parameter)
{
  if (oper == OPERATOR_LESS_THAN || oper == OPERATOR_BEFORE || oper == OPERATOR_NOT_IN_THE_LAST)
    return field + " IS NULL OR " + field + parameter;
  else
    return field + " IS NOT NULL AND " + field + parameter;
}

std::string FormatNullableNumber(const std::string& field,
                                 CDatabaseQueryRule::SearchOperator oper,
                                 std::string_view param,
                                 const std::string& parameter)
{
  if ((oper == OPERATOR_EQUALS && param == "0") ||
      (oper == OPERATOR_DOES_NOT_EQUAL && param != "0") || (oper == OPERATOR_LESS_THAN))
    return field + " IS NULL OR " + field + parameter;
  else
    return field + " IS NOT NULL AND " + field + parameter;
}
} // namespace

std::string CSmartPlaylistRule::FormatWhereClause(const std::string &negate, const std::string &oper, const std::string &param,
                                                 const CDatabase &db, const std::string &strType) const
{
  std::string parameter = FormatParameter(oper, param, db, strType);

  std::string query;
  std::string table;
  if (strType == "songs")
  {
    table = "songview";

    if (m_field == FieldGenre)
      query = negate + " EXISTS (SELECT 1 FROM song_genre, genre WHERE song_genre.idSong = " + GetField(FieldId, strType) + " AND song_genre.idGenre = genre.idGenre AND genre.strGenre" + parameter + ")";
    else if (m_field == FieldArtist)
      query = negate + " EXISTS (SELECT 1 FROM song_artist, artist WHERE song_artist.idSong = " + GetField(FieldId, strType) + " AND song_artist.idArtist = artist.idArtist AND artist.strArtist" + parameter + ")";
    else if (m_field == FieldAlbumArtist)
      query = negate + " EXISTS (SELECT 1 FROM album_artist, artist WHERE album_artist.idAlbum = " + table + ".idAlbum AND album_artist.idArtist = artist.idArtist AND artist.strArtist" + parameter + ")";
    else if (m_field == FieldLastPlayed)
      query = FormatNullableDate(GetField(m_field, strType), m_operator, parameter);
    else if (m_field == FieldSource)
      query = negate + " EXISTS (SELECT 1 FROM album_source, source WHERE album_source.idAlbum = " + table + ".idAlbum AND album_source.idSource = source.idSource AND source.strName" + parameter + ")";
    else if (m_field == FieldYear || m_field == FieldOrigYear)
    {
      std::string field;
      if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
        field = GetField(FieldOrigYear, strType);
      else
        field = GetField(m_field, strType);
      query = FormatYearQuery(field, param, parameter);
    }
  }
  else if (strType == "albums")
  {
    table = "albumview";

    if (m_field == FieldGenre)
      query = negate + " EXISTS (SELECT 1 FROM song, song_genre, genre WHERE song.idAlbum = " + GetField(FieldId, strType) + " AND song.idSong = song_genre.idSong AND song_genre.idGenre = genre.idGenre AND genre.strGenre" + parameter + ")";
    else if (m_field == FieldArtist)
      query = negate + " EXISTS (SELECT 1 FROM song, song_artist, artist WHERE song.idAlbum = " + GetField(FieldId, strType) + " AND song.idSong = song_artist.idSong AND song_artist.idArtist = artist.idArtist AND artist.strArtist" + parameter + ")";
    else if (m_field == FieldAlbumArtist)
      query = negate + " EXISTS (SELECT 1 FROM album_artist, artist WHERE album_artist.idAlbum = " + GetField(FieldId, strType) + " AND album_artist.idArtist = artist.idArtist AND artist.strArtist" + parameter + ")";
    else if (m_field == FieldPath)
      query = negate + " EXISTS (SELECT 1 FROM song JOIN path on song.idpath = path.idpath WHERE song.idAlbum = " + GetField(FieldId, strType) + " AND path.strPath" + parameter + ")";
    else if (m_field == FieldLastPlayed)
      query = FormatNullableDate(GetField(m_field, strType), m_operator, parameter);
    else if (m_field == FieldSource)
      query = negate + " EXISTS (SELECT 1 FROM album_source, source WHERE album_source.idAlbum = " + GetField(FieldId, strType) + " AND album_source.idSource = source.idSource AND source.strName" + parameter + ")";
    else if (m_field == FieldDiscTitle)
      query = negate +
              " EXISTS (SELECT 1 FROM song WHERE song.idAlbum = " + GetField(FieldId, strType) +
              " AND song.strDiscSubtitle" + parameter + ")";
    else if (m_field == FieldYear || m_field == FieldOrigYear)
    {
      std::string field;
      if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
              CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
        field = GetField(FieldOrigYear, strType);
      else
        field = GetField(m_field, strType);
      query = FormatYearQuery(field, param, parameter);
    }
  }
  else if (strType == "artists")
  {
    table = "artistview";

    if (m_field == FieldGenre)
    {
      query = negate + " (EXISTS (SELECT DISTINCT song_artist.idArtist FROM song_artist, song_genre, genre WHERE song_artist.idArtist = " + GetField(FieldId, strType) + " AND song_artist.idSong = song_genre.idSong AND song_genre.idGenre = genre.idGenre AND genre.strGenre" + parameter + ")";
      query += " OR ";
      query += "EXISTS (SELECT DISTINCT album_artist.idArtist FROM album_artist, song, song_genre, genre WHERE album_artist.idArtist = " + GetField(FieldId, strType) + " AND song.idAlbum = album_artist.idAlbum AND song.idSong = song_genre.idSong AND song_genre.idGenre = genre.idGenre AND genre.strGenre" + parameter + "))";
    }
    else if (m_field == FieldRole)
    {
      query = negate + " (EXISTS (SELECT DISTINCT song_artist.idArtist FROM song_artist, role WHERE song_artist.idArtist = " + GetField(FieldId, strType) + " AND song_artist.idRole = role.idRole AND role.strRole" + parameter + "))";
    }
    else if (m_field == FieldPath)
    {
      query = negate + " (EXISTS (SELECT DISTINCT song_artist.idArtist FROM song_artist JOIN song ON song.idSong = song_artist.idSong JOIN path ON song.idpath = path.idpath ";
      query += "WHERE song_artist.idArtist = " + GetField(FieldId, strType) + " AND path.strPath" + parameter + "))";
    }
    else if (m_field == FieldSource)
    {
      query = negate + " (EXISTS(SELECT 1 FROM song_artist, song, album_source, source WHERE song_artist.idArtist = " + GetField(FieldId, strType) + " AND song.idSong = song_artist.idSong AND song_artist.idRole = 1 AND album_source.idAlbum = song.idAlbum AND album_source.idSource = source.idSource AND source.strName" + parameter + ")";
      query += " OR ";
      query += " EXISTS (SELECT 1 FROM album_artist, album_source, source WHERE album_artist.idArtist = " + GetField(FieldId, strType) + " AND album_source.idAlbum = album_artist.idAlbum AND album_source.idSource = source.idSource AND source.strName" + parameter + "))";
    }
  }
  else if (strType == "movies")
  {
    table = "movie_view";

    if (m_field == FieldGenre)
      query = negate + FormatLinkQuery("genre", "genre", MediaTypeMovie, GetField(FieldId, strType), parameter);
    else if (m_field == FieldDirector)
      query = negate + FormatLinkQuery("director", "actor", MediaTypeMovie, GetField(FieldId, strType), parameter);
    else if (m_field == FieldActor)
      query = negate + FormatLinkQuery("actor", "actor", MediaTypeMovie, GetField(FieldId, strType), parameter);
    else if (m_field == FieldWriter)
      query = negate + FormatLinkQuery("writer", "actor", MediaTypeMovie, GetField(FieldId, strType), parameter);
    else if (m_field == FieldStudio)
      query = negate + FormatLinkQuery("studio", "studio", MediaTypeMovie, GetField(FieldId, strType), parameter);
    else if (m_field == FieldCountry)
      query = negate + FormatLinkQuery("country", "country", MediaTypeMovie, GetField(FieldId, strType), parameter);
    else if (m_field == FieldLastPlayed || m_field == FieldDateAdded)
      query = FormatNullableDate(GetField(m_field, strType), m_operator, parameter);
    else if (m_field == FieldTag)
      query = negate + FormatLinkQuery("tag", "tag", MediaTypeMovie, GetField(FieldId, strType), parameter);
  }
  else if (strType == "musicvideos")
  {
    table = "musicvideo_view";

    if (m_field == FieldGenre)
      query = negate + FormatLinkQuery("genre", "genre", MediaTypeMusicVideo, GetField(FieldId, strType), parameter);
    else if (m_field == FieldArtist || m_field == FieldAlbumArtist)
      query = negate + FormatLinkQuery("actor", "actor", MediaTypeMusicVideo, GetField(FieldId, strType), parameter);
    else if (m_field == FieldStudio)
      query = negate + FormatLinkQuery("studio", "studio", MediaTypeMusicVideo, GetField(FieldId, strType), parameter);
    else if (m_field == FieldDirector)
      query = negate + FormatLinkQuery("director", "actor", MediaTypeMusicVideo, GetField(FieldId, strType), parameter);
    else if (m_field == FieldLastPlayed || m_field == FieldDateAdded)
      query = FormatNullableDate(GetField(m_field, strType), m_operator, parameter);
    else if (m_field == FieldTag)
      query = negate + FormatLinkQuery("tag", "tag", MediaTypeMusicVideo, GetField(FieldId, strType), parameter);
  }
  else if (strType == "tvshows")
  {
    table = "tvshow_view";

    if (m_field == FieldGenre)
      query = negate + FormatLinkQuery("genre", "genre", MediaTypeTvShow, GetField(FieldId, strType), parameter);
    else if (m_field == FieldDirector)
      query = negate + FormatLinkQuery("director", "actor", MediaTypeTvShow, GetField(FieldId, strType), parameter);
    else if (m_field == FieldActor)
      query = negate + FormatLinkQuery("actor", "actor", MediaTypeTvShow, GetField(FieldId, strType), parameter);
    else if (m_field == FieldStudio)
      query = negate + FormatLinkQuery("studio", "studio", MediaTypeTvShow, GetField(FieldId, strType), parameter);
    else if (m_field == FieldMPAA)
      query = negate + " (" + GetField(m_field, strType) + parameter + ")";
    else if (m_field == FieldLastPlayed || m_field == FieldDateAdded)
      query = FormatNullableDate(GetField(m_field, strType), m_operator, parameter);
    else if (m_field == FieldPlaycount)
      query = "CASE WHEN COALESCE(" + GetField(FieldNumberOfEpisodes, strType) + " - " + GetField(FieldNumberOfWatchedEpisodes, strType) + ", 0) > 0 THEN 0 ELSE 1 END " + parameter;
    else if (m_field == FieldTag)
      query = negate + FormatLinkQuery("tag", "tag", MediaTypeTvShow, GetField(FieldId, strType), parameter);
  }
  else if (strType == "episodes")
  {
    table = "episode_view";

    if (m_field == FieldGenre)
      query = negate + FormatLinkQuery("genre", "genre", MediaTypeTvShow, (table + ".idShow").c_str(), parameter);
    else if (m_field == FieldTag)
      query = negate + FormatLinkQuery("tag", "tag", MediaTypeTvShow, (table + ".idShow").c_str(), parameter);
    else if (m_field == FieldDirector)
      query = negate + FormatLinkQuery("director", "actor", MediaTypeEpisode, GetField(FieldId, strType), parameter);
    else if (m_field == FieldActor)
      query = negate + FormatLinkQuery("actor", "actor", MediaTypeEpisode, GetField(FieldId, strType), parameter);
    else if (m_field == FieldWriter)
      query = negate + FormatLinkQuery("writer", "actor", MediaTypeEpisode, GetField(FieldId, strType), parameter);
    else if (m_field == FieldLastPlayed || m_field == FieldDateAdded)
      query = FormatNullableDate(GetField(m_field, strType), m_operator, parameter);
    else if (m_field == FieldStudio)
      query = negate + FormatLinkQuery("studio", "studio", MediaTypeTvShow, (table + ".idShow").c_str(), parameter);
    else if (m_field == FieldMPAA)
      query = negate + " (" + GetField(m_field, strType) +  parameter + ")";
  }
  if (m_field == FieldVideoResolution)
    query = table + ".idFile" + negate + GetVideoResolutionQuery(param);
  else if (m_field == FieldAudioChannels)
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND iAudioChannels " + parameter + ")";
  else if (m_field == FieldVideoCodec)
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strVideoCodec " + parameter + ")";
  else if (m_field == FieldAudioCodec)
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strAudioCodec " + parameter + ")";
  else if (m_field == FieldAudioLanguage)
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strAudioLanguage " + parameter + ")";
  else if (m_field == FieldSubtitleLanguage)
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strSubtitleLanguage " + parameter + ")";
  else if (m_field == FieldVideoAspectRatio)
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND fVideoAspect " + parameter + ")";
  else if (m_field == FieldAudioCount)
    query = db.PrepareSQL(negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND streamdetails.iStreamtype = %i GROUP BY streamdetails.idFile HAVING COUNT(streamdetails.iStreamType) " + parameter + ")",CStreamDetail::AUDIO);
  else if (m_field == FieldSubtitleCount)
    query = db.PrepareSQL(negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND streamdetails.iStreamType = %i GROUP BY streamdetails.idFile HAVING COUNT(streamdetails.iStreamType) " + parameter + ")",CStreamDetail::SUBTITLE);
  else if (m_field == FieldHdrType)
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strHdrType " + parameter + ")";

  if ((m_field == FieldPlaycount && strType != "songs" && strType != "albums" &&
       strType != "tvshows") ||
      m_field == FieldUserRating)
    query = FormatNullableNumber(GetField(m_field, strType), m_operator, param, parameter);

  if (query.empty())
    query = CDatabaseQueryRule::FormatWhereClause(negate, oper, param, db, strType);

  return query;
}

std::string CSmartPlaylistRule::GetField(int field, const std::string &type) const
{
  if (field >= FieldUnknown && field < FieldMax)
    return DatabaseUtils::GetField(static_cast<Field>(field), CMediaTypes::FromString(type),
                                   DatabaseQueryPart::WHERE);
  return "";
}

std::string CSmartPlaylistRuleCombination::GetWhereClause(
    const CDatabase& db,
    const std::string& strType,
    std::set<std::string, std::less<>>& referencedPlaylists) const
{
  std::string rule;

  // translate the combinations into SQL
  const CDatabaseQueryRuleCombinations& combinations = GetCombinations();
  for (auto it = combinations.cbegin(); it != combinations.cend(); ++it)
  {
    if (it != combinations.cbegin())
      rule += GetType() == CDatabaseQueryRuleCombination::Type::COMBINATION_AND ? " AND " : " OR ";
    std::shared_ptr<CSmartPlaylistRuleCombination> combo = std::static_pointer_cast<CSmartPlaylistRuleCombination>(*it);
    if (combo)
      rule += "(" + combo->GetWhereClause(db, strType, referencedPlaylists) + ")";
  }

  // translate the rules into SQL
  for (const auto& r : GetRules())
  {
    // don't include playlists that are meant to be displayed
    // as a virtual folders in the SQL WHERE clause
    if (r->m_field == FieldVirtualFolder)
      continue;

    if (!rule.empty())
      rule += GetType() == CDatabaseQueryRuleCombination::Type::COMBINATION_AND ? " AND " : " OR ";
    rule += "(";
    std::string currentRule;
    if (r->m_field == FieldPlaylist)
    {
      const std::string playlistFile =
          CSmartPlaylistDirectory::GetPlaylistByName(r->m_parameter.at(0), strType);
      if (!playlistFile.empty() && !referencedPlaylists.contains(playlistFile))
      {
        referencedPlaylists.insert(playlistFile);
        CSmartPlaylist playlist;
        if (playlist.Load(playlistFile))
        {
          std::string playlistQuery;
          // only playlists of same type will be part of the query
          if (playlist.GetType() == strType || (playlist.GetType() == "mixed" && (strType == "songs" || strType == "musicvideos")) || playlist.GetType().empty())
          {
            playlist.SetType(strType);
            playlistQuery = playlist.GetWhereClause(db, referencedPlaylists);
          }
          if (playlist.GetType() == strType)
          {
            if (r->m_operator == OPERATOR_DOES_NOT_EQUAL)
              currentRule = StringUtils::Format("NOT ({})", playlistQuery);
            else
              currentRule = playlistQuery;
          }
        }
      }
    }
    else
      currentRule = r->GetWhereClause(db, strType);
    // if we don't get a rule, we add '1' or '0' so the query is still valid and doesn't fail
    if (currentRule.empty())
      currentRule =
          GetType() == CDatabaseQueryRuleCombination::Type::COMBINATION_AND ? "'1'" : "'0'";
    rule += currentRule;
    rule += ")";
  }

  return rule;
}

void CSmartPlaylistRuleCombination::GetVirtualFolders(const std::string& strType, std::vector<std::string> &virtualFolders) const
{
  for (const auto& combination : GetCombinations())
  {
    const auto combo = std::static_pointer_cast<CSmartPlaylistRuleCombination>(combination);
    if (combo)
      combo->GetVirtualFolders(strType, virtualFolders);
  }

  for (const auto& r : GetRules())
  {
    if ((r->m_field != FieldVirtualFolder && r->m_field != FieldPlaylist) ||
        r->m_operator != OPERATOR_EQUALS)
      continue;

    const std::string playlistFile =
        CSmartPlaylistDirectory::GetPlaylistByName(r->m_parameter.at(0), strType);
    if (playlistFile.empty())
      continue;

    if (r->m_field == FieldVirtualFolder)
      virtualFolders.push_back(playlistFile);
    else
    {
      // look for any virtual folders in the expanded playlists
      CSmartPlaylist playlist;
      if (!playlist.Load(playlistFile))
        continue;

      if (CSmartPlaylist::CheckTypeCompatibility(playlist.GetType(), strType))
        playlist.GetVirtualFolders(virtualFolders);
    }
  }
}

CSmartPlaylist::CSmartPlaylist()
{
  Reset();
}

bool CSmartPlaylist::OpenAndReadName(const CURL &url)
{
  if (readNameFromPath(url) == NULL)
    return false;

  return !m_playlistName.empty();
}

const TiXmlNode* CSmartPlaylist::readName(const TiXmlNode *root)
{
  if (root == NULL)
    return NULL;

  const TiXmlElement *rootElem = root->ToElement();
  if (rootElem == NULL)
    return NULL;

  if (!StringUtils::EqualsNoCase(root->Value(), "smartplaylist"))
  {
    CLog::Log(LOGERROR, "Error loading Smart playlist");
    return NULL;
  }

  // load the playlist type
  const char* type = rootElem->Attribute("type");
  if (type)
    m_playlistType = type;
  // backward compatibility:
  if (m_playlistType == "music")
    m_playlistType = "songs";
  if (m_playlistType == "video")
    m_playlistType = "musicvideos";

  // load the playlist name
  XMLUtils::GetString(root, "name", m_playlistName);

  return root;
}

const TiXmlNode* CSmartPlaylist::readNameFromPath(const CURL &url)
{
  CFileStream file;
  if (!file.Open(url))
  {
    CLog::Log(LOGERROR, "Error loading Smart playlist {} (failed to read file)", url.GetRedacted());
    return NULL;
  }

  m_xmlDoc.Clear();
  file >> m_xmlDoc;

  const TiXmlNode *root = readName(m_xmlDoc.RootElement());
  if (m_playlistName.empty())
  {
    m_playlistName = CUtil::GetTitleFromPath(url.Get());
    if (URIUtils::HasExtension(m_playlistName, ".xsp"))
      URIUtils::RemoveExtension(m_playlistName);
  }

  return root;
}

const TiXmlNode* CSmartPlaylist::readNameFromXml(const std::string &xml)
{
  if (xml.empty())
  {
    CLog::Log(LOGERROR, "Error loading empty Smart playlist");
    return NULL;
  }

  m_xmlDoc.Clear();
  if (!m_xmlDoc.Parse(xml))
  {
    CLog::Log(LOGERROR, "Error loading Smart playlist (failed to parse xml: {})",
              m_xmlDoc.ErrorDesc());
    return NULL;
  }

  const TiXmlNode *root = readName(m_xmlDoc.RootElement());

  return root;
}

bool CSmartPlaylist::load(const TiXmlNode *root)
{
  if (root == NULL)
    return false;

  return LoadFromXML(root);
}

bool CSmartPlaylist::Load(const CURL &url)
{
  return load(readNameFromPath(url));
}

bool CSmartPlaylist::Load(const std::string &path)
{
  const CURL pathToUrl(path);
  return load(readNameFromPath(pathToUrl));
}

bool CSmartPlaylist::Load(const CVariant &obj)
{
  if (!obj.isObject())
    return false;

  // load the playlist type
  if (obj.isMember("type") && obj["type"].isString())
    m_playlistType = obj["type"].asString();

  // backward compatibility
  if (m_playlistType == "music")
    m_playlistType = "songs";
  if (m_playlistType == "video")
    m_playlistType = "musicvideos";

  // load the playlist name
  if (obj.isMember("name") && obj["name"].isString())
    m_playlistName = obj["name"].asString();

  if (obj.isMember("rules"))
    m_ruleCombination.Load(obj["rules"], this);

  if (obj.isMember("group") && obj["group"].isMember("type") && obj["group"]["type"].isString())
  {
    m_group = obj["group"]["type"].asString();
    if (obj["group"].isMember("mixed") && obj["group"]["mixed"].isBoolean())
      m_groupMixed = obj["group"]["mixed"].asBoolean();
  }

  // now any limits
  if (obj.isMember("limit") && (obj["limit"].isInteger() || obj["limit"].isUnsignedInteger()) && obj["limit"].asUnsignedInteger() > 0)
    m_limit = (unsigned int)obj["limit"].asUnsignedInteger();

  // and order
  if (obj.isMember("order") && obj["order"].isMember("method") && obj["order"]["method"].isString())
  {
    const CVariant &order = obj["order"];
    if (order.isMember("direction") && order["direction"].isString())
      m_orderDirection = StringUtils::EqualsNoCase(order["direction"].asString(), "ascending")
                             ? SortOrder::ASCENDING
                             : SortOrder::DESCENDING;

    if (order.isMember("ignorefolders") && obj["ignorefolders"].isBoolean())
      m_orderAttributes = obj["ignorefolders"].asBoolean() ? SortAttributeIgnoreFolders : SortAttributeNone;

    m_orderField = CSmartPlaylistRule::TranslateOrder(obj["order"]["method"].asString().c_str());
  }

  return true;
}

bool CSmartPlaylist::LoadFromXml(const std::string &xml)
{
  return load(readNameFromXml(xml));
}

bool CSmartPlaylist::LoadFromXML(const TiXmlNode *root, const std::string &encoding)
{
  if (!root)
    return false;

  std::string tmp;
  if (XMLUtils::GetString(root, "match", tmp))
    m_ruleCombination.SetType(StringUtils::EqualsNoCase(tmp, "all")
                                  ? CDatabaseQueryRuleCombination::Type::COMBINATION_AND
                                  : CDatabaseQueryRuleCombination::Type::COMBINATION_OR);

  // now the rules
  const TiXmlNode *ruleNode = root->FirstChild("rule");
  while (ruleNode)
  {
    const auto rule{std::make_shared<CSmartPlaylistRule>()};
    if (rule->Load(ruleNode, encoding))
      m_ruleCombination.AddRule(rule);

    ruleNode = ruleNode->NextSibling("rule");
  }

  const TiXmlElement *groupElement = root->FirstChildElement("group");
  if (groupElement != NULL && groupElement->FirstChild() != NULL)
  {
    m_group = groupElement->FirstChild()->ValueStr();
    const char* mixed = groupElement->Attribute("mixed");
    m_groupMixed = mixed != NULL && StringUtils::EqualsNoCase(mixed, "true");
  }

  // now any limits
  // format is <limit>25</limit>
  XMLUtils::GetUInt(root, "limit", m_limit);

  // and order
  // format is <order direction="ascending">field</order>
  const TiXmlElement *order = root->FirstChildElement("order");
  if (order && order->FirstChild())
  {
    const char *direction = order->Attribute("direction");
    if (direction)
      m_orderDirection = StringUtils::EqualsNoCase(direction, "ascending") ? SortOrder::ASCENDING
                                                                           : SortOrder::DESCENDING;

    const char *ignorefolders = order->Attribute("ignorefolders");
    if (ignorefolders != NULL)
      m_orderAttributes = StringUtils::EqualsNoCase(ignorefolders, "true") ? SortAttributeIgnoreFolders : SortAttributeNone;

    m_orderField = CSmartPlaylistRule::TranslateOrder(order->FirstChild()->Value());
  }
  return true;
}

bool CSmartPlaylist::LoadFromJson(const std::string &json)
{
  if (json.empty())
    return false;

  CVariant obj;
  if (!CJSONVariantParser::Parse(json, obj))
    return false;

  return Load(obj);
}

bool CSmartPlaylist::Save(const std::string &path) const
{
  CXBMCTinyXML doc;
  TiXmlDeclaration decl("1.0", "UTF-8", "yes");
  doc.InsertEndChild(decl);

  TiXmlElement xmlRootElement("smartplaylist");
  xmlRootElement.SetAttribute("type",m_playlistType.c_str());
  TiXmlNode *pRoot = doc.InsertEndChild(xmlRootElement);
  if (!pRoot)
    return false;

  // add the <name> tag
  XMLUtils::SetString(pRoot, "name", m_playlistName);

  // add the <match> tag
  XMLUtils::SetString(
      pRoot, "match",
      m_ruleCombination.GetType() == CDatabaseQueryRuleCombination::Type::COMBINATION_AND ? "all"
                                                                                          : "one");

  // add <rule> tags
  m_ruleCombination.Save(pRoot);

  // add <group> tag if necessary
  if (!m_group.empty())
  {
    TiXmlElement nodeGroup("group");
    if (m_groupMixed)
      nodeGroup.SetAttribute("mixed", "true");
    TiXmlText group(m_group.c_str());
    nodeGroup.InsertEndChild(group);
    pRoot->InsertEndChild(nodeGroup);
  }

  // add <limit> tag
  if (m_limit)
    XMLUtils::SetInt(pRoot, "limit", m_limit);

  // add <order> tag
  if (m_orderField != SortByNone)
  {
    TiXmlText order(CSmartPlaylistRule::TranslateOrder(m_orderField).c_str());
    TiXmlElement nodeOrder("order");
    nodeOrder.SetAttribute("direction",
                           m_orderDirection == SortOrder::DESCENDING ? "descending" : "ascending");
    if (m_orderAttributes & SortAttributeIgnoreFolders)
      nodeOrder.SetAttribute("ignorefolders", "true");
    nodeOrder.InsertEndChild(order);
    pRoot->InsertEndChild(nodeOrder);
  }
  return doc.SaveFile(path);
}

bool CSmartPlaylist::Save(CVariant &obj, bool full /* = true */) const
{
  if (obj.type() == CVariant::VariantTypeConstNull)
    return false;

  obj.clear();
  // add "type"
  obj["type"] = m_playlistType;

  // add "rules"
  CVariant rulesObj = CVariant(CVariant::VariantTypeObject);
  if (m_ruleCombination.Save(rulesObj))
    obj["rules"] = rulesObj;

  // add "group"
  if (!m_group.empty())
  {
    obj["group"]["type"] = m_group;
    obj["group"]["mixed"] = m_groupMixed;
  }

  // add "limit"
  if (full && m_limit)
    obj["limit"] = m_limit;

  // add "order"
  if (full && m_orderField != SortByNone)
  {
    obj["order"] = CVariant(CVariant::VariantTypeObject);
    obj["order"]["method"] = CSmartPlaylistRule::TranslateOrder(m_orderField);
    obj["order"]["direction"] =
        m_orderDirection == SortOrder::DESCENDING ? "descending" : "ascending";
    obj["order"]["ignorefolders"] = (m_orderAttributes & SortAttributeIgnoreFolders);
  }

  return true;
}

bool CSmartPlaylist::SaveAsJson(std::string &json, bool full /* = true */) const
{
  CVariant xsp(CVariant::VariantTypeObject);
  if (!Save(xsp, full))
    return false;

  return CJSONVariantWriter::Write(xsp, json, true) && !json.empty();
}

void CSmartPlaylist::Reset()
{
  m_ruleCombination.clear();
  m_limit = 0;
  m_orderField = SortByNone;
  m_orderDirection = SortOrder::NONE;
  m_orderAttributes = SortAttributeNone;
  m_playlistType = "songs"; // sane default
  m_group.clear();
  m_groupMixed = false;
}

void CSmartPlaylist::SetName(const std::string &name)
{
  m_playlistName = name;
}

void CSmartPlaylist::SetType(const std::string &type)
{
  m_playlistType = type;
}

bool CSmartPlaylist::IsVideoType() const
{
  return IsVideoType(m_playlistType);
}

bool CSmartPlaylist::IsMusicType() const
{
  return IsMusicType(m_playlistType);
}

bool CSmartPlaylist::IsVideoType(const std::string &type)
{
  return type == "movies" || type == "tvshows" || type == "episodes" ||
         type == "musicvideos" || type == "mixed";
}

bool CSmartPlaylist::IsMusicType(const std::string &type)
{
  return type == "artists" || type == "albums" ||
         type == "songs" || type == "mixed";
}

std::string CSmartPlaylist::GetWhereClause(
    const CDatabase& db, std::set<std::string, std::less<>>& referencedPlaylists) const
{
  return m_ruleCombination.GetWhereClause(db, GetType(), referencedPlaylists);
}

void CSmartPlaylist::GetVirtualFolders(std::vector<std::string> &virtualFolders) const
{
  m_ruleCombination.GetVirtualFolders(GetType(), virtualFolders);
}

std::string CSmartPlaylist::GetSaveLocation() const
{
  if (m_playlistType == "mixed")
    return "mixed";
  if (IsMusicType())
    return "music";
  // all others are video
  return "video";
}

void CSmartPlaylist::GetAvailableFields(const std::string &type, std::vector<std::string> &fieldList)
{
  std::vector<Field> typeFields = CSmartPlaylistRule::GetFields(type);
  for (std::vector<Field>::const_iterator field = typeFields.begin(); field != typeFields.end(); ++field)
  {
    for (const translateField& i : fields)
    {
      if (*field == i.field)
        fieldList.emplace_back(i.string);
    }
  }
}

bool CSmartPlaylist::IsEmpty(bool ignoreSortAndLimit /* = true */) const
{
  bool empty = m_ruleCombination.empty();
  if (empty && !ignoreSortAndLimit)
    empty = m_limit == 0 && m_orderField == SortByNone && m_orderDirection == SortOrder::NONE;

  return empty;
}

bool CSmartPlaylist::CheckTypeCompatibility(const std::string &typeLeft, const std::string &typeRight)
{
  if (typeLeft == typeRight)
    return true;

  if (typeLeft == "mixed" &&
     (typeRight == "songs" || typeRight == "musicvideos"))
    return true;

  if (typeRight == "mixed" &&
     (typeLeft == "songs" || typeLeft == "musicvideos"))
    return true;

  return false;
}

CDatabaseQueryRule *CSmartPlaylist::CreateRule() const
{
  return new CSmartPlaylistRule();
}
CDatabaseQueryRuleCombination *CSmartPlaylist::CreateCombination() const
{
  return new CSmartPlaylistRuleCombination();
}

} // namespace KODI::PLAYLIST

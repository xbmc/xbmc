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

#include <array>
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

struct TranslateField
{
  std::string_view string;
  Field field;
  CDatabaseQueryRule::FieldType type;
  StringValidation::Validator validator;
  bool browseable;
  int localizedString;
};

// clang-format off
static const auto fields = std::array{
  TranslateField{ "none",              Field::NONE,                       TEXT_FIELD,     nullptr,                              false, 231 },
  TranslateField{ "filename",          Field::FILENAME,                   TEXT_FIELD,     nullptr,                              false, 561 },
  TranslateField{ "path",              Field::PATH,                       TEXT_FIELD,     nullptr,                              true,  573 },
  TranslateField{ "album",             Field::ALBUM,                      TEXT_FIELD,     nullptr,                              true,  558 },
  TranslateField{ "albumartist",       Field::ALBUM_ARTIST,               TEXT_FIELD,     nullptr,                              true,  566 },
  TranslateField{ "artist",            Field::ARTIST,                     TEXT_FIELD,     nullptr,                              true,  557 },
  TranslateField{ "tracknumber",       Field::TRACK_NUMBER,               NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 554 },
  TranslateField{ "role",              Field::ROLE,                       TEXT_FIELD,     nullptr,                              true, 38033 },
  TranslateField{ "comment",           Field::COMMENT,                    TEXT_FIELD,     nullptr,                              false, 569 },
  TranslateField{ "review",            Field::REVIEW,                     TEXT_FIELD,     nullptr,                              false, 183 },
  TranslateField{ "themes",            Field::THEMES,                     TEXT_FIELD,     nullptr,                              false, 21895 },
  TranslateField{ "moods",             Field::MOODS,                      TEXT_FIELD,     nullptr,                              false, 175 },
  TranslateField{ "styles",            Field::STYLES,                     TEXT_FIELD,     nullptr,                              false, 176 },
  TranslateField{ "type",              Field::ALBUM_TYPE,                 TEXT_FIELD,     nullptr,                              false, 564 },
  TranslateField{ "compilation",       Field::COMPILATION,                BOOLEAN_FIELD,  nullptr,                              false, 204 },
  TranslateField{ "label",             Field::MUSIC_LABEL,                TEXT_FIELD,     nullptr,                              false, 21899 },
  TranslateField{ "title",             Field::TITLE,                      TEXT_FIELD,     nullptr,                              true,  556 },
  TranslateField{ "sorttitle",         Field::SORT_TITLE,                 TEXT_FIELD,     nullptr,                              false, 171 },
  TranslateField{ "originaltitle",     Field::ORIGINAL_TITLE,             TEXT_FIELD,     nullptr,                              false, 20376 },
  TranslateField{ "year",              Field::YEAR,                       NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  true,  562 },
  TranslateField{ "time",              Field::TIME,                       SECONDS_FIELD,  StringValidation::IsTime,             false, 180 },
  TranslateField{ "playcount",         Field::PLAYCOUNT,                  NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 567 },
  TranslateField{ "lastplayed",        Field::LAST_PLAYED,                DATE_FIELD,     CSmartPlaylistRule::ValidateDate,     false, 568 },
  TranslateField{ "inprogress",        Field::IN_PROGRESS,                BOOLEAN_FIELD,  nullptr,                              false, 575 },
  TranslateField{ "rating",            Field::RATING,                     REAL_FIELD,     CSmartPlaylistRule::ValidateRating,   false, 563 },
  TranslateField{ "userrating",        Field::USER_RATING,                REAL_FIELD,     CSmartPlaylistRule::ValidateMyRating, false, 38018 },
  TranslateField{ "votes",             Field::VOTES,                      REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 205 },
  TranslateField{ "top250",            Field::TOP250,                     NUMERIC_FIELD,  nullptr,                              false, 13409 },
  TranslateField{ "mpaarating",        Field::MPAA,                       TEXT_FIELD,     nullptr,                              false, 20074 },
  TranslateField{ "dateadded",         Field::DATE_ADDED,                 DATE_FIELD,     CSmartPlaylistRule::ValidateDate,     false, 570 },
  TranslateField{ "datemodified",      Field::DATE_MODIFIED,              DATE_FIELD,     CSmartPlaylistRule::ValidateDate,     false, 39119 },
  TranslateField{ "datenew",           Field::DATE_NEW,                   DATE_FIELD,     CSmartPlaylistRule::ValidateDate,     false, 21877 },
  TranslateField{ "genre",             Field::GENRE,                      TEXT_FIELD,     nullptr,                              true,  515 },
  TranslateField{ "plot",              Field::PLOT,                       TEXT_FIELD,     nullptr,                              false, 207 },
  TranslateField{ "plotoutline",       Field::PLOT_OUTLINE,               TEXT_FIELD,     nullptr,                              false, 203 },
  TranslateField{ "tagline",           Field::TAGLINE,                    TEXT_FIELD,     nullptr,                              false, 202 },
  TranslateField{ "set",               Field::SET,                        TEXT_FIELD,     nullptr,                              true,  20457 },
  TranslateField{ "director",          Field::DIRECTOR,                   TEXT_FIELD,     nullptr,                              true,  20339 },
  TranslateField{ "actor",             Field::ACTOR,                      TEXT_FIELD,     nullptr,                              true,  20337 },
  TranslateField{ "writers",           Field::WRITER,                     TEXT_FIELD,     nullptr,                              true,  20417 },
  TranslateField{ "airdate",           Field::AIR_DATE,                   DATE_FIELD,     CSmartPlaylistRule::ValidateDate,     false, 20416 },
  TranslateField{ "hastrailer",        Field::TRAILER,                    BOOLEAN_FIELD,  nullptr,                              false, 20423 },
  TranslateField{ "studio",            Field::STUDIO,                     TEXT_FIELD,     nullptr,                              true,  572 },
  TranslateField{ "country",           Field::COUNTRY,                    TEXT_FIELD,     nullptr,                              true,  574 },
  TranslateField{ "tvshow",            Field::TVSHOW_TITLE,               TEXT_FIELD,     nullptr,                              true,  20364 },
  TranslateField{ "status",            Field::TVSHOW_STATUS,              TEXT_FIELD,     nullptr,                              false, 126 },
  TranslateField{ "season",            Field::SEASON,                     NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 20373 },
  TranslateField{ "episode",           Field::EPISODE_NUMBER,             NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 20359 },
  TranslateField{ "numepisodes",       Field::NUMBER_OF_EPISODES,         REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 20360 },
  TranslateField{ "numwatched",        Field::NUMBER_OF_WATCHED_EPISODES, REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 21457 },
  TranslateField{ "videoresolution",   Field::VIDEO_RESOLUTION,           REAL_FIELD,     nullptr,                              false, 21443 },
  TranslateField{ "videocodec",        Field::VIDEO_CODEC,                TEXTIN_FIELD,   nullptr,                              false, 21445 },
  TranslateField{ "videoaspect",       Field::VIDEO_ASPECT_RATIO,         REAL_FIELD,     nullptr,                              false, 21374 },
  TranslateField{ "audiochannels",     Field::AUDIO_CHANNELS,             REAL_FIELD,     nullptr,                              false, 21444 },
  TranslateField{ "audiocodec",        Field::AUDIO_CODEC,                TEXTIN_FIELD,   nullptr,                              false, 21446 },
  TranslateField{ "audiolanguage",     Field::AUDIO_LANGUAGE,             TEXTIN_FIELD,   nullptr,                              false, 21447 },
  TranslateField{ "audiocount",        Field::AUDIO_COUNT,                REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 21481 },
  TranslateField{ "subtitlecount",     Field::SUBTITLE_COUNT,             REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 21482 },
  TranslateField{ "subtitlelanguage",  Field::SUBTITLE_LANGUAGE,          TEXTIN_FIELD,   nullptr,                              false, 21448 },
  TranslateField{ "random",            Field::RANDOM,                     TEXT_FIELD,     nullptr,                              false, 590 },
  TranslateField{ "playlist",          Field::PLAYLIST,                   PLAYLIST_FIELD, nullptr,                              true,  559 },
  TranslateField{ "virtualfolder",     Field::VIRTUAL_FOLDER,             PLAYLIST_FIELD, nullptr,                              true,  614 },
  TranslateField{ "tag",               Field::TAG,                        TEXT_FIELD,     nullptr,                              true,  20459 },
  TranslateField{ "instruments",       Field::INSTRUMENTS,                TEXT_FIELD,     nullptr,                              false, 21892 },
  TranslateField{ "biography",         Field::BIOGRAPHY,                  TEXT_FIELD,     nullptr,                              false, 21887 },
  TranslateField{ "born",              Field::BORN,                       TEXT_FIELD,     nullptr,                              false, 21893 },
  TranslateField{ "bandformed",        Field::BAND_FORMED,                TEXT_FIELD,     nullptr,                              false, 21894 },
  TranslateField{ "disbanded",         Field::DISBANDED,                  TEXT_FIELD,     nullptr,                              false, 21896 },
  TranslateField{ "died",              Field::DIED,                       TEXT_FIELD,     nullptr,                              false, 21897 },
  TranslateField{ "artisttype",        Field::ARTIST_TYPE,                TEXT_FIELD,     nullptr,                              false, 564 },
  TranslateField{ "gender",            Field::GENDER,                     TEXT_FIELD,     nullptr,                              false, 39025 },
  TranslateField{ "disambiguation",    Field::DISAMBIGUATION,             TEXT_FIELD,     nullptr,                              false, 39026 },
  TranslateField{ "source",            Field::SOURCE,                     TEXT_FIELD,     nullptr,                              true,  39030 },
  TranslateField{ "disctitle",         Field::DISC_TITLE,                 TEXT_FIELD,     nullptr,                              false, 38076 },
  TranslateField{ "isboxset",          Field::IS_BOXSET,                  BOOLEAN_FIELD,  nullptr,                              false, 38074 },
  TranslateField{ "totaldiscs",        Field::TOTAL_DISCS,                NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 38077 },
  TranslateField{ "originalyear",      Field::ORIG_YEAR,                  NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  true,  38078 },
  TranslateField{ "bpm",               Field::BPM,                        NUMERIC_FIELD,  nullptr,                              false, 38080 },
  TranslateField{ "samplerate",        Field::SAMPLE_RATE,                NUMERIC_FIELD,  nullptr,                              false, 613 },
  TranslateField{ "bitrate",           Field::MUSIC_BITRATE,              NUMERIC_FIELD,  nullptr,                              false, 623 },
  TranslateField{ "channels",          Field::NUMBER_OF_CHANNELS,         NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 253 },
  TranslateField{ "albumstatus",       Field::ALBUM_STATUS,               TEXT_FIELD,     nullptr,                              false, 38081 },
  TranslateField{ "albumduration",     Field::ALBUM_DURATION,             SECONDS_FIELD,  StringValidation::IsTime,             false, 180 },
  TranslateField{ "hdrtype",           Field::HDR_TYPE,                   TEXTIN_FIELD,   nullptr,                              false, 20474 },
  TranslateField{ "hasversions",       Field::HAS_VIDEO_VERSIONS,         BOOLEAN_FIELD,  nullptr,                              false, 20475 },
  TranslateField{ "hasextras",         Field::HAS_VIDEO_EXTRAS,           BOOLEAN_FIELD,  nullptr,                              false, 20476 },
};
// clang-format on

struct Group
{
  std::string_view name;
  Field field;
  bool canMix;
  int localizedString;
};

// clang-format off
static const auto groups = std::array{
  Group{ "",               Field::UNKNOWN,    false,    571 },
  Group{ "none",           Field::NONE,       false,    231 },
  Group{ "sets",           Field::SET,        true,   20434 },
  Group{ "genres",         Field::GENRE,      false,    135 },
  Group{ "years",          Field::YEAR,       false,    652 },
  Group{ "actors",         Field::ACTOR,      false,    344 },
  Group{ "directors",      Field::DIRECTOR,   false,  20348 },
  Group{ "writers",        Field::WRITER,     false,  20418 },
  Group{ "studios",        Field::STUDIO,     false,  20388 },
  Group{ "countries",      Field::COUNTRY,    false,  20451 },
  Group{ "artists",        Field::ARTIST,     false,    133 },
  Group{ "albums",         Field::ALBUM,      false,    132 },
  Group{ "tags",           Field::TAG,        false,  20459 },
  Group{ "originalyears",  Field::ORIG_YEAR,  false,  38078 },
};
// clang-format on

constexpr std::string_view RULE_VALUE_SEPARATOR = " / ";

CSmartPlaylistRule::CSmartPlaylistRule() = default;

int CSmartPlaylistRule::TranslateField(const char *field) const
{
  const auto it = std::ranges::find_if(fields, [field](const auto& f)
                                       { return StringUtils::EqualsNoCase(field, f.string); });
  return it == fields.end() ? static_cast<int>(Field::NONE) : static_cast<int>(it->field);
}

std::string CSmartPlaylistRule::TranslateField(int field) const
{
  const auto it = std::ranges::find_if(fields, [field](const auto& f)
                                       { return field == static_cast<int>(f.field); });
  return it == fields.end() ? "none" : std::string(it->string);
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
  const auto it = std::ranges::find_if(groups, [group](const auto& g)
                                       { return StringUtils::EqualsNoCase(group, g.name); });
  return it == groups.end() ? Field::UNKNOWN : it->field;
}

std::string CSmartPlaylistRule::TranslateGroup(Field group)
{
  const auto it = std::ranges::find_if(groups, [group](const auto& g) { return group == g.field; });
  return it == groups.end() ? "" : std::string(it->name);
}

std::string CSmartPlaylistRule::GetLocalizedField(int field)
{
  const auto it = std::ranges::find_if(fields, [field](const auto& f)
                                       { return field == static_cast<int>(f.field); });
  const int str = it == fields.end() ? 16018 : it->localizedString;
  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(str);
}

CDatabaseQueryRule::FieldType CSmartPlaylistRule::GetFieldType(int field) const
{
  const auto it = std::ranges::find_if(fields, [field](const auto& f)
                                       { return field == static_cast<int>(f.field); });
  return it == fields.end() ? TEXT_FIELD : it->type;
}

bool CSmartPlaylistRule::IsFieldBrowseable(int field)
{
  const auto it = std::ranges::find_if(fields, [field](const auto& f)
                                       { return field == static_cast<int>(f.field); });
  return it == fields.end() ? false : it->browseable;
}

bool CSmartPlaylistRule::Validate(const std::string& input, void* data)
{
  if (data == nullptr)
    return true;

  const auto* rule = static_cast<const CSmartPlaylistRule*>(data);

  // check if there's a validator for this rule
  const auto it = std::ranges::find_if(fields, [field = rule->m_field](const auto& f)
                                       { return field == static_cast<int>(f.field); });
  if (it == fields.end())
    return true;

  if (!it->validator)
    return true;

  if (input.empty())
    return it->validator("", data);

  // Split the input into multiple values and validate every value separately
  const std::vector<std::string> values{StringUtils::Split(input, RULE_VALUE_SEPARATOR)};

  return std::ranges::all_of(values, [data, validator = it->validator](const auto& s)
                             { return validator(s, data); });
}

bool CSmartPlaylistRule::ValidateRating(const std::string &input, void *data)
{
  char* end = nullptr;
  std::string strRating = input;
  StringUtils::Trim(strRating);

  const double rating = std::strtod(strRating.c_str(), &end);
  return (end == nullptr || *end == '\0') && rating >= 0.0 && rating <= 10.0;
}

bool CSmartPlaylistRule::ValidateMyRating(const std::string &input, void *data)
{
  std::string strRating = input;
  StringUtils::Trim(strRating);

  const int rating = atoi(strRating.c_str());
  return StringValidation::IsPositiveInteger(input, data) && rating <= 10;
}

bool CSmartPlaylistRule::ValidateDate(const std::string& input, void* data)
{
  if (!data)
    return false;

  const auto* rule = static_cast<const CSmartPlaylistRule*>(data);

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
    fields = {
        Field::GENRE,        Field::ALBUM,          Field::ARTIST, Field::ALBUM_ARTIST,
        Field::TITLE,        Field::ORIGINAL_TITLE, Field::YEAR,   Field::TIME,
        Field::TRACK_NUMBER, Field::FILENAME,       Field::PATH,   Field::PLAYCOUNT,
        Field::LAST_PLAYED,
    };
  }
  else if (type == "songs")
  {
    fields = {
        Field::GENRE,  Field::SOURCE,       Field::ALBUM, Field::DISC_TITLE,
        Field::ARTIST, Field::ALBUM_ARTIST, Field::TITLE, Field::YEAR,
    };
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
      CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
      fields.push_back(Field::ORIG_YEAR);
    fields.insert(fields.end(), {
                                    Field::TIME,
                                    Field::TRACK_NUMBER,
                                    Field::FILENAME,
                                    Field::PATH,
                                    Field::PLAYCOUNT,
                                    Field::LAST_PLAYED,
                                    Field::RATING,
                                    Field::USER_RATING,
                                    Field::COMMENT,
                                    Field::MOODS,
                                    Field::BPM,
                                    Field::SAMPLE_RATE,
                                    Field::MUSIC_BITRATE,
                                    Field::NUMBER_OF_CHANNELS,
                                    Field::DATE_ADDED,
                                    Field::DATE_MODIFIED,
                                    Field::DATE_NEW,
                                });
  }
  else if (type == "albums")
  {
    fields = {
        Field::GENRE,        Field::SOURCE,      Field::ALBUM,
        Field::DISC_TITLE,   Field::TOTAL_DISCS, Field::IS_BOXSET,
        Field::ARTIST, // any artist
        Field::ALBUM_ARTIST, // album artist
        Field::YEAR,
    };
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
      fields.push_back(Field::ORIG_YEAR);
    fields.insert(fields.end(), {
                                    Field::ALBUM_DURATION,
                                    Field::REVIEW,
                                    Field::THEMES,
                                    Field::MOODS,
                                    Field::STYLES,
                                    Field::COMPILATION,
                                    Field::ALBUM_TYPE,
                                    Field::MUSIC_LABEL,
                                    Field::RATING,
                                    Field::USER_RATING,
                                    Field::PLAYCOUNT,
                                    Field::LAST_PLAYED,
                                    Field::PATH,
                                    Field::ALBUM_STATUS,
                                    Field::DATE_ADDED,
                                    Field::DATE_MODIFIED,
                                    Field::DATE_NEW,
                                });
  }
  else if (type == "artists")
  {
    fields = {
        Field::ARTIST,     Field::SOURCE,         Field::GENRE,     Field::MOODS,
        Field::STYLES,     Field::INSTRUMENTS,    Field::BIOGRAPHY, Field::ARTIST_TYPE,
        Field::GENDER,     Field::DISAMBIGUATION, Field::BORN,      Field::BAND_FORMED,
        Field::DISBANDED,  Field::DIED,           Field::ROLE,      Field::PATH,
        Field::DATE_ADDED, Field::DATE_MODIFIED,  Field::DATE_NEW,
    };
  }
  else if (type == "tvshows")
  {
    fields = {
        Field::TITLE,
        Field::ORIGINAL_TITLE,
        Field::PLOT,
        Field::TAGLINE,
        Field::TVSHOW_STATUS,
        Field::VOTES,
        Field::RATING,
        Field::USER_RATING,
        Field::YEAR,
        Field::GENRE,
        Field::DIRECTOR,
        Field::ACTOR,
        Field::NUMBER_OF_EPISODES,
        Field::NUMBER_OF_WATCHED_EPISODES,
        Field::PLAYCOUNT,
        Field::PATH,
        Field::STUDIO,
        Field::MPAA,
        Field::DATE_ADDED,
        Field::LAST_PLAYED,
        Field::IN_PROGRESS,
        Field::TAG,
        Field::TRAILER,
    };
  }
  else if (type == "episodes")
  {
    fields = {
        Field::TITLE,       Field::TVSHOW_TITLE, Field::ORIGINAL_TITLE, Field::PLOT,
        Field::VOTES,       Field::RATING,       Field::USER_RATING,    Field::TIME,
        Field::WRITER,      Field::AIR_DATE,     Field::PLAYCOUNT,      Field::LAST_PLAYED,
        Field::IN_PROGRESS, Field::GENRE,
        Field::YEAR, // premiered
        Field::DIRECTOR,    Field::ACTOR,        Field::EPISODE_NUMBER, Field::SEASON,
        Field::FILENAME,    Field::PATH,         Field::STUDIO,         Field::MPAA,
        Field::DATE_ADDED,  Field::TAG,
    };
    isVideo = true;
  }
  else if (type == "movies")
  {
    fields = {
        Field::TITLE,
        Field::ORIGINAL_TITLE,
        Field::PLOT,
        Field::PLOT_OUTLINE,
        Field::TAGLINE,
        Field::VOTES,
        Field::RATING,
        Field::USER_RATING,
        Field::TIME,
        Field::WRITER,
        Field::PLAYCOUNT,
        Field::LAST_PLAYED,
        Field::IN_PROGRESS,
        Field::GENRE,
        Field::COUNTRY,
        Field::YEAR, // premiered
        Field::DIRECTOR,
        Field::ACTOR,
        Field::MPAA,
        Field::TOP250,
        Field::STUDIO,
        Field::TRAILER,
        Field::FILENAME,
        Field::PATH,
        Field::SET,
        Field::TAG,
        Field::DATE_ADDED,
        Field::HAS_VIDEO_VERSIONS,
        Field::HAS_VIDEO_EXTRAS,
    };
    isVideo = true;
  }
  else if (type == "musicvideos")
  {
    fields = {
        Field::TITLE,       Field::GENRE,      Field::ALBUM,     Field::YEAR,        Field::ARTIST,
        Field::FILENAME,    Field::PATH,       Field::PLAYCOUNT, Field::LAST_PLAYED, Field::RATING,
        Field::USER_RATING, Field::TIME,       Field::DIRECTOR,  Field::STUDIO,      Field::PLOT,
        Field::TAG,         Field::DATE_ADDED,
    };
    isVideo = true;
  }
  if (isVideo)
  {
    fields.insert(fields.end(), {
                                    Field::VIDEO_RESOLUTION,
                                    Field::AUDIO_CHANNELS,
                                    Field::AUDIO_COUNT,
                                    Field::SUBTITLE_COUNT,
                                    Field::VIDEO_CODEC,
                                    Field::AUDIO_CODEC,
                                    Field::AUDIO_LANGUAGE,
                                    Field::SUBTITLE_LANGUAGE,
                                    Field::VIDEO_ASPECT_RATIO,
                                    Field::HDR_TYPE,
                                });
  }
  fields.insert(fields.end(), {
                                  Field::PLAYLIST,
                                  Field::VIRTUAL_FOLDER,
                              });

  return fields;
}

std::vector<SortBy> CSmartPlaylistRule::GetOrders(const std::string& type)
{
  std::vector<SortBy> orders;
  if (type == "mixed")
  {
    orders = {
        SortBy::NONE,  SortBy::GENRE, SortBy::ALBUM,     SortBy::ARTIST,
        SortBy::TITLE, SortBy::YEAR,  SortBy::TIME,      SortBy::TRACK_NUMBER,
        SortBy::FILE,  SortBy::PATH,  SortBy::PLAYCOUNT, SortBy::LAST_PLAYED,
    };
  }
  else if (type == "songs")
  {
    orders = {
        SortBy::NONE, SortBy::GENRE, SortBy::ALBUM, SortBy::ARTIST, SortBy::TITLE, SortBy::YEAR,
    };
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
      CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
      orders.push_back(SortBy::ORIG_DATE);
    orders.insert(orders.end(), {
                                    SortBy::TIME,
                                    SortBy::TRACK_NUMBER,
                                    SortBy::FILE,
                                    SortBy::PATH,
                                    SortBy::PLAYCOUNT,
                                    SortBy::LAST_PLAYED,
                                    SortBy::DATE_ADDED,
                                    SortBy::RATING,
                                    SortBy::USER_RATING,
                                    SortBy::BPM,
                                });
  }
  else if (type == "albums")
  {
    orders = {
        SortBy::NONE,   SortBy::GENRE, SortBy::ALBUM, SortBy::TOTAL_DISCS,
        SortBy::ARTIST, // any artist
        SortBy::YEAR,
    };
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
      orders.push_back(SortBy::ORIG_DATE);
    orders.insert(orders.end(), {
                                    SortBy::ALBUM_TYPE,
                                    SortBy::RATING,
                                    SortBy::USER_RATING,
                                    SortBy::PLAYCOUNT,
                                    SortBy::LAST_PLAYED,
                                    SortBy::DATE_ADDED,
                                });
  }
  else if (type == "artists")
  {
    orders = {SortBy::NONE, SortBy::ARTIST};
  }
  else if (type == "tvshows")
  {
    orders = {
        SortBy::NONE,
        SortBy::SORT_TITLE,
        SortBy::ORIGINAL_TITLE,
        SortBy::TVSHOW_STATUS,
        SortBy::VOTES,
        SortBy::RATING,
        SortBy::USER_RATING,
        SortBy::YEAR,
        SortBy::GENRE,
        SortBy::NUMBER_OF_EPISODES,
        SortBy::NUMBER_OF_WATCHED_EPISODES,
        SortBy::PATH,
        SortBy::STUDIO,
        SortBy::MPAA,
        SortBy::DATE_ADDED,
        SortBy::LAST_PLAYED,
    };
  }
  else if (type == "episodes")
  {
    orders = {
        SortBy::NONE,           SortBy::TITLE,       SortBy::ORIGINAL_TITLE, SortBy::TVSHOW_TITLE,
        SortBy::VOTES,          SortBy::RATING,      SortBy::USER_RATING,    SortBy::TIME,
        SortBy::PLAYCOUNT,      SortBy::LAST_PLAYED,
        SortBy::YEAR, // premiered/dateaired
        SortBy::EPISODE_NUMBER, SortBy::SEASON,      SortBy::FILE,           SortBy::PATH,
        SortBy::STUDIO,         SortBy::MPAA,        SortBy::DATE_ADDED,
    };
  }
  else if (type == "movies")
  {
    orders = {
        SortBy::NONE,        SortBy::SORT_TITLE,  SortBy::ORIGINAL_TITLE, SortBy::VOTES,
        SortBy::RATING,      SortBy::USER_RATING, SortBy::TIME,           SortBy::PLAYCOUNT,
        SortBy::LAST_PLAYED, SortBy::GENRE,       SortBy::COUNTRY,
        SortBy::YEAR, // premiered
        SortBy::MPAA,        SortBy::TOP250,      SortBy::STUDIO,         SortBy::FILE,
        SortBy::PATH,        SortBy::DATE_ADDED,
    };
  }
  else if (type == "musicvideos")
  {
    orders = {
        SortBy::NONE,   SortBy::TITLE,  SortBy::GENRE,       SortBy::ALBUM,     SortBy::YEAR,
        SortBy::ARTIST, SortBy::FILE,   SortBy::PATH,        SortBy::PLAYCOUNT, SortBy::LAST_PLAYED,
        SortBy::TIME,   SortBy::RATING, SortBy::USER_RATING, SortBy::STUDIO,    SortBy::DATE_ADDED,
    };
  }
  orders.push_back(SortBy::RANDOM);

  return orders;
}

std::vector<Field> CSmartPlaylistRule::GetGroups(const std::string &type)
{
  std::vector<Field> groups;
  if (type == "artists")
  {
    groups = {
        Field::UNKNOWN,
        Field::GENRE,
    };
  }
  else if (type == "albums")
  {
    groups = {
        Field::UNKNOWN,
        Field::YEAR,
    };
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
      groups.push_back(Field::ORIG_YEAR);
  }
  if (type == "movies")
  {
    groups = {
        Field::UNKNOWN,  Field::NONE,   Field::SET,    Field::GENRE,   Field::YEAR, Field::ACTOR,
        Field::DIRECTOR, Field::WRITER, Field::STUDIO, Field::COUNTRY, Field::TAG,
    };
  }
  else if (type == "tvshows")
  {
    groups = {
        Field::UNKNOWN,  Field::GENRE,  Field::YEAR, Field::ACTOR,
        Field::DIRECTOR, Field::STUDIO, Field::TAG,
    };
  }
  else if (type == "musicvideos")
  {
    groups = {
        Field::UNKNOWN, Field::ARTIST,   Field::ALBUM,  Field::GENRE,
        Field::YEAR,    Field::DIRECTOR, Field::STUDIO, Field::TAG,
    };
  }

  return groups;
}

std::string CSmartPlaylistRule::GetLocalizedGroup(Field group)
{
  const auto it = std::ranges::find_if(groups, [group](const auto& g) { return group == g.field; });
  const int str = it == groups.end() ? groups[0].localizedString : it->localizedString;
  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(str);
}

bool CSmartPlaylistRule::CanGroupMix(Field group)
{
  const auto it = std::ranges::find_if(groups, [group](const auto& g) { return group == g.field; });
  return it == groups.end() ? false : it->canMix;
}

std::string CSmartPlaylistRule::GetLocalizedRule() const
{
  return StringUtils::Format("{} {} {}", GetLocalizedField(m_field),
                             GetLocalizedOperator(m_operator), GetParameter());
}

std::string CSmartPlaylistRule::GetVideoResolutionQuery(const std::string &parameter) const
{
  std::string retVal(" IN (SELECT DISTINCT idFile FROM streamdetails WHERE iVideoWidth ");
  int iRes = static_cast<int>(std::strtol(parameter.c_str(), nullptr, 10));

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
    if (m_field == static_cast<int>(Field::IN_PROGRESS))
      return "movie_view.idFile " + negate + " IN (SELECT DISTINCT idFile FROM bookmark WHERE type = 1)";
    else if (m_field == static_cast<int>(Field::TRAILER))
      return negate + GetField(m_field, strType) + "!= ''";
    else if (m_field == static_cast<int>(Field::HAS_VIDEO_VERSIONS) ||
             m_field == static_cast<int>(Field::HAS_VIDEO_EXTRAS))
      return negate + GetField(m_field, strType);
  }
  else if (strType == "episodes")
  {
    if (m_field == static_cast<int>(Field::IN_PROGRESS))
      return "episode_view.idFile " + negate + " IN (SELECT DISTINCT idFile FROM bookmark WHERE type = 1)";
  }
  else if (strType == "tvshows")
  {
    if (m_field == static_cast<int>(Field::IN_PROGRESS))
      return negate +
             " ("
             "(tvshow_view.watchedcount > 0 AND tvshow_view.watchedcount < tvshow_view.totalCount) "
             "OR "
             "(tvshow_view.watchedcount = 0 AND EXISTS "
             "(SELECT 1 FROM episode_view WHERE episode_view.idShow = " +
             GetField(static_cast<int>(Field::ID), strType) +
             " AND episode_view.resumeTimeInSeconds > 0)"
             ")"
             ")";
    else if (m_field == static_cast<int>(Field::TRAILER))
      return negate + GetField(m_field, strType) + "!= ''";
  }
  if (strType == "albums")
  {
    if (m_field == static_cast<int>(Field::COMPILATION))
      return negate + GetField(m_field, strType);
    if (m_field == static_cast<int>(Field::IS_BOXSET))
      return negate + "albumview.bBoxedSet = 1";
  }
  return "";
}

CDatabaseQueryRule::SearchOperator CSmartPlaylistRule::GetOperator(const std::string& strType) const
{
  SearchOperator op = CDatabaseQueryRule::GetOperator(strType);
  if ((strType == "tvshows" || strType == "episodes") && m_field == static_cast<int>(Field::YEAR))
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
  if (m_field == static_cast<int>(Field::TIME) ||
      m_field == static_cast<int>(Field::ALBUM_DURATION))
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

    if (m_field == static_cast<int>(Field::GENRE))
      query = negate + " EXISTS (SELECT 1 FROM song_genre, genre WHERE song_genre.idSong = " +
              GetField(static_cast<int>(Field::ID), strType) +
              " AND song_genre.idGenre = genre.idGenre AND genre.strGenre" + parameter + ")";
    else if (m_field == static_cast<int>(Field::ARTIST))
      query = negate + " EXISTS (SELECT 1 FROM song_artist, artist WHERE song_artist.idSong = " +
              GetField(static_cast<int>(Field::ID), strType) +
              " AND song_artist.idArtist = artist.idArtist AND artist.strArtist" + parameter + ")";
    else if (m_field == static_cast<int>(Field::ALBUM_ARTIST))
      query = negate + " EXISTS (SELECT 1 FROM album_artist, artist WHERE album_artist.idAlbum = " + table + ".idAlbum AND album_artist.idArtist = artist.idArtist AND artist.strArtist" + parameter + ")";
    else if (m_field == static_cast<int>(Field::LAST_PLAYED))
      query = FormatNullableDate(GetField(m_field, strType), m_operator, parameter);
    else if (m_field == static_cast<int>(Field::SOURCE))
      query = negate + " EXISTS (SELECT 1 FROM album_source, source WHERE album_source.idAlbum = " + table + ".idAlbum AND album_source.idSource = source.idSource AND source.strName" + parameter + ")";
    else if (m_field == static_cast<int>(Field::YEAR) ||
             m_field == static_cast<int>(Field::ORIG_YEAR))
    {
      std::string field;
      if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
        field = GetField(static_cast<int>(Field::ORIG_YEAR), strType);
      else
        field = GetField(m_field, strType);
      query = FormatYearQuery(field, param, parameter);
    }
  }
  else if (strType == "albums")
  {
    table = "albumview";

    if (m_field == static_cast<int>(Field::GENRE))
      query = negate + " EXISTS (SELECT 1 FROM song, song_genre, genre WHERE song.idAlbum = " +
              GetField(static_cast<int>(Field::ID), strType) +
              " AND song.idSong = song_genre.idSong AND song_genre.idGenre = genre.idGenre AND "
              "genre.strGenre" +
              parameter + ")";
    else if (m_field == static_cast<int>(Field::ARTIST))
      query = negate + " EXISTS (SELECT 1 FROM song, song_artist, artist WHERE song.idAlbum = " +
              GetField(static_cast<int>(Field::ID), strType) +
              " AND song.idSong = song_artist.idSong AND song_artist.idArtist = artist.idArtist "
              "AND artist.strArtist" +
              parameter + ")";
    else if (m_field == static_cast<int>(Field::ALBUM_ARTIST))
      query = negate + " EXISTS (SELECT 1 FROM album_artist, artist WHERE album_artist.idAlbum = " +
              GetField(static_cast<int>(Field::ID), strType) +
              " AND album_artist.idArtist = artist.idArtist AND artist.strArtist" + parameter + ")";
    else if (m_field == static_cast<int>(Field::PATH))
      query = negate +
              " EXISTS (SELECT 1 FROM song JOIN path on song.idpath = path.idpath WHERE "
              "song.idAlbum = " +
              GetField(static_cast<int>(Field::ID), strType) + " AND path.strPath" + parameter +
              ")";
    else if (m_field == static_cast<int>(Field::LAST_PLAYED))
      query = FormatNullableDate(GetField(m_field, strType), m_operator, parameter);
    else if (m_field == static_cast<int>(Field::SOURCE))
      query = negate + " EXISTS (SELECT 1 FROM album_source, source WHERE album_source.idAlbum = " +
              GetField(static_cast<int>(Field::ID), strType) +
              " AND album_source.idSource = source.idSource AND source.strName" + parameter + ")";
    else if (m_field == static_cast<int>(Field::DISC_TITLE))
      query = negate + " EXISTS (SELECT 1 FROM song WHERE song.idAlbum = " +
              GetField(static_cast<int>(Field::ID), strType) + " AND song.strDiscSubtitle" +
              parameter + ")";
    else if (m_field == static_cast<int>(Field::YEAR) ||
             m_field == static_cast<int>(Field::ORIG_YEAR))
    {
      std::string field;
      if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
              CSettings::SETTING_MUSICLIBRARY_USEORIGINALDATE))
        field = GetField(static_cast<int>(Field::ORIG_YEAR), strType);
      else
        field = GetField(m_field, strType);
      query = FormatYearQuery(field, param, parameter);
    }
  }
  else if (strType == "artists")
  {
    table = "artistview";

    if (m_field == static_cast<int>(Field::GENRE))
    {
      query = negate +
              " (EXISTS (SELECT DISTINCT song_artist.idArtist FROM song_artist, song_genre, genre "
              "WHERE song_artist.idArtist = " +
              GetField(static_cast<int>(Field::ID), strType) +
              " AND song_artist.idSong = song_genre.idSong AND song_genre.idGenre = genre.idGenre "
              "AND genre.strGenre" +
              parameter + ")";
      query += " OR ";
      query += "EXISTS (SELECT DISTINCT album_artist.idArtist FROM album_artist, song, song_genre, "
               "genre WHERE album_artist.idArtist = " +
               GetField(static_cast<int>(Field::ID), strType) +
               " AND song.idAlbum = album_artist.idAlbum AND song.idSong = song_genre.idSong AND "
               "song_genre.idGenre = genre.idGenre AND genre.strGenre" +
               parameter + "))";
    }
    else if (m_field == static_cast<int>(Field::ROLE))
    {
      query = negate +
              " (EXISTS (SELECT DISTINCT song_artist.idArtist FROM song_artist, role WHERE "
              "song_artist.idArtist = " +
              GetField(static_cast<int>(Field::ID), strType) +
              " AND song_artist.idRole = role.idRole AND role.strRole" + parameter + "))";
    }
    else if (m_field == static_cast<int>(Field::PATH))
    {
      query = negate + " (EXISTS (SELECT DISTINCT song_artist.idArtist FROM song_artist JOIN song ON song.idSong = song_artist.idSong JOIN path ON song.idpath = path.idpath ";
      query += "WHERE song_artist.idArtist = " + GetField(static_cast<int>(Field::ID), strType) +
               " AND path.strPath" + parameter + "))";
    }
    else if (m_field == static_cast<int>(Field::SOURCE))
    {
      query = negate +
              " (EXISTS(SELECT 1 FROM song_artist, song, album_source, source WHERE "
              "song_artist.idArtist = " +
              GetField(static_cast<int>(Field::ID), strType) +
              " AND song.idSong = song_artist.idSong AND song_artist.idRole = 1 AND "
              "album_source.idAlbum = song.idAlbum AND album_source.idSource = source.idSource AND "
              "source.strName" +
              parameter + ")";
      query += " OR ";
      query += " EXISTS (SELECT 1 FROM album_artist, album_source, source WHERE "
               "album_artist.idArtist = " +
               GetField(static_cast<int>(Field::ID), strType) +
               " AND album_source.idAlbum = album_artist.idAlbum AND album_source.idSource = "
               "source.idSource AND source.strName" +
               parameter + "))";
    }
  }
  else if (strType == "movies")
  {
    table = "movie_view";

    if (m_field == static_cast<int>(Field::GENRE))
      query = negate + FormatLinkQuery("genre", "genre", MediaTypeMovie,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
    else if (m_field == static_cast<int>(Field::DIRECTOR))
      query = negate + FormatLinkQuery("director", "actor", MediaTypeMovie,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
    else if (m_field == static_cast<int>(Field::ACTOR))
      query = negate + FormatLinkQuery("actor", "actor", MediaTypeMovie,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
    else if (m_field == static_cast<int>(Field::WRITER))
      query = negate + FormatLinkQuery("writer", "actor", MediaTypeMovie,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
    else if (m_field == static_cast<int>(Field::STUDIO))
      query = negate + FormatLinkQuery("studio", "studio", MediaTypeMovie,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
    else if (m_field == static_cast<int>(Field::COUNTRY))
      query = negate + FormatLinkQuery("country", "country", MediaTypeMovie,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
    else if (m_field == static_cast<int>(Field::LAST_PLAYED) ||
             m_field == static_cast<int>(Field::DATE_ADDED))
      query = FormatNullableDate(GetField(m_field, strType), m_operator, parameter);
    else if (m_field == static_cast<int>(Field::TAG))
      query = negate + FormatLinkQuery("tag", "tag", MediaTypeMovie,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
  }
  else if (strType == "musicvideos")
  {
    table = "musicvideo_view";

    if (m_field == static_cast<int>(Field::GENRE))
      query = negate + FormatLinkQuery("genre", "genre", MediaTypeMusicVideo,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
    else if (m_field == static_cast<int>(Field::ARTIST) ||
             m_field == static_cast<int>(Field::ALBUM_ARTIST))
      query = negate + FormatLinkQuery("actor", "actor", MediaTypeMusicVideo,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
    else if (m_field == static_cast<int>(Field::STUDIO))
      query = negate + FormatLinkQuery("studio", "studio", MediaTypeMusicVideo,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
    else if (m_field == static_cast<int>(Field::DIRECTOR))
      query = negate + FormatLinkQuery("director", "actor", MediaTypeMusicVideo,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
    else if (m_field == static_cast<int>(Field::LAST_PLAYED) ||
             m_field == static_cast<int>(Field::DATE_ADDED))
      query = FormatNullableDate(GetField(m_field, strType), m_operator, parameter);
    else if (m_field == static_cast<int>(Field::TAG))
      query = negate + FormatLinkQuery("tag", "tag", MediaTypeMusicVideo,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
  }
  else if (strType == "tvshows")
  {
    table = "tvshow_view";

    if (m_field == static_cast<int>(Field::GENRE))
      query = negate + FormatLinkQuery("genre", "genre", MediaTypeTvShow,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
    else if (m_field == static_cast<int>(Field::DIRECTOR))
      query = negate + FormatLinkQuery("director", "actor", MediaTypeTvShow,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
    else if (m_field == static_cast<int>(Field::ACTOR))
      query = negate + FormatLinkQuery("actor", "actor", MediaTypeTvShow,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
    else if (m_field == static_cast<int>(Field::STUDIO))
      query = negate + FormatLinkQuery("studio", "studio", MediaTypeTvShow,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
    else if (m_field == static_cast<int>(Field::MPAA))
      query = negate + " (" + GetField(m_field, strType) + parameter + ")";
    else if (m_field == static_cast<int>(Field::LAST_PLAYED) ||
             m_field == static_cast<int>(Field::DATE_ADDED))
      query = FormatNullableDate(GetField(m_field, strType), m_operator, parameter);
    else if (m_field == static_cast<int>(Field::PLAYCOUNT))
      query = "CASE WHEN COALESCE(" +
              GetField(static_cast<int>(Field::NUMBER_OF_EPISODES), strType) + " - " +
              GetField(static_cast<int>(Field::NUMBER_OF_WATCHED_EPISODES), strType) +
              ", 0) > 0 THEN 0 ELSE 1 END " + parameter;
    else if (m_field == static_cast<int>(Field::TAG))
      query = negate + FormatLinkQuery("tag", "tag", MediaTypeTvShow,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
  }
  else if (strType == "episodes")
  {
    table = "episode_view";

    if (m_field == static_cast<int>(Field::GENRE))
      query = negate + FormatLinkQuery("genre", "genre", MediaTypeTvShow, (table + ".idShow").c_str(), parameter);
    else if (m_field == static_cast<int>(Field::TAG))
      query = negate + FormatLinkQuery("tag", "tag", MediaTypeTvShow, (table + ".idShow").c_str(), parameter);
    else if (m_field == static_cast<int>(Field::DIRECTOR))
      query = negate + FormatLinkQuery("director", "actor", MediaTypeEpisode,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
    else if (m_field == static_cast<int>(Field::ACTOR))
      query = negate + FormatLinkQuery("actor", "actor", MediaTypeEpisode,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
    else if (m_field == static_cast<int>(Field::WRITER))
      query = negate + FormatLinkQuery("writer", "actor", MediaTypeEpisode,
                                       GetField(static_cast<int>(Field::ID), strType), parameter);
    else if (m_field == static_cast<int>(Field::LAST_PLAYED) ||
             m_field == static_cast<int>(Field::DATE_ADDED))
      query = FormatNullableDate(GetField(m_field, strType), m_operator, parameter);
    else if (m_field == static_cast<int>(Field::STUDIO))
      query = negate + FormatLinkQuery("studio", "studio", MediaTypeTvShow, (table + ".idShow").c_str(), parameter);
    else if (m_field == static_cast<int>(Field::MPAA))
      query = negate + " (" + GetField(m_field, strType) +  parameter + ")";
  }
  if (m_field == static_cast<int>(Field::VIDEO_RESOLUTION))
    query = table + ".idFile" + negate + GetVideoResolutionQuery(param);
  else if (m_field == static_cast<int>(Field::AUDIO_CHANNELS))
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND iAudioChannels " + parameter + ")";
  else if (m_field == static_cast<int>(Field::VIDEO_CODEC))
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strVideoCodec " + parameter + ")";
  else if (m_field == static_cast<int>(Field::AUDIO_CODEC))
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strAudioCodec " + parameter + ")";
  else if (m_field == static_cast<int>(Field::AUDIO_LANGUAGE))
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strAudioLanguage " + parameter + ")";
  else if (m_field == static_cast<int>(Field::SUBTITLE_LANGUAGE))
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strSubtitleLanguage " + parameter + ")";
  else if (m_field == static_cast<int>(Field::VIDEO_ASPECT_RATIO))
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND fVideoAspect " + parameter + ")";
  else if (m_field == static_cast<int>(Field::AUDIO_COUNT))
    query = db.PrepareSQL(negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND streamdetails.iStreamtype = %i GROUP BY streamdetails.idFile HAVING COUNT(streamdetails.iStreamType) " + parameter + ")",CStreamDetail::AUDIO);
  else if (m_field == static_cast<int>(Field::SUBTITLE_COUNT))
    query = db.PrepareSQL(negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND streamdetails.iStreamType = %i GROUP BY streamdetails.idFile HAVING COUNT(streamdetails.iStreamType) " + parameter + ")",CStreamDetail::SUBTITLE);
  else if (m_field == static_cast<int>(Field::HDR_TYPE))
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strHdrType " + parameter + ")";

  if ((m_field == static_cast<int>(Field::PLAYCOUNT) && strType != "songs" && strType != "albums" &&
       strType != "tvshows") ||
      m_field == static_cast<int>(Field::USER_RATING))
    query = FormatNullableNumber(GetField(m_field, strType), m_operator, param, parameter);

  if (query.empty())
    query = CDatabaseQueryRule::FormatWhereClause(negate, oper, param, db, strType);

  return query;
}

std::string CSmartPlaylistRule::GetField(int field, const std::string &type) const
{
  if (field >= static_cast<int>(Field::UNKNOWN) && field < static_cast<int>(Field::MAX))
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
    if (r->m_field == static_cast<int>(Field::VIRTUAL_FOLDER))
      continue;

    if (!rule.empty())
      rule += GetType() == CDatabaseQueryRuleCombination::Type::COMBINATION_AND ? " AND " : " OR ";
    rule += "(";
    std::string currentRule;
    if (r->m_field == static_cast<int>(Field::PLAYLIST))
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
    if ((r->m_field != static_cast<int>(Field::VIRTUAL_FOLDER) &&
         r->m_field != static_cast<int>(Field::PLAYLIST)) ||
        r->m_operator != OPERATOR_EQUALS)
      continue;

    const std::string playlistFile =
        CSmartPlaylistDirectory::GetPlaylistByName(r->m_parameter.at(0), strType);
    if (playlistFile.empty())
      continue;

    if (r->m_field == static_cast<int>(Field::VIRTUAL_FOLDER))
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
  if (readNameFromPath(url) == nullptr)
    return false;

  return !m_playlistName.empty();
}

const TiXmlNode* CSmartPlaylist::readName(const TiXmlNode *root)
{
  if (root == nullptr)
    return nullptr;

  const TiXmlElement *rootElem = root->ToElement();
  if (rootElem == nullptr)
    return nullptr;

  if (!StringUtils::EqualsNoCase(root->Value(), "smartplaylist"))
  {
    CLog::Log(LOGERROR, "Error loading Smart playlist");
    return nullptr;
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
    return nullptr;
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
    return nullptr;
  }

  m_xmlDoc.Clear();
  if (!m_xmlDoc.Parse(xml))
  {
    CLog::Log(LOGERROR, "Error loading Smart playlist (failed to parse xml: {})",
              m_xmlDoc.ErrorDesc());
    return nullptr;
  }

  const TiXmlNode *root = readName(m_xmlDoc.RootElement());

  return root;
}

bool CSmartPlaylist::load(const TiXmlNode *root)
{
  if (root == nullptr)
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
  if (groupElement != nullptr && groupElement->FirstChild() != nullptr)
  {
    m_group = groupElement->FirstChild()->ValueStr();
    const char* mixed = groupElement->Attribute("mixed");
    m_groupMixed = mixed != nullptr && StringUtils::EqualsNoCase(mixed, "true");
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
    if (ignorefolders != nullptr)
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
  if (m_orderField != SortBy::NONE)
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
  if (full && m_orderField != SortBy::NONE)
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
  m_orderField = SortBy::NONE;
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
  const std::vector<Field> typeFields = CSmartPlaylistRule::GetFields(type);
  for (const auto& field : typeFields)
  {
    const auto it =
        std::ranges::find_if(fields, [field](const auto& f) { return field == f.field; });
    if (it != fields.end())
      fieldList.emplace_back(it->string);
  }
}

bool CSmartPlaylist::IsEmpty(bool ignoreSortAndLimit /* = true */) const
{
  bool empty = m_ruleCombination.empty();
  if (empty && !ignoreSortAndLimit)
    empty = m_limit == 0 && m_orderField == SortBy::NONE && m_orderDirection == SortOrder::NONE;

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

/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LibraryQueryRule.h"

#include "ServiceBroker.h"
#include "XBDateTime.h"
#include "dbwrappers/Database.h"
#include "dbwrappers/DatabaseQueryImpl.h"
#include "media/LibraryQuery.h"
#include "media/LibraryQueryReferences.h"
#include "media/LibraryQueryType.h"
#include "music/MusicLibraryQuery.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/DatabaseUtils.h"
#include "utils/StringUtils.h"
#include "utils/StringValidation.h"
#include "utils/log.h"
#include "video/VideoLibraryQuery.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using enum CDatabaseQueryRule::FieldType;
using enum CDatabaseQueryRule::SearchOperator;
using KODI::MEDIA::MediaTypeFromName;

template class CDatabaseQueryRuleOf<Field, KODI::MEDIA::CLibraryQueryContext>;
template class CDatabaseQueryRuleCombination<KODI::MEDIA::CLibraryQueryRule>;

namespace KODI::MEDIA
{

namespace
{
bool ValidateRating(const std::string& input, void* data)
{
  char* end = nullptr;
  std::string strRating = input;
  StringUtils::Trim(strRating);

  const double rating = std::strtod(strRating.c_str(), &end);
  return (end == nullptr || *end == '\0') && rating >= 0.0 && rating <= 10.0;
}

bool ValidateMyRating(const std::string& input, void* data)
{
  std::string strRating = input;
  StringUtils::Trim(strRating);

  const int rating = atoi(strRating.c_str());
  return StringValidation::IsPositiveInteger(input, data) && rating <= 10;
}

using LIBRARY_QUERY::BROWSE;

struct FieldRow
{
  //! The name in .xsp files and JSON filters
  std::string_view string;
  Field field;
  CDatabaseQueryRule::FieldType type;
  StringValidation::Validator validator;
  //! Whether the rule editor offers to browse for values
  bool browseable;
  //! What the field's values are browsed from
  BROWSE browse;
  int localizedString;
};

// clang-format off
const auto fields = std::array{
  FieldRow{ "none",              Field::NONE,                       TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        231 },
  FieldRow{ "filename",          Field::FILENAME,                   TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        561 },
  FieldRow{ "path",              Field::PATH,                       TEXT_FIELD,     nullptr,                              true,  BROWSE::PATHS,          573 },
  FieldRow{ "album",             Field::ALBUM,                      TEXT_FIELD,     nullptr,                              true,  BROWSE::ALBUMS,         558 },
  FieldRow{ "albumartist",       Field::ALBUM_ARTIST,               TEXT_FIELD,     nullptr,                              true,  BROWSE::ALBUM_ARTISTS,  566 },
  FieldRow{ "artist",            Field::ARTIST,                     TEXT_FIELD,     nullptr,                              true,  BROWSE::ARTISTS,        557 },
  FieldRow{ "tracknumber",       Field::TRACK_NUMBER,               NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, BROWSE::NOTHING,        554 },
  FieldRow{ "role",              Field::ROLE,                       TEXT_FIELD,     nullptr,                              true,  BROWSE::ROLES,          38033 },
  FieldRow{ "comment",           Field::COMMENT,                    TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        569 },
  FieldRow{ "review",            Field::REVIEW,                     TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        183 },
  FieldRow{ "themes",            Field::THEMES,                     TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        21895 },
  FieldRow{ "moods",             Field::MOODS,                      TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        175 },
  FieldRow{ "styles",            Field::STYLES,                     TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        176 },
  FieldRow{ "type",              Field::ALBUM_TYPE,                 TEXT_FIELD,     nullptr,                              false, BROWSE::ALBUM_TYPES,    564 },
  FieldRow{ "compilation",       Field::COMPILATION,                BOOLEAN_FIELD,  nullptr,                              false, BROWSE::NOTHING,        204 },
  FieldRow{ "label",             Field::MUSIC_LABEL,                TEXT_FIELD,     nullptr,                              false, BROWSE::MUSIC_LABELS,   21899 },
  FieldRow{ "title",             Field::TITLE,                      TEXT_FIELD,     nullptr,                              true,  BROWSE::TITLES,         556 },
  FieldRow{ "sorttitle",         Field::SORT_TITLE,                 TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        171 },
  FieldRow{ "originaltitle",     Field::ORIGINAL_TITLE,             TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        20376 },
  FieldRow{ "year",              Field::YEAR,                       NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  true,  BROWSE::YEARS,          562 },
  FieldRow{ "time",              Field::TIME,                       SECONDS_FIELD,  StringValidation::IsTime,             false, BROWSE::NOTHING,        180 },
  FieldRow{ "playcount",         Field::PLAYCOUNT,                  NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, BROWSE::NOTHING,        567 },
  FieldRow{ "lastplayed",        Field::LAST_PLAYED,                DATE_FIELD,     CLibraryQueryRule::ValidateDate,     false, BROWSE::NOTHING,        568 },
  FieldRow{ "inprogress",        Field::IN_PROGRESS,                BOOLEAN_FIELD,  nullptr,                              false, BROWSE::NOTHING,        575 },
  FieldRow{ "rating",            Field::RATING,                     REAL_FIELD,     ValidateRating,                       false, BROWSE::NOTHING,        563 },
  FieldRow{ "userrating",        Field::USER_RATING,                REAL_FIELD,     ValidateMyRating,                     false, BROWSE::NOTHING,        38018 },
  FieldRow{ "votes",             Field::VOTES,                      REAL_FIELD,     StringValidation::IsPositiveInteger,  false, BROWSE::NOTHING,        205 },
  FieldRow{ "top250",            Field::TOP250,                     NUMERIC_FIELD,  nullptr,                              false, BROWSE::NOTHING,        13409 },
  FieldRow{ "mpaarating",        Field::MPAA,                       TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        20074 },
  FieldRow{ "dateadded",         Field::DATE_ADDED,                 DATE_FIELD,     CLibraryQueryRule::ValidateDate,     false, BROWSE::NOTHING,        570 },
  FieldRow{ "datemodified",      Field::DATE_MODIFIED,              DATE_FIELD,     CLibraryQueryRule::ValidateDate,     false, BROWSE::NOTHING,        39119 },
  FieldRow{ "datenew",           Field::DATE_NEW,                   DATE_FIELD,     CLibraryQueryRule::ValidateDate,     false, BROWSE::NOTHING,        21877 },
  FieldRow{ "genre",             Field::GENRE,                      TEXT_FIELD,     nullptr,                              true,  BROWSE::GENRES,         515 },
  FieldRow{ "plot",              Field::PLOT,                       TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        207 },
  FieldRow{ "plotoutline",       Field::PLOT_OUTLINE,               TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        203 },
  FieldRow{ "tagline",           Field::TAGLINE,                    TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        202 },
  FieldRow{ "set",               Field::SET,                        TEXT_FIELD,     nullptr,                              true,  BROWSE::SETS,           20457 },
  FieldRow{ "director",          Field::DIRECTOR,                   TEXT_FIELD,     nullptr,                              true,  BROWSE::DIRECTORS,      20339 },
  FieldRow{ "actor",             Field::ACTOR,                      TEXT_FIELD,     nullptr,                              true,  BROWSE::ACTORS,         20337 },
  FieldRow{ "writers",           Field::WRITER,                     TEXT_FIELD,     nullptr,                              true,  BROWSE::WRITERS,        20417 },
  FieldRow{ "airdate",           Field::AIR_DATE,                   DATE_FIELD,     CLibraryQueryRule::ValidateDate,     false, BROWSE::NOTHING,        20416 },
  FieldRow{ "hastrailer",        Field::TRAILER,                    BOOLEAN_FIELD,  nullptr,                              false, BROWSE::NOTHING,        20423 },
  FieldRow{ "studio",            Field::STUDIO,                     TEXT_FIELD,     nullptr,                              true,  BROWSE::STUDIOS,        572 },
  FieldRow{ "country",           Field::COUNTRY,                    TEXT_FIELD,     nullptr,                              true,  BROWSE::COUNTRIES,      574 },
  FieldRow{ "tvshow",            Field::TVSHOW_TITLE,               TEXT_FIELD,     nullptr,                              true,  BROWSE::TVSHOWS,        20364 },
  FieldRow{ "status",            Field::TVSHOW_STATUS,              TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        126 },
  FieldRow{ "season",            Field::SEASON,                     NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, BROWSE::NOTHING,        20373 },
  FieldRow{ "episode",           Field::EPISODE_NUMBER,             NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, BROWSE::NOTHING,        20359 },
  FieldRow{ "numepisodes",       Field::NUMBER_OF_EPISODES,         REAL_FIELD,     StringValidation::IsPositiveInteger,  false, BROWSE::NOTHING,        20360 },
  FieldRow{ "numwatched",        Field::NUMBER_OF_WATCHED_EPISODES, REAL_FIELD,     StringValidation::IsPositiveInteger,  false, BROWSE::NOTHING,        21457 },
  FieldRow{ "videoresolution",   Field::VIDEO_RESOLUTION,           REAL_FIELD,     nullptr,                              false, BROWSE::NOTHING,        21443 },
  FieldRow{ "videocodec",        Field::VIDEO_CODEC,                TEXTIN_FIELD,   nullptr,                              false, BROWSE::NOTHING,        21445 },
  FieldRow{ "videoaspect",       Field::VIDEO_ASPECT_RATIO,         REAL_FIELD,     nullptr,                              false, BROWSE::NOTHING,        21374 },
  FieldRow{ "audiochannels",     Field::AUDIO_CHANNELS,             REAL_FIELD,     nullptr,                              false, BROWSE::NOTHING,        21444 },
  FieldRow{ "audiocodec",        Field::AUDIO_CODEC,                TEXTIN_FIELD,   nullptr,                              false, BROWSE::NOTHING,        21446 },
  FieldRow{ "audiolanguage",     Field::AUDIO_LANGUAGE,             TEXTIN_FIELD,   nullptr,                              false, BROWSE::NOTHING,        21447 },
  FieldRow{ "audiocount",        Field::AUDIO_COUNT,                REAL_FIELD,     StringValidation::IsPositiveInteger,  false, BROWSE::NOTHING,        21481 },
  FieldRow{ "subtitlecount",     Field::SUBTITLE_COUNT,             REAL_FIELD,     StringValidation::IsPositiveInteger,  false, BROWSE::NOTHING,        21482 },
  FieldRow{ "subtitlelanguage",  Field::SUBTITLE_LANGUAGE,          TEXTIN_FIELD,   nullptr,                              false, BROWSE::NOTHING,        21448 },
  FieldRow{ "random",            Field::RANDOM,                     TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        590 },
  FieldRow{ "playlist",          Field::PLAYLIST,                   TEXT_FIELD,     nullptr,                              true,  BROWSE::PLAYLISTS,      559 },
  FieldRow{ "virtualfolder",     Field::VIRTUAL_FOLDER,             TEXT_FIELD,     nullptr,                              true,  BROWSE::PLAYLISTS,      614 },
  FieldRow{ "tag",               Field::TAG,                        TEXT_FIELD,     nullptr,                              true,  BROWSE::TAGS,           20459 },
  FieldRow{ "instruments",       Field::INSTRUMENTS,                TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        21892 },
  FieldRow{ "biography",         Field::BIOGRAPHY,                  TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        21887 },
  FieldRow{ "born",              Field::BORN,                       TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        21893 },
  FieldRow{ "bandformed",        Field::BAND_FORMED,                TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        21894 },
  FieldRow{ "disbanded",         Field::DISBANDED,                  TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        21896 },
  FieldRow{ "died",              Field::DIED,                       TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        21897 },
  FieldRow{ "artisttype",        Field::ARTIST_TYPE,                TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        564 },
  FieldRow{ "gender",            Field::GENDER,                     TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        39025 },
  FieldRow{ "disambiguation",    Field::DISAMBIGUATION,             TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        39026 },
  FieldRow{ "source",            Field::SOURCE,                     TEXT_FIELD,     nullptr,                              true,  BROWSE::SOURCES,        39030 },
  FieldRow{ "disctitle",         Field::DISC_TITLE,                 TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        38076 },
  FieldRow{ "isboxset",          Field::IS_BOXSET,                  BOOLEAN_FIELD,  nullptr,                              false, BROWSE::NOTHING,        38074 },
  FieldRow{ "totaldiscs",        Field::TOTAL_DISCS,                NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, BROWSE::NOTHING,        38077 },
  FieldRow{ "originalyear",      Field::ORIG_YEAR,                  NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  true,  BROWSE::ORIGINAL_YEARS, 38078 },
  FieldRow{ "bpm",               Field::BPM,                        NUMERIC_FIELD,  nullptr,                              false, BROWSE::NOTHING,        38080 },
  FieldRow{ "samplerate",        Field::SAMPLE_RATE,                NUMERIC_FIELD,  nullptr,                              false, BROWSE::NOTHING,        613 },
  FieldRow{ "bitrate",           Field::MUSIC_BITRATE,              NUMERIC_FIELD,  nullptr,                              false, BROWSE::NOTHING,        623 },
  FieldRow{ "channels",          Field::NUMBER_OF_CHANNELS,         NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, BROWSE::NOTHING,        253 },
  FieldRow{ "albumstatus",       Field::ALBUM_STATUS,               TEXT_FIELD,     nullptr,                              false, BROWSE::NOTHING,        38081 },
  FieldRow{ "albumduration",     Field::ALBUM_DURATION,             SECONDS_FIELD,  StringValidation::IsTime,             false, BROWSE::NOTHING,        180 },
  FieldRow{ "hdrtype",           Field::HDR_TYPE,                   TEXTIN_FIELD,   nullptr,                              false, BROWSE::NOTHING,        20474 },
  FieldRow{ "hasversions",       Field::HAS_VIDEO_VERSIONS,         BOOLEAN_FIELD,  nullptr,                              false, BROWSE::NOTHING,        20475 },
  FieldRow{ "hasextras",         Field::HAS_VIDEO_EXTRAS,           BOOLEAN_FIELD,  nullptr,                              false, BROWSE::NOTHING,        20476 },
  FieldRow{ "hdrdetail",         Field::HDR_DETAIL,                 TEXTIN_FIELD,   nullptr,                              false, BROWSE::NOTHING,        20478 },
};
// clang-format on

std::vector<Field> ListFields(LIBRARY_QUERY::TYPE type)
{
  std::vector<Field> fields;
  bool isVideo = false;
  if (type == LIBRARY_QUERY::TYPE::MIXED)
  {
    // What songs and music videos both have, so that a rule can be asked of either. The video
    // library also writes an album artist and a track number for music videos, which their own
    // playlists do not offer.
    const std::vector<Field> musicVideos{ListFields(LIBRARY_QUERY::TYPE::MUSIC_VIDEOS)};
    std::ranges::copy_if(ListFields(LIBRARY_QUERY::TYPE::SONGS), std::back_inserter(fields),
                         [&musicVideos](Field field)
                         {
                           return field == Field::ALBUM_ARTIST || field == Field::TRACK_NUMBER ||
                                  std::ranges::find(musicVideos, field) != musicVideos.end();
                         });
    return fields;
  }
  else if (type == LIBRARY_QUERY::TYPE::SONGS)
  {
    fields = {
        Field::GENRE,  Field::SOURCE,       Field::ALBUM, Field::DISC_TITLE,
        Field::ARTIST, Field::ALBUM_ARTIST, Field::TITLE, Field::YEAR,
    };
    fields.insert(fields.end(), {
                                    Field::ORIG_YEAR,
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
  else if (type == LIBRARY_QUERY::TYPE::ALBUMS)
  {
    fields = {
        Field::GENRE,        Field::SOURCE,      Field::ALBUM,
        Field::DISC_TITLE,   Field::TOTAL_DISCS, Field::IS_BOXSET,
        Field::ARTIST, // any artist
        Field::ALBUM_ARTIST, // album artist
        Field::YEAR,
    };
    fields.insert(fields.end(), {
                                    Field::ORIG_YEAR,
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
  else if (type == LIBRARY_QUERY::TYPE::ARTISTS)
  {
    fields = {
        Field::ARTIST,     Field::SOURCE,         Field::GENRE,     Field::MOODS,
        Field::STYLES,     Field::INSTRUMENTS,    Field::BIOGRAPHY, Field::ARTIST_TYPE,
        Field::GENDER,     Field::DISAMBIGUATION, Field::BORN,      Field::BAND_FORMED,
        Field::DISBANDED,  Field::DIED,           Field::ROLE,      Field::PATH,
        Field::DATE_ADDED, Field::DATE_MODIFIED,  Field::DATE_NEW,
    };
  }
  else if (type == LIBRARY_QUERY::TYPE::TV_SHOWS)
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
  else if (type == LIBRARY_QUERY::TYPE::EPISODES)
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
  else if (type == LIBRARY_QUERY::TYPE::MOVIES)
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
  else if (type == LIBRARY_QUERY::TYPE::MUSIC_VIDEOS)
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
                                    Field::HDR_DETAIL,
                                });
  }
  fields.insert(fields.end(), {
                                  Field::PLAYLIST,
                                  Field::VIRTUAL_FOLDER,
                              });

  return fields;
}

const std::vector<Field>& FieldsOf(LIBRARY_QUERY::TYPE type)
{
  static const std::map<LIBRARY_QUERY::TYPE, std::vector<Field>> lists{
      []
      {
        std::map<LIBRARY_QUERY::TYPE, std::vector<Field>> all;
        for (const LIBRARY_QUERY::TYPE each : LIBRARY_QUERY::TYPES)
        {
          all.emplace(each, ListFields(each));
        }
        return all;
      }()};
  return lists.at(type);
}

//! Whether the music database holds the items of this type; the video database holds the others
bool InMusicDatabase(MEDIA::TYPE itemType)
{
  const std::optional<LIBRARY_QUERY::TYPE> type{LIBRARY_QUERY::TypeOf(itemType)};
  return type && LIBRARY_QUERY::ListsFrom(*type, MediaSection::MUSIC);
}
} // namespace

CLibraryQueryRule::CLibraryQueryRule() = default;

Field CLibraryQueryRule::TranslateField(const char* field) const
{
  const auto it = std::ranges::find_if(fields, [field](const auto& f)
                                       { return StringUtils::EqualsNoCase(field, f.string); });
  return it == fields.end() ? Field::NONE : it->field;
}

std::string CLibraryQueryRule::TranslateField(Field field) const
{
  return GetFieldName(field);
}

std::string CLibraryQueryRule::GetFieldName(Field field)
{
  const auto it = std::ranges::find_if(fields, [field](const auto& f) { return field == f.field; });
  return it == fields.end() ? "none" : std::string(it->string);
}

SortBy CLibraryQueryRule::TranslateOrder(const char* order)
{
  return SortUtils::SortMethodFromString(order);
}

std::string CLibraryQueryRule::TranslateOrder(SortBy order)
{
  std::string sortOrder = SortUtils::SortMethodToString(order);
  if (sortOrder.empty())
    return "none";

  return sortOrder;
}

int CLibraryQueryRule::GetFieldLabel(Field field)
{
  const auto it = std::ranges::find_if(fields, [field](const auto& f) { return field == f.field; });
  return it == fields.end() ? 16018 : it->localizedString;
}

std::string CLibraryQueryRule::GetLocalizedField(Field field)
{
  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(GetFieldLabel(field));
}

LIBRARY_QUERY::BROWSE CLibraryQueryRule::BrowseOf(Field field)
{
  const auto it = std::ranges::find_if(fields, [field](const auto& f) { return field == f.field; });
  return it == fields.end() ? BROWSE::NOTHING : it->browse;
}

CDatabaseQueryRule::FieldType CLibraryQueryRule::GetFieldType(Field field) const
{
  const auto it = std::ranges::find_if(fields, [field](const auto& f) { return field == f.field; });
  return it == fields.end() ? TEXT_FIELD : it->type;
}

bool CLibraryQueryRule::IsPlaylistField(Field field)
{
  return field == Field::PLAYLIST || field == Field::VIRTUAL_FOLDER;
}

bool CLibraryQueryRule::IsFieldBrowseable(Field field)
{
  const auto it = std::ranges::find_if(fields, [field](const auto& f) { return field == f.field; });
  return it == fields.end() ? false : it->browseable;
}

bool CLibraryQueryRule::Validate(const std::string& input, void* data)
{
  if (data == nullptr)
    return true;

  const auto* rule = static_cast<const CLibraryQueryRule*>(data);

  // check if there's a validator for this rule
  const auto it = std::ranges::find_if(fields, [field = rule->m_field](const auto& f)
                                       { return field == f.field; });
  if (it == fields.end())
    return true;

  if (!it->validator)
    return true;

  if (input.empty())
    return it->validator("", data);

  // Split the input into multiple values and validate every value separately
  const std::vector<std::string> values{StringUtils::Split(input, VALUE_SEPARATOR)};

  return std::ranges::all_of(values, [data, validator = it->validator](const auto& s)
                             { return validator(s, data); });
}

bool CLibraryQueryRule::ValidateDate(const std::string& input, void* data)
{
  if (!data)
    return false;

  const auto* rule = static_cast<const CLibraryQueryRule*>(data);

  //! @todo implement a validation for relative dates
  if (rule->m_operator == OPERATOR_IN_THE_LAST || rule->m_operator == OPERATOR_NOT_IN_THE_LAST)
    return true;

  // The date format must be YYYY-MM-DD
  CDateTime dt;
  return dt.SetFromRFC3339FullDate(input);
}

std::vector<Field> CLibraryQueryRule::GetFields(LIBRARY_QUERY::TYPE type)
{
  return FieldsOf(type);
}

bool CLibraryQueryRule::HasField(LIBRARY_QUERY::TYPE type, Field field)
{
  const std::vector<Field>& fields{FieldsOf(type)};
  return std::ranges::find(fields, field) != fields.end();
}

std::vector<SortBy> CLibraryQueryRule::GetOrders(LIBRARY_QUERY::TYPE type)
{
  std::vector<SortBy> orders;
  if (type == LIBRARY_QUERY::TYPE::MIXED)
  {
    orders = {
        SortBy::NONE,  SortBy::GENRE, SortBy::ALBUM,     SortBy::ARTIST,
        SortBy::TITLE, SortBy::YEAR,  SortBy::TIME,      SortBy::TRACK_NUMBER,
        SortBy::FILE,  SortBy::PATH,  SortBy::PLAYCOUNT, SortBy::LAST_PLAYED,
    };
  }
  else if (type == LIBRARY_QUERY::TYPE::SONGS)
  {
    orders = {
        SortBy::NONE, SortBy::GENRE, SortBy::ALBUM, SortBy::ARTIST, SortBy::TITLE, SortBy::YEAR,
    };
    orders.insert(orders.end(), {
                                    SortBy::ORIG_DATE,
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
  else if (type == LIBRARY_QUERY::TYPE::ALBUMS)
  {
    orders = {
        SortBy::NONE,   SortBy::GENRE, SortBy::ALBUM, SortBy::TOTAL_DISCS,
        SortBy::ARTIST, // any artist
        SortBy::YEAR,
    };
    orders.insert(orders.end(), {
                                    SortBy::ORIG_DATE,
                                    SortBy::ALBUM_TYPE,
                                    SortBy::RATING,
                                    SortBy::USER_RATING,
                                    SortBy::PLAYCOUNT,
                                    SortBy::LAST_PLAYED,
                                    SortBy::DATE_ADDED,
                                });
  }
  else if (type == LIBRARY_QUERY::TYPE::ARTISTS)
  {
    orders = {SortBy::NONE, SortBy::ARTIST};
  }
  else if (type == LIBRARY_QUERY::TYPE::TV_SHOWS)
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
  else if (type == LIBRARY_QUERY::TYPE::EPISODES)
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
  else if (type == LIBRARY_QUERY::TYPE::MOVIES)
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
  else if (type == LIBRARY_QUERY::TYPE::MUSIC_VIDEOS)
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

std::vector<LIBRARY_QUERY::GROUP> CLibraryQueryRule::GetGroups(LIBRARY_QUERY::TYPE type)
{
  using LIBRARY_QUERY::GROUP;
  std::vector<GROUP> groups;
  if (type == LIBRARY_QUERY::TYPE::ARTISTS)
  {
    groups = {
        GROUP::DEFAULT,
        GROUP::GENRE,
    };
  }
  else if (type == LIBRARY_QUERY::TYPE::ALBUMS)
  {
    groups = {
        GROUP::DEFAULT,
        GROUP::YEAR,
        GROUP::ORIGINAL_YEAR,
    };
  }
  if (type == LIBRARY_QUERY::TYPE::MOVIES)
  {
    groups = {
        GROUP::DEFAULT, GROUP::NONE,      GROUP::SET,      GROUP::GENRE,
        GROUP::YEAR,    GROUP::PERFORMER, GROUP::DIRECTOR, GROUP::WRITER,
        GROUP::STUDIO,  GROUP::COUNTRY,   GROUP::TAG,
    };
  }
  else if (type == LIBRARY_QUERY::TYPE::TV_SHOWS)
  {
    groups = {
        GROUP::DEFAULT,  GROUP::GENRE,  GROUP::YEAR, GROUP::PERFORMER,
        GROUP::DIRECTOR, GROUP::STUDIO, GROUP::TAG,
    };
  }
  else if (type == LIBRARY_QUERY::TYPE::MUSIC_VIDEOS)
  {
    groups = {
        GROUP::DEFAULT, GROUP::PERFORMER, GROUP::ALBUM,  GROUP::GENRE,
        GROUP::YEAR,    GROUP::DIRECTOR,  GROUP::STUDIO, GROUP::TAG,
    };
  }

  return groups;
}

std::string CLibraryQueryRule::GetLocalizedGroup(LIBRARY_QUERY::GROUP group,
                                                 LIBRARY_QUERY::TYPE type)
{
  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
      LIBRARY_QUERY::LabelOf(group, type));
}

std::string CLibraryQueryRule::GetLocalizedRule() const
{
  return StringUtils::Format("{} {} {}", GetLocalizedField(m_field),
                             GetLocalizedOperator(m_operator), GetParameter());
}

std::string CLibraryQueryRule::GetParameter() const
{
  return StringUtils::Join(m_parameter, VALUE_SEPARATOR);
}

void CLibraryQueryRule::SetParameter(const std::string& value)
{
  m_parameter = StringUtils::Split(value, VALUE_SEPARATOR);
}

std::optional<std::string> CLibraryQueryRule::GetWhereClause(
    const CDatabase& db, const CLibraryQueryContext& context) const
{
  if (!HasField(context.type, m_field))
  {
    CLog::LogF(LOGERROR, "Refusing the rule on field \"{}\": {} have no such field",
               GetFieldName(m_field), LIBRARY_QUERY::NameOf(context.type));
    return std::nullopt;
  }

  if (m_field != Field::PLAYLIST)
  {
    return CDatabaseQueryRuleOf::GetWhereClause(db, context);
  }

  const CLibraryQuery* playlist{context.references && !m_parameter.empty()
                                    ? context.references->Find(m_parameter.front())
                                    : nullptr};
  if (!playlist)
  {
    CLog::LogF(LOGERROR, "the playlist the rule \"playlist {}\" names was not resolved",
               GetParameter());
    return std::nullopt;
  }

  // A mixed playlist may name one of songs or of music videos, which holds nothing of the other
  const auto sources{LIBRARY_QUERY::SourcesOf(playlist->GetType())};
  if (std::ranges::find(sources, context.itemType, &LIBRARY_QUERY::Source::itemType) ==
      sources.end())
  {
    return m_operator == OPERATOR_DOES_NOT_EQUAL ? "1" : "0";
  }

  const std::optional<std::string> clause{
      playlist->GetWhereClause(db, context.itemType, context.useOriginalDate, *context.references)};
  if (!clause)
  {
    return std::nullopt;
  }
  // A playlist without rules holds everything
  const std::string rules{clause->empty() ? "1" : *clause};
  return m_operator == OPERATOR_DOES_NOT_EQUAL ? StringUtils::Format("NOT ({})", rules) : rules;
}

std::string CLibraryQueryRule::GetBooleanQuery(const std::string& negate,
                                               const CLibraryQueryContext& context) const
{
  return InMusicDatabase(context.itemType)
             ? MUSIC::LIBRARY_QUERY::BooleanClause(*this, context.itemType, negate)
             : VIDEO::LIBRARY_QUERY::BooleanClause(*this, context.itemType, negate);
}

CDatabaseQueryRule::SearchOperator CLibraryQueryRule::GetOperator(
    const CLibraryQueryContext& context) const
{
  return VIDEO::LIBRARY_QUERY::OperatorOf(*this, context.itemType);
}

std::optional<std::string> CLibraryQueryRule::FormatParameter(const std::string& operatorString,
                                                              const std::string& param,
                                                              const CDatabase& db) const
{
  // special-casing
  if (m_field == Field::TIME || m_field == Field::ALBUM_DURATION)
  { // translate time to seconds
    std::string seconds = std::to_string(StringUtils::TimeStringToSeconds(param));
    return db.PrepareSQL(operatorString, seconds.c_str());
  }
  return CDatabaseQueryRule::FormatParameter(operatorString, param, db);
}

std::string CLibraryQueryRule::FormatNullableDate(const std::string& field,
                                                  CDatabaseQueryRule::SearchOperator oper,
                                                  const std::string& parameter)
{
  if (oper == OPERATOR_LESS_THAN || oper == OPERATOR_BEFORE || oper == OPERATOR_NOT_IN_THE_LAST)
    return field + " IS NULL OR " + field + parameter;
  else
    return field + " IS NOT NULL AND " + field + parameter;
}

std::string CLibraryQueryRule::FormatNullableNumber(const std::string& field,
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

std::optional<std::string> CLibraryQueryRule::FormatWhereClause(
    const std::string& negate,
    const std::string& oper,
    const std::string& param,
    const CDatabase& db,
    const CLibraryQueryContext& context) const
{
  const std::optional<std::string> formatted{FormatParameter(oper, param, db)};
  if (!formatted)
  {
    return std::nullopt;
  }
  const std::string& parameter{*formatted};
  const MEDIA::TYPE type{context.itemType};

  const std::string query{
      InMusicDatabase(type)
          ? MUSIC::LIBRARY_QUERY::RuleClause(*this, type, negate, param, parameter,
                                             context.useOriginalDate)
          : VIDEO::LIBRARY_QUERY::RuleClause(*this, type, negate, param, parameter, db)};
  if (query.empty())
  {
    return CDatabaseQueryRuleOf::FormatWhereClause(negate, oper, param, db, context);
  }

  return query;
}

std::string CLibraryQueryRule::GetField(Field field, const CLibraryQueryContext& context) const
{
  return DatabaseUtils::GetField(field, context.itemType, DatabaseQueryPart::WHERE);
}

} // namespace KODI::MEDIA

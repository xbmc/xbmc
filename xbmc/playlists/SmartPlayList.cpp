/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <cstdlib>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "SmartPlayList.h"
#include "Util.h"
#include "dbwrappers/Database.h"
#include "filesystem/File.h"
#include "filesystem/SmartPlaylistDirectory.h"
#include "guilib/LocalizeStrings.h"
#include "utils/DatabaseUtils.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/log.h"
#include "utils/StreamDetails.h"
#include "utils/StringUtils.h"
#include "utils/StringValidation.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"

using namespace XFILE;

typedef struct
{
  char string[17];
  Field field;
  CDatabaseQueryRule::FIELD_TYPE type;
  StringValidation::Validator validator;
  bool browseable;
  int localizedString;
} translateField;

static const translateField fields[] = {
  { "none",              FieldNone,                    CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 231 },
  { "filename",          FieldFilename,                CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 561 },
  { "path",              FieldPath,                    CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  573 },
  { "album",             FieldAlbum,                   CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  558 },
  { "albumartist",       FieldAlbumArtist,             CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  566 },
  { "artist",            FieldArtist,                  CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  557 },
  { "tracknumber",       FieldTrackNumber,             CDatabaseQueryRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 554 },
  { "role",              FieldRole,                    CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true, 38033 },
  { "comment",           FieldComment,                 CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 569 },
  { "review",            FieldReview,                  CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 183 },
  { "themes",            FieldThemes,                  CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 21895 },
  { "moods",             FieldMoods,                   CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 175 },
  { "styles",            FieldStyles,                  CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 176 },
  { "type",              FieldAlbumType,               CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 564 },
  { "compilation",       FieldCompilation,             CDatabaseQueryRule::BOOLEAN_FIELD,  NULL,                                 false, 204 },
  { "label",             FieldMusicLabel,              CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 21899 },
  { "title",             FieldTitle,                   CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  556 },
  { "sorttitle",         FieldSortTitle,               CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 171 },
  { "year",              FieldYear,                    CDatabaseQueryRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  true,  562 },
  { "time",              FieldTime,                    CDatabaseQueryRule::SECONDS_FIELD,  StringValidation::IsTime,             false, 180 },
  { "playcount",         FieldPlaycount,               CDatabaseQueryRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 567 },
  { "lastplayed",        FieldLastPlayed,              CDatabaseQueryRule::DATE_FIELD,     NULL,                                 false, 568 },
  { "inprogress",        FieldInProgress,              CDatabaseQueryRule::BOOLEAN_FIELD,  NULL,                                 false, 575 },
  { "rating",            FieldRating,                  CDatabaseQueryRule::REAL_FIELD,     CSmartPlaylistRule::ValidateRating,   false, 563 },
  { "userrating",        FieldUserRating,              CDatabaseQueryRule::REAL_FIELD,     CSmartPlaylistRule::ValidateMyRating, false, 38018 },
  { "votes",             FieldVotes,                   CDatabaseQueryRule::REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 205 },
  { "top250",            FieldTop250,                  CDatabaseQueryRule::NUMERIC_FIELD,  NULL,                                 false, 13409 },
  { "mpaarating",        FieldMPAA,                    CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 20074 },
  { "dateadded",         FieldDateAdded,               CDatabaseQueryRule::DATE_FIELD,     NULL,                                 false, 570 },
  { "genre",             FieldGenre,                   CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  515 },
  { "plot",              FieldPlot,                    CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 207 },
  { "plotoutline",       FieldPlotOutline,             CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 203 },
  { "tagline",           FieldTagline,                 CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 202 },
  { "set",               FieldSet,                     CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  20457 },
  { "director",          FieldDirector,                CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  20339 },
  { "actor",             FieldActor,                   CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  20337 },
  { "writers",           FieldWriter,                  CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  20417 },
  { "airdate",           FieldAirDate,                 CDatabaseQueryRule::DATE_FIELD,     NULL,                                 false, 20416 },
  { "hastrailer",        FieldTrailer,                 CDatabaseQueryRule::BOOLEAN_FIELD,  NULL,                                 false, 20423 },
  { "studio",            FieldStudio,                  CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  572 },
  { "country",           FieldCountry,                 CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  574 },
  { "tvshow",            FieldTvShowTitle,             CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  20364 },
  { "status",            FieldTvShowStatus,            CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 126 },
  { "season",            FieldSeason,                  CDatabaseQueryRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 20373 },
  { "episode",           FieldEpisodeNumber,           CDatabaseQueryRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 20359 },
  { "numepisodes",       FieldNumberOfEpisodes,        CDatabaseQueryRule::REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 20360 },
  { "numwatched",        FieldNumberOfWatchedEpisodes, CDatabaseQueryRule::REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 21457 },
  { "videoresolution",   FieldVideoResolution,         CDatabaseQueryRule::REAL_FIELD,     NULL,                                 false, 21443 },
  { "videocodec",        FieldVideoCodec,              CDatabaseQueryRule::TEXTIN_FIELD,   NULL,                                 false, 21445 },
  { "videoaspect",       FieldVideoAspectRatio,        CDatabaseQueryRule::REAL_FIELD,     NULL,                                 false, 21374 },
  { "audiochannels",     FieldAudioChannels,           CDatabaseQueryRule::REAL_FIELD,     NULL,                                 false, 21444 },
  { "audiocodec",        FieldAudioCodec,              CDatabaseQueryRule::TEXTIN_FIELD,   NULL,                                 false, 21446 },
  { "audiolanguage",     FieldAudioLanguage,           CDatabaseQueryRule::TEXTIN_FIELD,   NULL,                                 false, 21447 },
  { "audiocount",        FieldAudioCount,              CDatabaseQueryRule::REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 21481 },
  { "subtitlecount",     FieldSubtitleCount,           CDatabaseQueryRule::REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 21482 },
  { "subtitlelanguage",  FieldSubtitleLanguage,        CDatabaseQueryRule::TEXTIN_FIELD,   NULL,                                 false, 21448 },
  { "random",            FieldRandom,                  CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 590 },
  { "playlist",          FieldPlaylist,                CDatabaseQueryRule::PLAYLIST_FIELD, NULL,                                 true,  559 },
  { "virtualfolder",     FieldVirtualFolder,           CDatabaseQueryRule::PLAYLIST_FIELD, NULL,                                 true,  614 },
  { "tag",               FieldTag,                     CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  20459 },
  { "instruments",       FieldInstruments,             CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 21892 },
  { "biography",         FieldBiography,               CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 21887 },
  { "born",              FieldBorn,                    CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 21893 },
  { "bandformed",        FieldBandFormed,              CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 21894 },
  { "disbanded",         FieldDisbanded,               CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 21896 },
  { "died",              FieldDied,                    CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 21897 }
};

static const size_t NUM_FIELDS = sizeof(fields) / sizeof(translateField);

typedef struct
{
  std::string name;
  Field field;
  bool canMix;
  int localizedString;
} group;

static const group groups[] = { { "",           FieldUnknown,   false,    571 },
                                { "none",       FieldNone,      false,    231 },
                                { "sets",       FieldSet,       true,   20434 },
                                { "genres",     FieldGenre,     false,    135 },
                                { "years",      FieldYear,      false,    652 },
                                { "actors",     FieldActor,     false,    344 },
                                { "directors",  FieldDirector,  false,  20348 },
                                { "writers",    FieldWriter,    false,  20418 },
                                { "studios",    FieldStudio,    false,  20388 },
                                { "countries",  FieldCountry,   false,  20451 },
                                { "artists",    FieldArtist,    false,    133 },
                                { "albums",     FieldAlbum,     false,    132 },
                                { "tags",       FieldTag,       false,  20459 },
                              };

static const size_t NUM_GROUPS = sizeof(groups) / sizeof(group);

#define RULE_VALUE_SEPARATOR  " / "

CSmartPlaylistRule::CSmartPlaylistRule()
{
}

int CSmartPlaylistRule::TranslateField(const char *field) const
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (StringUtils::EqualsNoCase(field, fields[i].string)) return fields[i].field;
  return FieldNone;
}

std::string CSmartPlaylistRule::TranslateField(int field) const
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return fields[i].string;
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
  for (unsigned int i = 0; i < NUM_GROUPS; i++)
  {
    if (StringUtils::EqualsNoCase(group, groups[i].name))
      return groups[i].field;
  }

  return FieldUnknown;
}

std::string CSmartPlaylistRule::TranslateGroup(Field group)
{
  for (unsigned int i = 0; i < NUM_GROUPS; i++)
  {
    if (group == groups[i].field)
      return groups[i].name;
  }

  return "";
}

std::string CSmartPlaylistRule::GetLocalizedField(int field)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return g_localizeStrings.Get(fields[i].localizedString);
  return g_localizeStrings.Get(16018);
}

CDatabaseQueryRule::FIELD_TYPE CSmartPlaylistRule::GetFieldType(int field) const
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return fields[i].type;
  return TEXT_FIELD;
}

bool CSmartPlaylistRule::IsFieldBrowseable(int field)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return fields[i].browseable;

  return false;
}

bool CSmartPlaylistRule::Validate(const std::string &input, void *data)
{
  if (data == NULL)
    return true;

  CSmartPlaylistRule *rule = (CSmartPlaylistRule*)data;

  // check if there's a validator for this rule
  StringValidation::Validator validator = NULL;
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
  {
    if (rule->m_field == fields[i].field)
    {
        validator = fields[i].validator;
        break;
    }
  }
  if (validator == NULL)
    return true;

  // split the input into multiple values and validate every value separately
  std::vector<std::string> values = StringUtils::Split(input, RULE_VALUE_SEPARATOR);
  for (std::vector<std::string>::const_iterator it = values.begin(); it != values.end(); ++it)
  {
    if (!validator(*it, data))
      return false;
  }

  return true;
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
    fields.push_back(FieldAlbum);
    fields.push_back(FieldArtist);
    fields.push_back(FieldAlbumArtist);
    fields.push_back(FieldTitle);
    fields.push_back(FieldYear);
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
  }
  else if (type == "albums")
  {
    fields.push_back(FieldGenre);
    fields.push_back(FieldAlbum);
    fields.push_back(FieldArtist);        // any artist
    fields.push_back(FieldAlbumArtist);  // album artist
    fields.push_back(FieldYear);
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
  }
  else if (type == "artists")
  {
    fields.push_back(FieldArtist);
    fields.push_back(FieldGenre);
    fields.push_back(FieldMoods);
    fields.push_back(FieldStyles);
    fields.push_back(FieldInstruments);
    fields.push_back(FieldBiography);
    fields.push_back(FieldBorn);
    fields.push_back(FieldBandFormed);
    fields.push_back(FieldDisbanded);
    fields.push_back(FieldDied);
    fields.push_back(FieldRole);
    fields.push_back(FieldPath);
  }
  else if (type == "tvshows")
  {
    fields.push_back(FieldTitle);
    fields.push_back(FieldPlot);
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
  }
  else if (type == "episodes")
  {
    fields.push_back(FieldTitle);
    fields.push_back(FieldTvShowTitle);
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
    orders.push_back(SortByTime);
    orders.push_back(SortByTrackNumber);
    orders.push_back(SortByFile);
    orders.push_back(SortByPath);
    orders.push_back(SortByPlaycount);
    orders.push_back(SortByLastPlayed);
    orders.push_back(SortByRating);
    orders.push_back(SortByUserRating);
  }
  else if (type == "albums")
  {
    orders.push_back(SortByGenre);
    orders.push_back(SortByAlbum);
    orders.push_back(SortByArtist);        // any artist
    orders.push_back(SortByYear);
    //orders.push_back(SortByThemes);
    //orders.push_back(SortByMoods);
    //orders.push_back(SortByStyles);
    orders.push_back(SortByAlbumType);
    //orders.push_back(SortByMusicLabel);
    orders.push_back(SortByRating);
    orders.push_back(SortByUserRating);
    orders.push_back(SortByPlaycount);
    orders.push_back(SortByLastPlayed);
  }
  else if (type == "artists")
  {
    orders.push_back(SortByArtist);
  }
  else if (type == "tvshows")
  {
    orders.push_back(SortBySortTitle);
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
    groups.push_back(FieldYear);
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
  for (unsigned int i = 0; i < NUM_GROUPS; i++)
  {
    if (group == groups[i].field)
      return g_localizeStrings.Get(groups[i].localizedString);
  }

  return g_localizeStrings.Get(groups[0].localizedString);
}

bool CSmartPlaylistRule::CanGroupMix(Field group)
{
  for (unsigned int i = 0; i < NUM_GROUPS; i++)
  {
    if (group == groups[i].field)
      return groups[i].canMix;
  }

  return false;
}

std::string CSmartPlaylistRule::GetLocalizedRule() const
{
  return StringUtils::Format("%s %s %s", GetLocalizedField(m_field).c_str(), GetLocalizedOperator(m_operator).c_str(), GetParameter().c_str());
}

odb::query<ODBView_Movie> CSmartPlaylistRule::GetODBVideoResolutionQuery(const std::string &parameter) const
{
  typedef odb::query<ODBView_Movie> query;
  query retVal;
  int iRes = (int)std::strtol(parameter.c_str(), NULL, 10);
  
  int min, max;
  if (iRes >= 1080)     { min = 1281; max = INT_MAX; }
  else if (iRes >= 720) { min =  961; max = 1280; }
  else if (iRes >= 540) { min =  721; max =  960; }
  else                  { min =    0; max =  720; }
  
  switch (m_operator)
  {
    case OPERATOR_EQUALS:
      retVal = query::CODBStreamDetails::videoWidth >= min && query::CODBStreamDetails::videoWidth <= max;
      break;
    case OPERATOR_DOES_NOT_EQUAL:
      retVal = query::CODBStreamDetails::videoWidth < min || query::CODBStreamDetails::videoWidth > max;
      break;
    case OPERATOR_LESS_THAN:
      retVal = query::CODBStreamDetails::videoWidth < min;
      break;
    case OPERATOR_GREATER_THAN:
      retVal = query::CODBStreamDetails::videoWidth > max;
      break;
    default:
      break;
  }
  
  return retVal;
}

std::string CSmartPlaylistRule::GetVideoResolutionQuery(const std::string &parameter) const
{
  std::string retVal(" IN (SELECT DISTINCT idFile FROM streamdetails WHERE iVideoWidth ");
  int iRes = (int)std::strtol(parameter.c_str(), NULL, 10);

  int min, max;
  if (iRes >= 1080)     { min = 1281; max = INT_MAX; }
  else if (iRes >= 720) { min =  961; max = 1280; }
  else if (iRes >= 540) { min =  721; max =  960; }
  else                  { min =    0; max =  720; }

  switch (m_operator)
  {
    case OPERATOR_EQUALS:
      retVal += StringUtils::Format(">= %i AND iVideoWidth <= %i", min, max);
      break;
    case OPERATOR_DOES_NOT_EQUAL:
      retVal += StringUtils::Format("< %i OR iVideoWidth > %i", min, max);
      break;
    case OPERATOR_LESS_THAN:
      retVal += StringUtils::Format("< %i", min);
      break;
    case OPERATOR_GREATER_THAN:
      retVal += StringUtils::Format("> %i", max);
      break;
    default:
      break;
  }

  retVal += ")";
  return retVal;
}

odb::query<ODBView_Movie> CSmartPlaylistRule::GetMovieBooleanQuery(const bool &negate, const std::string &strType)
{
  //TODO: Check why this is not called
  typedef odb::query<ODBView_Movie> query;
  
  if (m_field == FieldInProgress)
  {
    if (negate)
      return query( query::CODBMovie::resumeBookmark.is_not_null() );
    else
      return query( query::CODBMovie::resumeBookmark.is_null() );
  }
  else if (m_field == FieldTrailer)
  {
    if (negate)
      return query( query::CODBMovie::trailer == "" || query::CODBMovie::trailer.is_null());
    else
      return query( query::CODBMovie::trailer != "" && query::CODBMovie::trailer.is_not_null());
  }
  
  return query();
}

odb::query<ODBView_TVShow> CSmartPlaylistRule::GetTVShowBooleanQuery(const bool &negate, const std::string &strType)
{
  //TODO: Check why this is not called
  typedef odb::query<ODBView_TVShow> query;

  //TODO: Check why this is empty
  
  return query();
}

odb::query<ODBView_Episode> CSmartPlaylistRule::GetEpisodeBooleanQuery(const bool &negate, const std::string &strType)
{
  //TODO: Check why this is not called
  typedef odb::query<ODBView_Episode> query;
  
  if (m_field == FieldInProgress)
  {
    if (negate)
      return query( query::CODBEpisode::resumeBookmark.is_not_null() );
    else
      return query( query::CODBEpisode::resumeBookmark.is_null() );
  }
  
  return query();
}

odb::query<ODBView_Song_Artists> CSmartPlaylistRule::GetArtistBooleanQuery(const bool &negate,
                         const std::string &strType)
{
  CLog::Log(LOGDEBUG, "%s - boolean query: %i", __FUNCTION__, m_field);
  return odb::query<ODBView_Song_Artists>();
}


odb::query<ODBView_Album> CSmartPlaylistRule::GetAlbumBooleanQuery(const bool &negate,
                                                                   const std::string &strType)
{
  CLog::Log(LOGDEBUG, "%s - boolean query: %i", __FUNCTION__, m_field);
  return odb::query<ODBView_Album>();
}

odb::query<ODBView_Song> CSmartPlaylistRule::GetSongBooleanQuery(const bool &negate,
                                                                 const std::string &strType)
{
  CLog::Log(LOGDEBUG, "%s - boolean query: %i", __FUNCTION__, m_field);
  return odb::query<ODBView_Song>();
}

std::string CSmartPlaylistRule::GetBooleanQuery(const std::string &negate, const std::string &strType) const
{
  if (strType == "movies")
  {
    if (m_field == FieldInProgress)
      return "movie_view.idFile " + negate + " IN (SELECT DISTINCT idFile FROM bookmark WHERE type = 1)";
    else if (m_field == FieldTrailer)
      return negate + GetField(m_field, strType) + "!= ''";
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
  }
  if (strType == "albums")
  {
    if (m_field == FieldCompilation)
      return negate + GetField(m_field, strType);
  }
  return "";
}

CDatabaseQueryRule::SEARCH_OPERATOR CSmartPlaylistRule::GetOperator(const std::string &strType) const
{
  SEARCH_OPERATOR op = CDatabaseQueryRule::GetOperator(strType);
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
  if (m_field == FieldTime)
  { // translate time to seconds
    std::string seconds = StringUtils::Format("%li", StringUtils::TimeStringToSeconds(param));
    return db.PrepareSQL(operatorString.c_str(), seconds.c_str());
  }
  return CDatabaseQueryRule::FormatParameter(operatorString, param, db, strType);
}

std::string CSmartPlaylistRule::FormatLinkQuery(const char *field, const char *table, const MediaType& mediaType, const std::string& mediaField, const std::string& parameter)
{
  // NOTE: no need for a PrepareSQL here, as the parameter has already been formatted
  return StringUtils::Format(" EXISTS (SELECT 1 FROM %s_link"
                             "         JOIN %s ON %s.%s_id=%s_link.%s_id"
                             "         WHERE %s_link.media_id=%s AND %s.name %s AND %s_link.media_type = '%s')",
                             field, table, table, table, field, table, field, mediaField.c_str(), table, parameter.c_str(), field, mediaType.c_str());
}

odb::query<ODBView_Movie> CSmartPlaylistRule::FormatMovieWhereBetweenClause(const bool &negate,
                                                                     const CDatabaseQueryRule::SEARCH_OPERATOR &oper,
                                                                     const std::string &param1,
                                                                     const std::string &param2,
                                                                     const std::string &strType) const
{
  typedef odb::query<ODBView_Movie> query;
  query where_query;
  
  CLog::Log(LOGDEBUG, "%s - between param: %s | %s- type: %s - operator: %i", __FUNCTION__, param1.c_str(), param2.c_str(), strType.c_str(), oper);
  
  if (m_field == FieldRating)
  {
    where_query = (query::defaultRating::rating >= std::stof(param1) &&
                   query::defaultRating::rating <= std::stof(param2));
  }
  else if (m_field == FieldYear)
  {
    where_query = (query::CODBMovie::premiered.year >= std::stoi(param1) &&
                   query::CODBMovie::premiered.year <= std::stoi(param2));
  }
  else if (m_field == FieldUserRating)
  {
    where_query = (query::CODBMovie::userrating >= std::stoi(param1) &&
                   query::CODBMovie::userrating <= std::stoi(param2));
  }
  
  if (negate)
    return !where_query;
  else
    return where_query;
}

odb::query<ODBView_TVShow> CSmartPlaylistRule::FormatTVShowWhereBetweenClause(const bool &negate,
                                                                              const CDatabaseQueryRule::SEARCH_OPERATOR &oper,
                                                                              const std::string &param1,
                                                                              const std::string &param2,
                                                                              const std::string &strType) const
{
  typedef odb::query<ODBView_TVShow> query;
  query where_query;
  
  CLog::Log(LOGDEBUG, "%s - between param: %s | %s- type: %s - operator: %i", __FUNCTION__, param1.c_str(), param2.c_str(), strType.c_str(), oper);
  
  if (m_field == FieldRating)
  {
    where_query = (query::defaultRating::rating >= std::stof(param1) &&
                   query::defaultRating::rating <= std::stof(param2));
  }
  else if (m_field == FieldYear)
  {
    where_query = (query::CODBTVShow::premiered.year >= std::stoi(param1) &&
                   query::CODBTVShow::premiered.year <= std::stoi(param2));
  }
  else if (m_field == FieldUserRating)
  {
    where_query = (query::CODBTVShow::userrating >= std::stoi(param1) &&
                   query::CODBTVShow::userrating <= std::stoi(param2));
  }
  
  if (negate)
    return !where_query;
  else
    return where_query;
}

odb::query<ODBView_Episode> CSmartPlaylistRule::FormatEpisodeWhereBetweenClause(const bool &negate,
                                                                                const CDatabaseQueryRule::SEARCH_OPERATOR &oper,
                                                                                const std::string &param1,
                                                                                const std::string &param2,
                                                                                const std::string &strType) const
{
  typedef odb::query<ODBView_Episode> query;
  query where_query;
  
  CLog::Log(LOGDEBUG, "%s - between param: %s | %s- type: %s - operator: %i", __FUNCTION__, param1.c_str(), param2.c_str(), strType.c_str(), oper);
  
  if (m_field == FieldRating)
  {
    where_query = (query::defaultRating::rating >= std::stof(param1) &&
                   query::defaultRating::rating <= std::stof(param2));
  }
  else if (m_field == FieldYear)
  {
    where_query = (query::CODBEpisode::aired.year >= std::stoi(param1) &&
                   query::CODBEpisode::aired.year <= std::stoi(param2));
  }
  else if (m_field == FieldUserRating)
  {
    where_query = (query::CODBTVShow::userrating >= std::stoi(param1) &&
                   query::CODBTVShow::userrating <= std::stoi(param2));
  }
  
  if (negate)
    return !where_query;
  else
    return where_query;
}

odb::query<ODBView_Song_Artists> CSmartPlaylistRule::FormatArtistWhereBetweenClause(const bool &negate,
                                  const CDatabaseQueryRule::SEARCH_OPERATOR &oper,
                                  const std::string &param1,
                                  const std::string &param2,
                                  const std::string &strType) const
{
  CLog::Log(LOGDEBUG, "%s - between param: %s | %s- type: %s - operator: %i", __FUNCTION__, param1.c_str(), param2.c_str(), strType.c_str(), oper);
  
  return odb::query<ODBView_Song_Artists>();
}

odb::query<ODBView_Album> CSmartPlaylistRule::FormatAlbumWhereBetweenClause(const bool &negate,
                                                                            const CDatabaseQueryRule::SEARCH_OPERATOR &oper,
                                                                            const std::string &param1,
                                                                            const std::string &param2,
                                                                            const std::string &strType) const
{
  typedef odb::query<ODBView_Album> query;
  query where_query;
  
  CLog::Log(LOGDEBUG, "%s - between param: %s | %s- type: %s - operator: %i", __FUNCTION__, param1.c_str(), param2.c_str(), strType.c_str(), oper);
  
  if (m_field == FieldYear)
  {
    where_query = (query::CODBAlbum::year >= std::stoi(param1) &&
                   query::CODBAlbum::year <= std::stoi(param2));
  }
  
  if (negate)
    return !where_query;
  else
    return where_query;
}

odb::query<ODBView_Song> CSmartPlaylistRule::FormatSongWhereBetweenClause(const bool &negate,
                                                                          const CDatabaseQueryRule::SEARCH_OPERATOR &oper,
                                                                          const std::string &param1,
                                                                          const std::string &param2,
                                                                          const std::string &strType) const
{
  typedef odb::query<ODBView_Song> query;
  query where_query;
  
  CLog::Log(LOGDEBUG, "%s - between param: %s | %s- type: %s - operator: %i", __FUNCTION__, param1.c_str(), param2.c_str(), strType.c_str(), oper);
  
  if (m_field == FieldYear)
  {
    where_query = (query::CODBSong::year >= std::stoi(param1) &&
                   query::CODBSong::year <= std::stoi(param2));
  }
  
  if (negate)
    return !where_query;
  else
    return where_query;
}

std::string CSmartPlaylistRule::FormatODBString(const CDatabaseQueryRule::SEARCH_OPERATOR &oper,
                                                const std::string& param) const
{
  switch (oper) {
    case OPERATOR_CONTAINS:
    case OPERATOR_DOES_NOT_CONTAIN:
      return "%"+param+"%";
      
    case OPERATOR_STARTS_WITH:
      return param+"%";
    
    case OPERATOR_ENDS_WITH:
      return "%"+param;
      
    default:
      return param;
  }
}

/**
 *  T Return Type, normally odb::query<>
 *  U Value Type
 *  V Paramterter Type
 */
template<typename T, typename U, typename V> T CSmartPlaylistRule::FormatODBParam(const U& val,
                                     const CDatabaseQueryRule::SEARCH_OPERATOR &oper,
                                     const V& param) const
{
  /*
   OPERATOR_START = 0,
   OPERATOR_CONTAINS,
   OPERATOR_DOES_NOT_CONTAIN,
   OPERATOR_EQUALS,
   OPERATOR_DOES_NOT_EQUAL,
   OPERATOR_STARTS_WITH,
   OPERATOR_ENDS_WITH,
   OPERATOR_GREATER_THAN,
   OPERATOR_LESS_THAN,
   OPERATOR_AFTER,
   OPERATOR_BEFORE,
   OPERATOR_IN_THE_LAST,
   OPERATOR_NOT_IN_THE_LAST,
   OPERATOR_TRUE,
   OPERATOR_FALSE,
   OPERATOR_BETWEEN,
   OPERATOR_END
   
   enum FIELD_TYPE { TEXT_FIELD = 0,
   REAL_FIELD,
   NUMERIC_FIELD,
   DATE_FIELD,
   PLAYLIST_FIELD,
   SECONDS_FIELD,
   BOOLEAN_FIELD,
   TEXTIN_FIELD
   };
   
   */
  CDatabaseQueryRule::FIELD_TYPE field_type = GetFieldType(m_field);
  
  switch (oper) {
    case OPERATOR_CONTAINS:
      if (field_type == TEXT_FIELD
          || field_type == TEXTIN_FIELD)
      {
        return T(val.like(param));
      }
      else if (field_type == REAL_FIELD
               || field_type == NUMERIC_FIELD
               || field_type == DATE_FIELD
               || field_type == SECONDS_FIELD)
      {
        return T(val == param);
      }
      break;
      
    case OPERATOR_DOES_NOT_CONTAIN:
      if (field_type == TEXT_FIELD
          || field_type == TEXTIN_FIELD)
      {
        return T(!val.like(param));
      }
      else if (field_type == REAL_FIELD
               || field_type == NUMERIC_FIELD
               || field_type == DATE_FIELD
               || field_type == SECONDS_FIELD)
      {
        return T(val != param);
      }
      break;
      
    case OPERATOR_EQUALS:
      return T(val == param);
      break;
      
    case OPERATOR_DOES_NOT_EQUAL:
      return T(val != param);
      break;
      
    case OPERATOR_STARTS_WITH:
      if (field_type == TEXT_FIELD
          || field_type == TEXTIN_FIELD)
      {
        return T(val.like(param));
      }
      break;
      
    case OPERATOR_ENDS_WITH:
      if (field_type == TEXT_FIELD
          || field_type == TEXTIN_FIELD)
      {
        return T(val.like(param));
      }
      break;
      
    case OPERATOR_GREATER_THAN:
    case OPERATOR_AFTER:
      if (field_type == REAL_FIELD
          || field_type == NUMERIC_FIELD
          || field_type == DATE_FIELD
          || field_type == SECONDS_FIELD)
      {
        return T(val > param);
      }
      break;
      
    case OPERATOR_LESS_THAN:
    case OPERATOR_BEFORE:
      if (field_type == REAL_FIELD
          || field_type == NUMERIC_FIELD
          || field_type == DATE_FIELD
          || field_type == SECONDS_FIELD)
      {
        return T(val < param);
      }
      break;
      
    default:
      return T();
      break;
  }
  
  return T();
}

odb::query<ODBView_Movie> CSmartPlaylistRule::FormatMovieWhereClause(const bool &negate,
                                                         const CDatabaseQueryRule::SEARCH_OPERATOR &oper,
                                                         const std::string &param,
                                                         const std::string &strType) const
{
  typedef odb::query<ODBView_Movie> query;
  query where_query;

  CLog::Log(LOGDEBUG, "%s - param: %s - type: %s - operator: %i", __FUNCTION__, param.c_str(), strType.c_str(), oper);
  
  if (m_field == FieldTitle)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBMovie::title_type_, std::string>(query::CODBMovie::title, oper, prepared_string);
  }
  else if (m_field == FieldGenre)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::genre::name_type_, std::string>(query::genre::name, oper, prepared_string);
  }
  else if (m_field == FieldYear)
  {
    where_query = FormatODBParam<query, query::CODBMovie::premiered_class_::year_type_, int>(query::CODBMovie::premiered.year, oper, std::stoi(param));
  }
  else if (m_field == FieldTime)
  {
    int seconds = std::stoi(StringUtils::Format("%li", StringUtils::TimeStringToSeconds(param)));
    where_query = FormatODBParam<query, query::CODBMovie::runtime_type_, int>(query::CODBMovie::runtime, oper, seconds);
  }
  else if (m_field == FieldFilename)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::fileView::filename_type_, std::string>(query::fileView::filename, oper, prepared_string);
  }
  else if (m_field == FieldPath)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::pathView::path_type_, std::string>(query::pathView::path, oper, prepared_string);
  }
  else if (m_field == FieldRating)
  {
    where_query = FormatODBParam<query, query::defaultRating::rating_type_, float>(query::defaultRating::rating, oper, std::stof(param));
  }
  else if (m_field == FieldPlot)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBMovie::plot_type_, std::string>(query::CODBMovie::plot, oper, prepared_string);
  }
  else if (m_field == FieldPlotOutline)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBMovie::plotoutline_type_, std::string>(query::CODBMovie::plotoutline, oper, prepared_string);
  }
  else if (m_field == FieldTagline)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBMovie::tagline_type_, std::string>(query::CODBMovie::tagline, oper, prepared_string);
  }
  else if (m_field == FieldMPAA)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBMovie::mpaa_type_, std::string>(query::CODBMovie::mpaa, oper, prepared_string);
  }
  else if (m_field == FieldTop250)
  {
    where_query = FormatODBParam<query, query::CODBMovie::top250_type_, int>(query::CODBMovie::top250, oper, std::stoi(param));
  }
  else if (m_field == FieldVotes)
  {
    where_query = FormatODBParam<query, query::defaultRating::votes_type_, int>(query::defaultRating::votes, oper, std::stoi(param));
  }
  else if (m_field == FieldSet)
  {
    //TODO: How?
  }
  else if (m_field == FieldPlaylist)
  {
    //TODO: How?
  }
  else if (m_field == FieldDirector)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::director::name_type_, std::string>(query::director::name, oper, prepared_string);
  }
  else if (m_field == FieldActor)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::actor::name_type_, std::string>(query::actor::name, oper, prepared_string);
  }
  else if (m_field == FieldWriter)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::writingCredit::name_type_, std::string>(query::writingCredit::name, oper, prepared_string);
  }
  else if (m_field == FieldStudio)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::studio::name_type_, std::string>(query::studio::name, oper, prepared_string);
  }
  else if (m_field == FieldCountry)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::country::name_type_, std::string>(query::country::name, oper, prepared_string);
  }
  else if (m_field == FieldLastPlayed)
  {
    //TODO: Operators need to be implemented, after a final date object has been defined for odb
    //where_query = (query::fileView::lastPlayed.is_null() || query::fileView::lastPlayed == "");
    
    /* Orig:
     else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = GetField(m_field, strType) + " IS NULL OR " + GetField(m_field, strType) + parameter;*/
  }
  else if (m_field == FieldDateAdded)
  {
    //TODO: Operators need to be implemented, after a final date object has been defined for odb
    //where_query = (query::fileView::dateAdded.is_null() || query::fileView::dateAdded == "");
    
    /* Orig:
     else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
     query = GetField(m_field, strType) + " IS NULL OR " + GetField(m_field, strType) + parameter;*/
  }
  else if (m_field == FieldTag)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::tag::name_type_, std::string>(query::tag::name, oper, prepared_string);
  }
  else if (m_field == FieldVideoResolution)
  {
    // OPERATOR_CONTAINS
    //where_query = GetODBVideoResolutionQuery(param);
  }
  else if (m_field == FieldAudioChannels)
  {
    //TODO: Where is this used? Operators need to be included / checked
    //where_query = query::CODBStreamDetails::audioChannels == std::stoi(param);
    
    /* Orig:
     else if (m_field == FieldAudioChannels)
       query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND iAudioChannels " + parameter + ")";
     */
  }
  else if (m_field == FieldVideoCodec)
  {
    // OPERATOR_CONTAINS
    //where_query = query::CODBStreamDetails::videoCodec.like(param);
    /* Orig:
      query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strVideoCodec " + parameter + ")";
     */
  }
  else if (m_field == FieldAudioCodec)
  {
    // OPERATOR_CONTAINS
    //where_query = query::CODBStreamDetails::audioCodec.like(param);
    /* Orig:
     query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strAudioCodec " + parameter + ")";
     */
  }
  else if (m_field == FieldAudioLanguage)
  {
    // OPERATOR_CONTAINS
     //where_query = query::CODBStreamDetails::audioLanguage.like(param);
    
    /* Orig:
       query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strAudioLanguage " + parameter + ")";
     */
  }
  else if (m_field == FieldSubtitleLanguage)
  {
    // OPERATOR_CONTAINS
    //where_query = query::CODBStreamDetails::subtitleLanguage.like(param);
    
    /* Orig:
     query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strSubtitleLanguage " + parameter + ")";
     */
  }
  else if (m_field == FieldVideoAspectRatio)
  {
    // OPERATOR_CONTAINS
    //where_query = query::CODBStreamDetails::videoAspect == std::stof(param);
    
    /* Orig:
     query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND fVideoAspect " + parameter + ")";
     */
  }
  else if (m_field == FieldPlaycount)
  {
    /*if (oper == OPERATOR_EQUALS && param == "0")
      where_query += query::fileView::playCount.is_null() || query::fileView::playCount == std::stoi(param);
    else if (oper == OPERATOR_DOES_NOT_EQUAL && param != "0")
      where_query += query::fileView::playCount.is_null() || query::fileView::playCount != std::stoi(param);
    else if (oper == OPERATOR_LESS_THAN)
      where_query += query::fileView::playCount.is_null() || query::fileView::playCount < std::stoi(param);*/
  }
  
  if (negate)
    return !where_query;
  else
    return where_query;
}

odb::query<ODBView_TVShow> CSmartPlaylistRule::FormatTVShowWhereClause(const bool &negate,
                                                                     const CDatabaseQueryRule::SEARCH_OPERATOR &oper,
                                                                     const std::string &param,
                                                                     const std::string &strType) const
{
  typedef odb::query<ODBView_TVShow> query;
  query where_query;
  
  CLog::Log(LOGDEBUG, "%s - param: %s - type: %s - operator: %i", __FUNCTION__, param.c_str(), strType.c_str(), oper);
  
  if (m_field == FieldTitle)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBTVShow::title_type_, std::string>(query::CODBTVShow::title, oper, prepared_string);
  }
  else if (m_field == FieldGenre)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::genre::name_type_, std::string>(query::genre::name, oper, prepared_string);
  }
  else if (m_field == FieldYear)
  {
    where_query = FormatODBParam<query, query::CODBTVShow::premiered_class_::year_type_, int>(query::CODBTVShow::premiered.year, oper, std::stoi(param));
  }
  else if (m_field == FieldTime)
  {
    int seconds = std::stoi(StringUtils::Format("%li", StringUtils::TimeStringToSeconds(param)));
    where_query = FormatODBParam<query, query::CODBTVShow::runtime_type_, int>(query::CODBTVShow::runtime, oper, seconds);
  }
  else if (m_field == FieldRating)
  {
    where_query = FormatODBParam<query, query::defaultRating::rating_type_, float>(query::defaultRating::rating, oper, std::stof(param));
  }
  else if (m_field == FieldPlot)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBTVShow::plot_type_, std::string>(query::CODBTVShow::plot, oper, prepared_string);
  }
  else if (m_field == FieldMPAA)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBTVShow::mpaa_type_, std::string>(query::CODBTVShow::mpaa, oper, prepared_string);
  }
  else if (m_field == FieldVotes)
  {
    where_query = FormatODBParam<query, query::defaultRating::votes_type_, int>(query::defaultRating::votes, oper, std::stoi(param));
  }
  else if (m_field == FieldPath)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::path::path_type_, std::string>(query::path::path, oper, prepared_string);
  }
  else if (m_field == FieldSet)
  {
    //TODO: How?
  }
  else if (m_field == FieldPlaylist)
  {
    //TODO: How?
  }
  else if (m_field == FieldDirector)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::director::name_type_, std::string>(query::director::name, oper, prepared_string);
  }
  else if (m_field == FieldActor)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::actor::name_type_, std::string>(query::actor::name, oper, prepared_string);
  }
  else if (m_field == FieldStudio)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::studio::name_type_, std::string>(query::studio::name, oper, prepared_string);
  }
  else if (m_field == FieldLastPlayed)
  {
    //TODO: Operators need to be implemented, after a final date object has been defined for odb
    //where_query = (query::fileView::lastPlayed.is_null() || query::fileView::lastPlayed == "");
    
    /* Orig:
     else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
     query = GetField(m_field, strType) + " IS NULL OR " + GetField(m_field, strType) + parameter;*/
  }
  else if (m_field == FieldDateAdded)
  {
    //TODO: Operators need to be implemented, after a final date object has been defined for odb
    //where_query = (query::fileView::dateAdded.is_null() || query::fileView::dateAdded == "");
    
    /* Orig:
     else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
     query = GetField(m_field, strType) + " IS NULL OR " + GetField(m_field, strType) + parameter;*/
  }
  else if (m_field == FieldTag)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::tag::name_type_, std::string>(query::tag::name, oper, prepared_string);
  }
  else if (m_field == FieldPlaycount)
  {
    /*if (oper == OPERATOR_EQUALS && param == "0")
     where_query += query::fileView::playCount.is_null() || query::fileView::playCount == std::stoi(param);
     else if (oper == OPERATOR_DOES_NOT_EQUAL && param != "0")
     where_query += query::fileView::playCount.is_null() || query::fileView::playCount != std::stoi(param);
     else if (oper == OPERATOR_LESS_THAN)
     where_query += query::fileView::playCount.is_null() || query::fileView::playCount < std::stoi(param);*/
  }
  else if (m_field == FieldTvShowStatus)
  {
    //TODO
  }
  else if (m_field == FieldNumberOfEpisodes)
  {
    //TODO
  }
  else if (m_field == FieldNumberOfWatchedEpisodes)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBTVShow::status_type_, std::string>(query::CODBTVShow::status, oper, prepared_string);
  }
  
  if (negate)
    return !where_query;
  else
    return where_query;
}

odb::query<ODBView_Episode> CSmartPlaylistRule::FormatEpisodeWhereClause(const bool &negate,
                                                                         const CDatabaseQueryRule::SEARCH_OPERATOR &oper,
                                                                         const std::string &param,
                                                                         const std::string &strType) const
{
  typedef odb::query<ODBView_Episode> query;
  query where_query;
  
  CLog::Log(LOGDEBUG, "%s - param: %s - type: %s - operator: %i", __FUNCTION__, param.c_str(), strType.c_str(), oper);
  
  if (m_field == FieldTitle)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBEpisode::title_type_, std::string>(query::CODBEpisode::title, oper, prepared_string);
  }
  else if (m_field == FieldTvShowTitle)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBTVShow::title_type_, std::string>(query::CODBTVShow::title, oper, prepared_string);
  }
  else if (m_field == FieldFilename)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::fileView::filename_type_, std::string>(query::fileView::filename, oper, prepared_string);
  }
  else if (m_field == FieldPath)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::pathView::path_type_, std::string>(query::pathView::path, oper, prepared_string);
  }
  else if (m_field == FieldYear)
  {
    where_query = FormatODBParam<query, query::CODBEpisode::aired_class_::year_type_, int>(query::CODBEpisode::aired.year, oper, std::stoi(param));
  }
  else if (m_field == FieldTime)
  {
    int seconds = std::stoi(StringUtils::Format("%li", StringUtils::TimeStringToSeconds(param)));
    where_query = FormatODBParam<query, query::CODBEpisode::runtime_type_, int>(query::CODBEpisode::runtime, oper, seconds);
  }
  else if (m_field == FieldAirDate)
  {
    //TODO: Implement
  }
  else if (m_field == FieldRating)
  {
    where_query = FormatODBParam<query, query::defaultRating::rating_type_, float>(query::defaultRating::rating, oper, std::stof(param));
  }
  else if (m_field == FieldUserRating)
  {
    where_query = FormatODBParam<query, query::CODBEpisode::userrating_type_, int>(query::CODBEpisode::userrating, oper, std::stoi(param));
  }
  else if (m_field == FieldPlot)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBEpisode::plot_type_, std::string>(query::CODBEpisode::plot, oper, prepared_string);
  }
  else if (m_field == FieldVotes)
  {
    where_query = FormatODBParam<query, query::defaultRating::votes_type_, int>(query::defaultRating::votes, oper, std::stoi(param));
  }
  else if (m_field == FieldPath)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::pathView::path_type_, std::string>(query::pathView::path, oper, prepared_string);
  }
  else if (m_field == FieldGenre)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::genre::name_type_, std::string>(query::genre::name, oper, prepared_string);
  }
  else if (m_field == FieldDirector)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::director::name_type_, std::string>(query::director::name, oper, prepared_string);
  }
  else if (m_field == FieldActor)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::actor::name_type_, std::string>(query::actor::name, oper, prepared_string);
  }
  else if (m_field == FieldWriter)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::writingCredit::name_type_, std::string>(query::writingCredit::name, oper, prepared_string);
  }
  else if (m_field == FieldStudio)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::studio::name_type_, std::string>(query::studio::name, oper, prepared_string);
  }
  else if (m_field == FieldTag)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::tag::name_type_, std::string>(query::tag::name, oper, prepared_string);
  }
  else if (m_field == FieldMPAA)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBTVShow::mpaa_type_, std::string>(query::CODBTVShow::mpaa, oper, prepared_string);
  }
  else if (m_field == FieldEpisodeNumber)
  {
    where_query = FormatODBParam<query, query::CODBEpisode::episode_type_, int>(query::CODBEpisode::episode, oper, std::stoi(param));
  }
  else if (m_field == FieldSeason)
  {
    where_query = FormatODBParam<query, query::CODBSeason::season_type_, int>(query::CODBSeason::season, oper, std::stoi(param));
  }
  else if (m_field == FieldLastPlayed)
  {
    //TODO: Operators need to be implemented, after a final date object has been defined for odb
    //where_query = (query::fileView::lastPlayed.is_null() || query::fileView::lastPlayed == "");
    
    /* Orig:
     else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
     query = GetField(m_field, strType) + " IS NULL OR " + GetField(m_field, strType) + parameter;*/
  }
  else if (m_field == FieldDateAdded)
  {
    //TODO: Operators need to be implemented, after a final date object has been defined for odb
    //where_query = (query::fileView::dateAdded.is_null() || query::fileView::dateAdded == "");
    
    /* Orig:
     else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
     query = GetField(m_field, strType) + " IS NULL OR " + GetField(m_field, strType) + parameter;*/
  }
  else if (m_field == FieldVideoResolution)
  {
    // OPERATOR_CONTAINS
    //where_query = GetODBVideoResolutionQuery(param);
  }
  else if (m_field == FieldAudioChannels)
  {
    //TODO: Where is this used? Operators need to be included / checked
    //where_query = query::CODBStreamDetails::audioChannels == std::stoi(param);
    
    /* Orig:
     else if (m_field == FieldAudioChannels)
     query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND iAudioChannels " + parameter + ")";
     */
  }
  else if (m_field == FieldVideoCodec)
  {
    // OPERATOR_CONTAINS
    //where_query = query::CODBStreamDetails::videoCodec.like(param);
    /* Orig:
     query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strVideoCodec " + parameter + ")";
     */
  }
  else if (m_field == FieldAudioCodec)
  {
    // OPERATOR_CONTAINS
    //where_query = query::CODBStreamDetails::audioCodec.like(param);
    /* Orig:
     query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strAudioCodec " + parameter + ")";
     */
  }
  else if (m_field == FieldAudioLanguage)
  {
    // OPERATOR_CONTAINS
    //where_query = query::CODBStreamDetails::audioLanguage.like(param);
    
    /* Orig:
     query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strAudioLanguage " + parameter + ")";
     */
  }
  else if (m_field == FieldSubtitleLanguage)
  {
    // OPERATOR_CONTAINS
    //where_query = query::CODBStreamDetails::subtitleLanguage.like(param);
    
    /* Orig:
     query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strSubtitleLanguage " + parameter + ")";
     */
  }
  else if (m_field == FieldVideoAspectRatio)
  {
    // OPERATOR_CONTAINS
    //where_query = query::CODBStreamDetails::videoAspect == std::stof(param);
    
    /* Orig:
     query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND fVideoAspect " + parameter + ")";
     */
  }
  else if (m_field == FieldPlaycount)
  {
    /*if (oper == OPERATOR_EQUALS && param == "0")
     where_query += query::fileView::playCount.is_null() || query::fileView::playCount == std::stoi(param);
     else if (oper == OPERATOR_DOES_NOT_EQUAL && param != "0")
     where_query += query::fileView::playCount.is_null() || query::fileView::playCount != std::stoi(param);
     else if (oper == OPERATOR_LESS_THAN)
     where_query += query::fileView::playCount.is_null() || query::fileView::playCount < std::stoi(param);*/
  }
  
  if (negate)
    return !where_query;
  else
    return where_query;
}

odb::query<ODBView_Song_Artists> CSmartPlaylistRule::FormatArtistWhereClause( const bool &negate,
                                                                              const CDatabaseQueryRule::SEARCH_OPERATOR &oper,
                                                                              const std::string &param,
                                                                              const std::string &strType) const
{
  typedef odb::query<ODBView_Song_Artists> query;
  query where_query;
  
  CLog::Log(LOGDEBUG, "%s - param: %s - type: %s - operator: %i", __FUNCTION__, param.c_str(), strType.c_str(), oper);
  
  if (m_field == FieldGenre)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::genre::name_type_, std::string>(query::genre::name, oper, prepared_string);
  }
  else if (m_field == FieldArtist)
  {
    //TODO: We need to check what value is passed here
  }
  else if (m_field == FieldMoods)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBArtistDetail::moods_type_, std::string>(query::CODBArtistDetail::moods, oper, prepared_string);
  }
  else if (m_field == FieldStyles)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBArtistDetail::styles_type_, std::string>(query::CODBArtistDetail::styles, oper, prepared_string);
  }
  else if (m_field == FieldInstruments)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBArtistDetail::instruments_type_, std::string>(query::CODBArtistDetail::instruments, oper, prepared_string);
  }
  else if (m_field == FieldBiography)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBArtistDetail::biography_type_, std::string>(query::CODBArtistDetail::biography, oper, prepared_string);
  }
  else if (m_field == FieldBorn)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBArtistDetail::born_type_, std::string>(query::CODBArtistDetail::born, oper, prepared_string);
  }
  else if (m_field == FieldBandFormed)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBArtistDetail::formed_type_, std::string>(query::CODBArtistDetail::formed, oper, prepared_string);
  }
  else if (m_field == FieldDisbanded)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBArtistDetail::disbanded_type_, std::string>(query::CODBArtistDetail::disbanded, oper, prepared_string);
  }
  else if (m_field == FieldDied)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBArtistDetail::died_type_, std::string>(query::CODBArtistDetail::died, oper, prepared_string);
  }
  else if (m_field == FieldRole)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBRole::name_type_, std::string>(query::CODBRole::name, oper, prepared_string);
    SetHasRoleRule(true);
  }
  else if (m_field == FieldPath)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBPath::path_type_, std::string>(query::CODBPath::path, oper, prepared_string);
  }
  else if (m_field == FieldPlaylist)
  {
    //TODO: How?
  }
    

  if (negate)
    return !where_query;
  else
    return where_query;
}

odb::query<ODBView_Album> CSmartPlaylistRule::FormatAlbumWhereClause( const bool &negate,
                                                                      const CDatabaseQueryRule::SEARCH_OPERATOR &oper,
                                                                      const std::string &param,
                                                                      const std::string &strType) const
{
  typedef odb::query<ODBView_Album> query;
  query where_query;
  
  CLog::Log(LOGDEBUG, "%s - param: %s - type: %s - operator: %i", __FUNCTION__, param.c_str(), strType.c_str(), oper);
  
  if (m_field == FieldGenre)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBGenre::name_type_, std::string>(query::CODBGenre::name, oper, prepared_string);
  }
  else if (m_field == FieldAlbum)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBAlbum::album_type_, std::string>(query::CODBAlbum::album, oper, prepared_string);
  }
  else if (m_field == FieldArtist || m_field == FieldAlbumArtist)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBPerson::name_type_, std::string>(query::CODBPerson::name, oper, prepared_string);
  }
  else if (m_field == FieldYear)
  {
    where_query = FormatODBParam<query, query::CODBAlbum::year_type_, int>(query::CODBAlbum::year, oper, std::stoi(param));
  }
  else if (m_field == FieldReview)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBAlbum::review_type_, std::string>(query::CODBAlbum::review, oper, prepared_string);
  }
  else if (m_field == FieldThemes)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBAlbum::themes_type_, std::string>(query::CODBAlbum::themes, oper, prepared_string);
  }
  else if (m_field == FieldMoods)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBAlbum::moods_type_, std::string>(query::CODBAlbum::moods, oper, prepared_string);
  }
  else if (m_field == FieldStyles)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBAlbum::styles_type_, std::string>(query::CODBAlbum::styles, oper, prepared_string);
  }
  else if (m_field == FieldAlbumType)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBAlbum::type_type_, std::string>(query::CODBAlbum::type, oper, prepared_string);
  }
  else if (m_field == FieldLabel)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBAlbum::label_type_, std::string>(query::CODBAlbum::label, oper, prepared_string);
  }
  else if (m_field == FieldPlaylist)
  {
    //TODO: How?
  }
  
  if (negate)
    return !where_query;
  else
    return where_query;
}

odb::query<ODBView_Song> CSmartPlaylistRule::FormatSongWhereClause( const bool &negate,
                                                                    const CDatabaseQueryRule::SEARCH_OPERATOR &oper,
                                                                    const std::string &param,
                                                                    const std::string &strType) const
{
  typedef odb::query<ODBView_Song> query;
  query where_query;
  
  CLog::Log(LOGDEBUG, "%s - param: %s - type: %s - operator: %i", __FUNCTION__, param.c_str(), strType.c_str(), oper);
  
  if (m_field == FieldGenre)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBGenre::name_type_, std::string>(query::CODBGenre::name, oper, prepared_string);
  }
  else if (m_field == FieldAlbum)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBAlbum::album_type_, std::string>(query::CODBAlbum::album, oper, prepared_string);
  }
  else if (m_field == FieldArtist || m_field == FieldAlbumArtist)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBPerson::name_type_, std::string>(query::CODBPerson::name, oper, prepared_string);
  }
  else if (m_field == FieldTitle)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBSong::title_type_, std::string>(query::CODBSong::title, oper, prepared_string);
  }
  else if (m_field == FieldYear)
  {
    where_query = FormatODBParam<query, query::CODBSong::year_type_, int>(query::CODBSong::year, oper, std::stoi(param));
  }
  else if (m_field == FieldTime)
  {
    int seconds = std::stoi(StringUtils::Format("%li", StringUtils::TimeStringToSeconds(param)));
    where_query = FormatODBParam<query, query::CODBSong::duration_type_, int>(query::CODBSong::duration, oper, seconds);
  }
  else if (m_field == FieldTrackNumber)
  {
    where_query = FormatODBParam<query, query::CODBSong::track_type_, int>(query::CODBSong::track, oper, std::stoi(param));
  }
  else if (m_field == FieldFilename)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBFile::filename_type_, std::string>(query::CODBFile::filename, oper, prepared_string);
  }
  else if (m_field == FieldPath)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBPath::path_type_, std::string>(query::CODBPath::path, oper, prepared_string);
  }
  else if (m_field == FieldPlaycount)
  {
    where_query = FormatODBParam<query, query::CODBFile::playCount_type_, int>(query::CODBFile::playCount, oper, std::stoi(param));
  }
  else if (m_field == FieldRating)
  {
    where_query = FormatODBParam<query, query::CODBSong::rating_type_, int>(query::CODBSong::rating, oper, std::stoi(param));
  }
  else if (m_field == FieldComment)
  {
    std::string prepared_string = FormatODBString(oper, param);
    where_query = FormatODBParam<query, query::CODBSong::comment_type_, std::string>(query::CODBSong::comment, oper, prepared_string);
  }
  else if (m_field == FieldPlaylist)
  {
    //TODO: How?
  }
  
  if (negate)
    return !where_query;
  else
    return where_query;
}

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
    else if (m_field == FieldLastPlayed && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = GetField(m_field, strType) + " is NULL or " + GetField(m_field, strType) + parameter;
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
    else if (m_field == FieldLastPlayed && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = GetField(m_field, strType) + " is NULL or " + GetField(m_field, strType) + parameter;
  }
  else if (strType == "artists")
  {
    table = "artistview";

    if (m_field == FieldGenre)
    {
      query = negate + " (EXISTS (SELECT DISTINCT song_artist.idArtist FROM song_artist, song_genre, genre WHERE song_artist.idArtist = " + GetField(FieldId, strType) + " AND song_artist.idSong = song_genre.idSong AND song_genre.idGenre = genre.idGenre AND genre.strGenre" + parameter + ")";
      query += " OR ";
      query += "EXISTS (SELECT DISTINCT album_artist.idArtist FROM album_artist, album_genre, genre WHERE album_artist.idArtist = " + GetField(FieldId, strType) + " AND album_artist.idAlbum = album_genre.idAlbum AND album_genre.idGenre = genre.idGenre AND genre.strGenre" + parameter + "))";
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
    else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = GetField(m_field, strType) + " IS NULL OR " + GetField(m_field, strType) + parameter;
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
    else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = GetField(m_field, strType) + " IS NULL OR " + GetField(m_field, strType) + parameter;
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
    else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = GetField(m_field, strType) + " IS NULL OR " + GetField(m_field, strType) + parameter;
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
    else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = GetField(m_field, strType) + " IS NULL OR " + GetField(m_field, strType) + parameter;
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
  if (m_field == FieldPlaycount && strType != "songs" && strType != "albums" && strType != "tvshows")
  { // playcount IS stored as NULL OR number IN video db
    if ((m_operator == OPERATOR_EQUALS && param == "0") ||
        (m_operator == OPERATOR_DOES_NOT_EQUAL && param != "0") ||
        (m_operator == OPERATOR_LESS_THAN))
    {
      std::string field = GetField(FieldPlaycount, strType);
      query = field + " IS NULL OR " + field + parameter;
    }
  }
  if (query.empty())
    query = CDatabaseQueryRule::FormatWhereClause(negate, oper, param, db, strType);
  return query;
}

std::string CSmartPlaylistRule::GetField(int field, const std::string &type) const
{
  if (field >= FieldUnknown && field < FieldMax)
    return DatabaseUtils::GetField((Field)field, CMediaTypes::FromString(type), DatabaseQueryPartWhere);
  return "";
}

odb::query<ODBView_Movie> CSmartPlaylistRuleCombination::GetMovieWhereClause(const std::string& strType, std::set<std::string> &referencedPlaylists)
{
  typedef odb::query<ODBView_Movie> query;
  query movie_query;
  
  // translate the combinations into SQL
  for (CDatabaseQueryRuleCombinations::const_iterator it = m_combinations.begin(); it != m_combinations.end(); ++it)
  {
    std::shared_ptr<CSmartPlaylistRuleCombination> combo = std::static_pointer_cast<CSmartPlaylistRuleCombination>(*it);
    if (combo)
    {
      if (it != m_combinations.begin())
      {
        if (m_type == CombinationAnd)
          movie_query = query(movie_query && (combo->GetMovieWhereClause(strType, referencedPlaylists)));
        else
          movie_query = query(movie_query || (combo->GetMovieWhereClause(strType, referencedPlaylists)));
      }
    }
  }
  
  for (CDatabaseQueryRules::const_iterator it = m_rules.begin(); it != m_rules.end(); ++it)
  {
    // don't include playlists that are meant to be displayed
    // as a virtual folders in the SQL WHERE clause
    if ((*it)->m_field == FieldVirtualFolder)
      continue;
    
    //if (!rule.empty())
    //  rule += m_type == CombinationAnd ? " AND " : " OR ";
    //rule += "(";
    query currentRule;
    if ((*it)->m_field == FieldPlaylist)
    {
      std::string playlistFile = CSmartPlaylistDirectory::GetPlaylistByName((*it)->m_parameter.at(0), strType);
      if (!playlistFile.empty() && referencedPlaylists.find(playlistFile) == referencedPlaylists.end())
      {
        referencedPlaylists.insert(playlistFile);
        CSmartPlaylist playlist;
        if (playlist.Load(playlistFile))
        {
          query playlistQuery;
          // only playlists of same type will be part of the query
          if (playlist.GetType() == strType || (playlist.GetType() == "mixed" && (strType == "songs" || strType == "musicvideos")) || playlist.GetType().empty())
          {
            playlist.SetType(strType);
            playlistQuery = playlist.GetMovieWhereClause(referencedPlaylists);
          }
          if (playlist.GetType() == strType)
          {
            if ((*it)->m_operator == CDatabaseQueryRule::OPERATOR_DOES_NOT_EQUAL)
              currentRule = query(" NOT ("+playlistQuery+") ");
            else
              currentRule = playlistQuery;
          }
        }
      }
    }
    else
      currentRule = (*it)->GetMovieWhereClause(strType);
    
    if (m_type == CombinationAnd)
      movie_query = query(movie_query && currentRule);
    else
      movie_query = query(movie_query || currentRule);
  }
  
  return movie_query;
}

odb::query<ODBView_TVShow> CSmartPlaylistRuleCombination::GetTVShowWhereClause(const std::string& strType, std::set<std::string> &referencedPlaylists)
{
  typedef odb::query<ODBView_TVShow> query;
  query tvshow_query;
  
  // translate the combinations into SQL
  for (CDatabaseQueryRuleCombinations::const_iterator it = m_combinations.begin(); it != m_combinations.end(); ++it)
  {
    std::shared_ptr<CSmartPlaylistRuleCombination> combo = std::static_pointer_cast<CSmartPlaylistRuleCombination>(*it);
    if (combo)
    {
      if (it != m_combinations.begin())
      {
        if (m_type == CombinationAnd)
          tvshow_query = query(tvshow_query && (combo->GetTVShowWhereClause(strType, referencedPlaylists)));
        else
          tvshow_query = query(tvshow_query || (combo->GetTVShowWhereClause(strType, referencedPlaylists)));
      }
    }
  }
  
  for (CDatabaseQueryRules::const_iterator it = m_rules.begin(); it != m_rules.end(); ++it)
  {
    // don't include playlists that are meant to be displayed
    // as a virtual folders in the SQL WHERE clause
    if ((*it)->m_field == FieldVirtualFolder)
      continue;
    
    //if (!rule.empty())
    //  rule += m_type == CombinationAnd ? " AND " : " OR ";
    //rule += "(";
    query currentRule;
    if ((*it)->m_field == FieldPlaylist)
    {
      std::string playlistFile = CSmartPlaylistDirectory::GetPlaylistByName((*it)->m_parameter.at(0), strType);
      if (!playlistFile.empty() && referencedPlaylists.find(playlistFile) == referencedPlaylists.end())
      {
        referencedPlaylists.insert(playlistFile);
        CSmartPlaylist playlist;
        if (playlist.Load(playlistFile))
        {
          query playlistQuery;
          // only playlists of same type will be part of the query
          if (playlist.GetType() == strType || (playlist.GetType() == "mixed" && (strType == "songs" || strType == "musicvideos")) || playlist.GetType().empty())
          {
            playlist.SetType(strType);
            playlistQuery = playlist.GetTVShowWhereClause(referencedPlaylists);
          }
          if (playlist.GetType() == strType)
          {
            if ((*it)->m_operator == CDatabaseQueryRule::OPERATOR_DOES_NOT_EQUAL)
              currentRule = query(" NOT ("+playlistQuery+") ");
            else
              currentRule = playlistQuery;
          }
        }
      }
    }
    else
      currentRule = (*it)->GetTVShowWhereClause(strType);
    
    if (m_type == CombinationAnd)
      tvshow_query = query(tvshow_query && currentRule);
    else
      tvshow_query = query(tvshow_query || currentRule);
  }
  
  return tvshow_query;
}

odb::query<ODBView_Episode> CSmartPlaylistRuleCombination::GetEpisodeWhereClause(const std::string& strType, std::set<std::string> &referencedPlaylists)
{
  typedef odb::query<ODBView_Episode> query;
  query episode_query;
  
  // translate the combinations into SQL
  for (CDatabaseQueryRuleCombinations::const_iterator it = m_combinations.begin(); it != m_combinations.end(); ++it)
  {
    std::shared_ptr<CSmartPlaylistRuleCombination> combo = std::static_pointer_cast<CSmartPlaylistRuleCombination>(*it);
    if (combo)
    {
      if (it != m_combinations.begin())
      {
        if (m_type == CombinationAnd)
          episode_query = query(episode_query && (combo->GetEpisodeWhereClause(strType, referencedPlaylists)));
        else
          episode_query = query(episode_query || (combo->GetEpisodeWhereClause(strType, referencedPlaylists)));
      }
    }
  }
  
  for (CDatabaseQueryRules::const_iterator it = m_rules.begin(); it != m_rules.end(); ++it)
  {
    // don't include playlists that are meant to be displayed
    // as a virtual folders in the SQL WHERE clause
    if ((*it)->m_field == FieldVirtualFolder)
      continue;
    
    //if (!rule.empty())
    //  rule += m_type == CombinationAnd ? " AND " : " OR ";
    //rule += "(";
    query currentRule;
    if ((*it)->m_field == FieldPlaylist)
    {
      std::string playlistFile = CSmartPlaylistDirectory::GetPlaylistByName((*it)->m_parameter.at(0), strType);
      if (!playlistFile.empty() && referencedPlaylists.find(playlistFile) == referencedPlaylists.end())
      {
        referencedPlaylists.insert(playlistFile);
        CSmartPlaylist playlist;
        if (playlist.Load(playlistFile))
        {
          query playlistQuery;
          // only playlists of same type will be part of the query
          if (playlist.GetType() == strType || (playlist.GetType() == "mixed" && (strType == "songs" || strType == "musicvideos")) || playlist.GetType().empty())
          {
            playlist.SetType(strType);
            playlistQuery = playlist.GetTVShowWhereClause(referencedPlaylists);
          }
          if (playlist.GetType() == strType)
          {
            if ((*it)->m_operator == CDatabaseQueryRule::OPERATOR_DOES_NOT_EQUAL)
              currentRule = query(" NOT ("+playlistQuery+") ");
            else
              currentRule = playlistQuery;
          }
        }
      }
    }
    else
      currentRule = (*it)->GetEpisodeWhereClause(strType);
    
    if (m_type == CombinationAnd)
      episode_query = query(episode_query && currentRule);
    else
      episode_query = query(episode_query || currentRule);
  }
  
  return episode_query;
}

odb::query<ODBView_Song_Artists> CSmartPlaylistRuleCombination::GetArtistWhereClause(const std::string& strType, std::set<std::string> &referencedPlaylists)
{
  typedef odb::query<ODBView_Song_Artists> query;
  query artists_query;
  
  // translate the combinations into SQL
  for (CDatabaseQueryRuleCombinations::const_iterator it = m_combinations.begin(); it != m_combinations.end(); ++it)
  {
    std::shared_ptr<CSmartPlaylistRuleCombination> combo = std::static_pointer_cast<CSmartPlaylistRuleCombination>(*it);
    if (combo)
    {
      if (it != m_combinations.begin())
      {
        if (m_type == CombinationAnd)
          artists_query = query(artists_query && (combo->GetArtistWhereClause(strType, referencedPlaylists)));
        else
          artists_query = query(artists_query || (combo->GetArtistWhereClause(strType, referencedPlaylists)));
      }
    }
  }
  
  for (CDatabaseQueryRules::const_iterator it = m_rules.begin(); it != m_rules.end(); ++it)
  {
    // don't include playlists that are meant to be displayed
    // as a virtual folders in the SQL WHERE clause
    if ((*it)->m_field == FieldVirtualFolder)
      continue;
    
    //if (!rule.empty())
    //  rule += m_type == CombinationAnd ? " AND " : " OR ";
    //rule += "(";
    query currentRule;
    if ((*it)->m_field == FieldPlaylist)
    {
      std::string playlistFile = CSmartPlaylistDirectory::GetPlaylistByName((*it)->m_parameter.at(0), strType);
      if (!playlistFile.empty() && referencedPlaylists.find(playlistFile) == referencedPlaylists.end())
      {
        referencedPlaylists.insert(playlistFile);
        CSmartPlaylist playlist;
        if (playlist.Load(playlistFile))
        {
          query playlistQuery;
          // only playlists of same type will be part of the query
          if (playlist.GetType() == strType || (playlist.GetType() == "mixed" && (strType == "songs" || strType == "musicvideos")) || playlist.GetType().empty())
          {
            playlist.SetType(strType);
            playlistQuery = playlist.GetArtistWhereClause(referencedPlaylists);
          }
          if (playlist.GetType() == strType)
          {
            if ((*it)->m_operator == CDatabaseQueryRule::OPERATOR_DOES_NOT_EQUAL)
              currentRule = query(" NOT ("+playlistQuery+") ");
            else
              currentRule = playlistQuery;
          }
        }
      }
    }
    else
    {
      currentRule = (*it)->GetArtistWhereClause(strType);
      if ((*it)->GetHasRoleRule())
        SetHasRoleRule(true);
    }
    
    if (m_type == CombinationAnd)
      artists_query = query(artists_query && currentRule);
    else
      artists_query = query(artists_query || currentRule);
  }
  
  return artists_query;
}

odb::query<ODBView_Album> CSmartPlaylistRuleCombination::GetAlbumWhereClause(const std::string& strType, std::set<std::string> &referencedPlaylists)
{
  typedef odb::query<ODBView_Album> query;
  query album_query;
  
  // translate the combinations into SQL
  for (CDatabaseQueryRuleCombinations::const_iterator it = m_combinations.begin(); it != m_combinations.end(); ++it)
  {
    std::shared_ptr<CSmartPlaylistRuleCombination> combo = std::static_pointer_cast<CSmartPlaylistRuleCombination>(*it);
    if (combo)
    {
      if (it != m_combinations.begin())
      {
        if (m_type == CombinationAnd)
          album_query = query(album_query && (combo->GetAlbumWhereClause(strType, referencedPlaylists)));
        else
          album_query = query(album_query || (combo->GetAlbumWhereClause(strType, referencedPlaylists)));
      }
    }
  }
  
  for (CDatabaseQueryRules::const_iterator it = m_rules.begin(); it != m_rules.end(); ++it)
  {
    // don't include playlists that are meant to be displayed
    // as a virtual folders in the SQL WHERE clause
    if ((*it)->m_field == FieldVirtualFolder)
      continue;
    
    query currentRule;
    if ((*it)->m_field == FieldPlaylist)
    {
      std::string playlistFile = CSmartPlaylistDirectory::GetPlaylistByName((*it)->m_parameter.at(0), strType);
      if (!playlistFile.empty() && referencedPlaylists.find(playlistFile) == referencedPlaylists.end())
      {
        referencedPlaylists.insert(playlistFile);
        CSmartPlaylist playlist;
        if (playlist.Load(playlistFile))
        {
          query playlistQuery;
          // only playlists of same type will be part of the query
          if (playlist.GetType() == strType || (playlist.GetType() == "mixed" && (strType == "songs" || strType == "musicvideos")) || playlist.GetType().empty())
          {
            playlist.SetType(strType);
            playlistQuery = playlist.GetAlbumWhereClause(referencedPlaylists);
          }
          if (playlist.GetType() == strType)
          {
            if ((*it)->m_operator == CDatabaseQueryRule::OPERATOR_DOES_NOT_EQUAL)
              currentRule = query(" NOT ("+playlistQuery+") ");
            else
              currentRule = playlistQuery;
          }
        }
      }
    }
    else
      currentRule = (*it)->GetAlbumWhereClause(strType);
    
    if (m_type == CombinationAnd)
      album_query = query(album_query && currentRule);
    else
      album_query = query(album_query || currentRule);
  }
  
  return album_query;
}

odb::query<ODBView_Song> CSmartPlaylistRuleCombination::GetSongWhereClause(const std::string& strType, std::set<std::string> &referencedPlaylists)
{
  typedef odb::query<ODBView_Song> query;
  query song_query;
  
  // translate the combinations into SQL
  for (CDatabaseQueryRuleCombinations::const_iterator it = m_combinations.begin(); it != m_combinations.end(); ++it)
  {
    std::shared_ptr<CSmartPlaylistRuleCombination> combo = std::static_pointer_cast<CSmartPlaylistRuleCombination>(*it);
    if (combo)
    {
      if (it != m_combinations.begin())
      {
        if (m_type == CombinationAnd)
          song_query = query(song_query && (combo->GetSongWhereClause(strType, referencedPlaylists)));
        else
          song_query = query(song_query || (combo->GetSongWhereClause(strType, referencedPlaylists)));
      }
    }
  }
  
  for (CDatabaseQueryRules::const_iterator it = m_rules.begin(); it != m_rules.end(); ++it)
  {
    // don't include playlists that are meant to be displayed
    // as a virtual folders in the SQL WHERE clause
    if ((*it)->m_field == FieldVirtualFolder)
      continue;
    
    query currentRule;
    if ((*it)->m_field == FieldPlaylist)
    {
      std::string playlistFile = CSmartPlaylistDirectory::GetPlaylistByName((*it)->m_parameter.at(0), strType);
      if (!playlistFile.empty() && referencedPlaylists.find(playlistFile) == referencedPlaylists.end())
      {
        referencedPlaylists.insert(playlistFile);
        CSmartPlaylist playlist;
        if (playlist.Load(playlistFile))
        {
          query playlistQuery;
          // only playlists of same type will be part of the query
          if (playlist.GetType() == strType || (playlist.GetType() == "mixed" && (strType == "songs" || strType == "musicvideos")) || playlist.GetType().empty())
          {
            playlist.SetType(strType);
            playlistQuery = playlist.GetAlbumWhereClause(referencedPlaylists);
          }
          if (playlist.GetType() == strType)
          {
            if ((*it)->m_operator == CDatabaseQueryRule::OPERATOR_DOES_NOT_EQUAL)
              currentRule = query(" NOT ("+playlistQuery+") ");
            else
              currentRule = playlistQuery;
          }
        }
      }
    }
    else
      currentRule = (*it)->GetSongWhereClause(strType);
    
    if (m_type == CombinationAnd)
      song_query = query(song_query && currentRule);
    else
      song_query = query(song_query || currentRule);
  }
  
  return song_query;
}

std::string CSmartPlaylistRuleCombination::GetWhereClause(const CDatabase &db, const std::string& strType, std::set<std::string> &referencedPlaylists) const
{
  std::string rule;
  
  // translate the combinations into SQL
  for (CDatabaseQueryRuleCombinations::const_iterator it = m_combinations.begin(); it != m_combinations.end(); ++it)
  {
    if (it != m_combinations.begin())
      rule += m_type == CombinationAnd ? " AND " : " OR ";
    std::shared_ptr<CSmartPlaylistRuleCombination> combo = std::static_pointer_cast<CSmartPlaylistRuleCombination>(*it);
    if (combo)
      rule += "(" + combo->GetWhereClause(db, strType, referencedPlaylists) + ")";
  }

  // translate the rules into SQL
  for (CDatabaseQueryRules::const_iterator it = m_rules.begin(); it != m_rules.end(); ++it)
  {
    // don't include playlists that are meant to be displayed
    // as a virtual folders in the SQL WHERE clause
    if ((*it)->m_field == FieldVirtualFolder)
      continue;

    if (!rule.empty())
      rule += m_type == CombinationAnd ? " AND " : " OR ";
    rule += "(";
    std::string currentRule;
    if ((*it)->m_field == FieldPlaylist)
    {
      std::string playlistFile = CSmartPlaylistDirectory::GetPlaylistByName((*it)->m_parameter.at(0), strType);
      if (!playlistFile.empty() && referencedPlaylists.find(playlistFile) == referencedPlaylists.end())
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
            if ((*it)->m_operator == CDatabaseQueryRule::OPERATOR_DOES_NOT_EQUAL)
              currentRule = StringUtils::Format("NOT (%s)", playlistQuery.c_str());
            else
              currentRule = playlistQuery;
          }
        }
      }
    }
    else
      currentRule = (*it)->GetWhereClause(db, strType);
    // if we don't get a rule, we add '1' or '0' so the query is still valid and doesn't fail
    if (currentRule.empty())
      currentRule = m_type == CombinationAnd ? "'1'" : "'0'";
    rule += currentRule;
    rule += ")";
  }

  return rule;
}

void CSmartPlaylistRuleCombination::GetVirtualFolders(const std::string& strType, std::vector<std::string> &virtualFolders) const
{
  for (CDatabaseQueryRuleCombinations::const_iterator it = m_combinations.begin(); it != m_combinations.end(); ++it)
  {
    std::shared_ptr<CSmartPlaylistRuleCombination> combo = std::static_pointer_cast<CSmartPlaylistRuleCombination>(*it);
    if (combo)
      combo->GetVirtualFolders(strType, virtualFolders);
  }

  for (CDatabaseQueryRules::const_iterator it = m_rules.begin(); it != m_rules.end(); ++it)
  {
    if (((*it)->m_field != FieldVirtualFolder && (*it)->m_field != FieldPlaylist) || (*it)->m_operator != CDatabaseQueryRule::OPERATOR_EQUALS)
      continue;

    std::string playlistFile = CSmartPlaylistDirectory::GetPlaylistByName((*it)->m_parameter.at(0), strType);
    if (playlistFile.empty())
      continue;

    if ((*it)->m_field == FieldVirtualFolder)
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

void CSmartPlaylistRuleCombination::AddRule(const CSmartPlaylistRule &rule)
{
  std::shared_ptr<CSmartPlaylistRule> ptr(new CSmartPlaylistRule(rule));
  m_rules.push_back(ptr);
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

  if (!root || !StringUtils::EqualsNoCase(root->Value(),"smartplaylist"))
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
    CLog::Log(LOGERROR, "Error loading Smart playlist %s (failed to read file)", url.GetRedacted().c_str());
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
    CLog::Log(LOGERROR, "Error loading Smart playlist (failed to parse xml: %s)", m_xmlDoc.ErrorDesc());
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
      m_orderDirection = StringUtils::EqualsNoCase(order["direction"].asString(), "ascending") ? SortOrderAscending : SortOrderDescending;

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
    m_ruleCombination.SetType(StringUtils::EqualsNoCase(tmp, "all") ? CSmartPlaylistRuleCombination::CombinationAnd : CSmartPlaylistRuleCombination::CombinationOr);

  // now the rules
  const TiXmlNode *ruleNode = root->FirstChild("rule");
  while (ruleNode)
  {
    CSmartPlaylistRule rule;
    if (rule.Load(ruleNode, encoding))
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
      m_orderDirection = StringUtils::EqualsNoCase(direction, "ascending") ? SortOrderAscending : SortOrderDescending;

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
  XMLUtils::SetString(pRoot, "match", m_ruleCombination.GetType() == CSmartPlaylistRuleCombination::CombinationAnd ? "all" : "one");

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
    nodeOrder.SetAttribute("direction", m_orderDirection == SortOrderDescending ? "descending" : "ascending");
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
    obj["order"]["direction"] = m_orderDirection == SortOrderDescending ? "descending" : "ascending";
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
  m_orderDirection = SortOrderNone;
  m_orderAttributes = SortAttributeNone;
  m_playlistType = "songs"; // sane default
  m_group.clear();
  m_groupMixed = false;
  m_hasRoleRules = false;
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

odb::query<ODBView_Movie> CSmartPlaylist::GetMovieWhereClause(std::set<std::string> &referencedPlaylists)
{
  return m_ruleCombination.GetMovieWhereClause(GetType(), referencedPlaylists);
}

odb::query<ODBView_TVShow> CSmartPlaylist::GetTVShowWhereClause(std::set<std::string> &referencedPlaylists)
{
  return m_ruleCombination.GetTVShowWhereClause(GetType(), referencedPlaylists);
}

odb::query<ODBView_Episode> CSmartPlaylist::GetEpisodeWhereClause(std::set<std::string> &referencedPlaylists)
{
  return m_ruleCombination.GetEpisodeWhereClause(GetType(), referencedPlaylists);
}

odb::query<ODBView_Song_Artists> CSmartPlaylist::GetArtistWhereClause(std::set<std::string> &referencedPlaylists)
{
  return m_ruleCombination.GetArtistWhereClause(GetType(), referencedPlaylists);
}

odb::query<ODBView_Album> CSmartPlaylist::GetAlbumWhereClause(std::set<std::string> &referencedPlaylists)
{
  return m_ruleCombination.GetAlbumWhereClause(GetType(), referencedPlaylists);
}

odb::query<ODBView_Song> CSmartPlaylist::GetSongWhereClause(std::set<std::string> &referencedPlaylists)
{
  return m_ruleCombination.GetSongWhereClause(GetType(), referencedPlaylists);
}

std::string CSmartPlaylist::GetWhereClause(const CDatabase &db, std::set<std::string> &referencedPlaylists) const
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
    for (unsigned int i = 0; i < NUM_FIELDS; i++)
    {
      if (*field == fields[i].field)
        fieldList.push_back(fields[i].string);
    }
  }
}

bool CSmartPlaylist::IsEmpty(bool ignoreSortAndLimit /* = true */) const
{
  bool empty = m_ruleCombination.empty();
  if (empty && !ignoreSortAndLimit)
    empty = m_limit <= 0 && m_orderField == SortByNone && m_orderDirection == SortOrderNone;

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

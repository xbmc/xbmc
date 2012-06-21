/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "SmartPlayList.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "filesystem/SmartPlaylistDirectory.h"
#include "filesystem/File.h"
#include "utils/CharsetConverter.h"
#include "utils/DatabaseUtils.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include "video/VideoDatabase.h"
#include "Util.h"
#include "XBDateTime.h"
#include "guilib/LocalizeStrings.h"

using namespace std;
using namespace XFILE;

typedef struct
{
  char string[17];
  Field field;
  SortBy sort;
  CSmartPlaylistRule::FIELD_TYPE type;
  int localizedString;
} translateField;

static const translateField fields[] = {
  { "none",              FieldNone,                    SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,       231 },
  { "filename",          FieldFilename,                SortByFile,                     CSmartPlaylistRule::TEXT_FIELD,       561 },
  { "path",              FieldPath,                    SortByPath,                     CSmartPlaylistRule::BROWSEABLE_FIELD, 573 },
  { "album",             FieldAlbum,                   SortByAlbum,                    CSmartPlaylistRule::BROWSEABLE_FIELD, 558 },
  { "albumartist",       FieldAlbumArtist,             SortByNone,                     CSmartPlaylistRule::BROWSEABLE_FIELD, 566 },
  { "artist",            FieldArtist,                  SortByArtist,                   CSmartPlaylistRule::BROWSEABLE_FIELD, 557 },
  { "tracknumber",       FieldTrackNumber,             SortByTrackNumber,              CSmartPlaylistRule::NUMERIC_FIELD,    554 },
  { "comment",           FieldComment,                 SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,       569 },
  { "review",            FieldReview,                  SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,       183 },
  { "themes",            FieldThemes,                  SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,       21895 },
  { "moods",             FieldMoods,                   SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,       175 },
  { "styles",            FieldStyles,                  SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,       176 },
  { "type",              FieldAlbumType,               SortByAlbumType,                CSmartPlaylistRule::TEXT_FIELD,       564 },
  { "label",             FieldMusicLabel,              SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,       21899 },
  { "title",             FieldTitle,                   SortByTitle,                    CSmartPlaylistRule::TEXT_FIELD,       556 },
  { "sorttitle",         FieldSortTitle,               SortBySortTitle,                CSmartPlaylistRule::TEXT_FIELD,       556 },
  { "year",              FieldYear,                    SortByYear,                     CSmartPlaylistRule::NUMERIC_FIELD,    562 },
  { "time",              FieldTime,                    SortByTime,                     CSmartPlaylistRule::SECONDS_FIELD,    180 },
  { "playcount",         FieldPlaycount,               SortByPlaycount,                CSmartPlaylistRule::NUMERIC_FIELD,    567 },
  { "lastplayed",        FieldLastPlayed,              SortByLastPlayed,               CSmartPlaylistRule::DATE_FIELD,       568 },
  { "inprogress",        FieldInProgress,              SortByNone,                     CSmartPlaylistRule::BOOLEAN_FIELD,    575 },
  { "rating",            FieldRating,                  SortByRating,                   CSmartPlaylistRule::NUMERIC_FIELD,    563 },
  { "votes",             FieldVotes,                   SortByVotes,                    CSmartPlaylistRule::TEXT_FIELD,       205 },
  { "top250",            FieldTop250,                  SortByTop250,                   CSmartPlaylistRule::NUMERIC_FIELD,    13409 },
  { "mpaarating",        FieldMPAA,                    SortByMPAA,                     CSmartPlaylistRule::TEXT_FIELD,       20074 },
  { "dateadded",         FieldDateAdded,               SortByDateAdded,                CSmartPlaylistRule::DATE_FIELD,       570 },
  { "genre",             FieldGenre,                   SortByGenre,                    CSmartPlaylistRule::BROWSEABLE_FIELD, 515 },
  { "plot",              FieldPlot,                    SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,       207 },
  { "plotoutline",       FieldPlotOutline,             SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,       203 },
  { "tagline",           FieldTagline,                 SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,       202 },
  { "set",               FieldSet,                     SortByNone,                     CSmartPlaylistRule::BROWSEABLE_FIELD, 20457 },
  { "director",          FieldDirector,                SortByNone,                     CSmartPlaylistRule::BROWSEABLE_FIELD, 20339 },
  { "actor",             FieldActor,                   SortByNone,                     CSmartPlaylistRule::BROWSEABLE_FIELD, 20337 },
  { "writers",           FieldWriter,                  SortByNone,                     CSmartPlaylistRule::BROWSEABLE_FIELD, 20417 },
  { "airdate",           FieldAirDate,                 SortByYear,                     CSmartPlaylistRule::DATE_FIELD,       20416 },
  { "hastrailer",        FieldTrailer,                 SortByNone,                     CSmartPlaylistRule::BOOLEAN_FIELD,    20423 },
  { "studio",            FieldStudio,                  SortByStudio,                   CSmartPlaylistRule::BROWSEABLE_FIELD, 572 },
  { "country",           FieldCountry,                 SortByCountry,                  CSmartPlaylistRule::BROWSEABLE_FIELD, 574 },
  { "tvshow",            FieldTvShowTitle,             SortByTvShowTitle,              CSmartPlaylistRule::BROWSEABLE_FIELD, 20364 },
  { "status",            FieldTvShowStatus,            SortByTvShowStatus,             CSmartPlaylistRule::TEXT_FIELD,       126 },
  { "season",            FieldSeason,                  SortBySeason,                   CSmartPlaylistRule::NUMERIC_FIELD,    20373 },
  { "episode",           FieldEpisodeNumber,           SortByEpisodeNumber,            CSmartPlaylistRule::NUMERIC_FIELD,    20359 },
  { "numepisodes",       FieldNumberOfEpisodes,        SortByNumberOfEpisodes,         CSmartPlaylistRule::NUMERIC_FIELD,    20360 },
  { "numwatched",        FieldNumberOfWatchedEpisodes, SortByNumberOfWatchedEpisodes,  CSmartPlaylistRule::NUMERIC_FIELD,    21441 },
  { "videoresolution",   FieldVideoResolution,         SortByVideoResolution,          CSmartPlaylistRule::NUMERIC_FIELD,    21443 },
  { "videocodec",        FieldVideoCodec,              SortByVideoCodec,               CSmartPlaylistRule::TEXTIN_FIELD,     21445 },
  { "videoaspect",       FieldVideoAspectRatio,        SortByVideoAspectRatio,         CSmartPlaylistRule::NUMERIC_FIELD,    21374 },
  { "audiochannels",     FieldAudioChannels,           SortByAudioChannels,            CSmartPlaylistRule::NUMERIC_FIELD,    21444 },
  { "audiocodec",        FieldAudioCodec,              SortByAudioCodec,               CSmartPlaylistRule::TEXTIN_FIELD,     21446 },
  { "audiolanguage",     FieldAudioLanguage,           SortByAudioLanguage,            CSmartPlaylistRule::TEXTIN_FIELD,     21447 },
  { "subtitlelanguage",  FieldSubtitleLanguage,        SortBySubtitleLanguage,         CSmartPlaylistRule::TEXTIN_FIELD,     21448 },
  { "random",            FieldRandom,                  SortByRandom,                   CSmartPlaylistRule::TEXT_FIELD,       590 },
  { "playlist",          FieldPlaylist,                SortByPlaylistOrder,            CSmartPlaylistRule::PLAYLIST_FIELD,   559 }
};

#define NUM_FIELDS sizeof(fields) / sizeof(translateField)

typedef struct
{
  char string[15];
  CSmartPlaylistRule::SEARCH_OPERATOR op;
  int localizedString;
} operatorField;

static const operatorField operators[] = { { "contains", CSmartPlaylistRule::OPERATOR_CONTAINS, 21400 },
                                           { "doesnotcontain", CSmartPlaylistRule::OPERATOR_DOES_NOT_CONTAIN, 21401 },
                                           { "is", CSmartPlaylistRule::OPERATOR_EQUALS, 21402 },
                                           { "isnot", CSmartPlaylistRule::OPERATOR_DOES_NOT_EQUAL, 21403 },
                                           { "startswith", CSmartPlaylistRule::OPERATOR_STARTS_WITH, 21404 },
                                           { "endswith", CSmartPlaylistRule::OPERATOR_ENDS_WITH, 21405 },
                                           { "greaterthan", CSmartPlaylistRule::OPERATOR_GREATER_THAN, 21406 },
                                           { "lessthan", CSmartPlaylistRule::OPERATOR_LESS_THAN, 21407 },
                                           { "after", CSmartPlaylistRule::OPERATOR_AFTER, 21408 },
                                           { "before", CSmartPlaylistRule::OPERATOR_BEFORE, 21409 },
                                           { "inthelast", CSmartPlaylistRule::OPERATOR_IN_THE_LAST, 21410 },
                                           { "notinthelast", CSmartPlaylistRule::OPERATOR_NOT_IN_THE_LAST, 21411 },
                                           { "true", CSmartPlaylistRule::OPERATOR_TRUE, 20122 },
                                           { "false", CSmartPlaylistRule::OPERATOR_FALSE, 20424 }
                                         };

#define NUM_OPERATORS sizeof(operators) / sizeof(operatorField)

CSmartPlaylistRule::CSmartPlaylistRule()
{
  m_field = FieldNone;
  m_operator = OPERATOR_CONTAINS;
  m_parameter = "";
}

void CSmartPlaylistRule::TranslateStrings(const char *field, const char *oper, const char *parameter)
{
  m_field = TranslateField(field);
  m_operator = TranslateOperator(oper);
  m_parameter = parameter;
}

TiXmlElement CSmartPlaylistRule::GetAsElement() const
{
  TiXmlElement rule("rule");
  TiXmlText parameter(m_parameter.c_str());
  rule.InsertEndChild(parameter);
  rule.SetAttribute("field", TranslateField(m_field).c_str());
  rule.SetAttribute("operator", TranslateOperator(m_operator).c_str());
  return rule;
}

Field CSmartPlaylistRule::TranslateField(const char *field)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (strcmpi(field, fields[i].string) == 0) return fields[i].field;
  return FieldNone;
}

CStdString CSmartPlaylistRule::TranslateField(Field field)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return fields[i].string;
  return "none";
}

SortBy CSmartPlaylistRule::TranslateOrder(const char *order)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (strcmpi(order, fields[i].string) == 0) return fields[i].sort;
  return SortByNone;
}

CStdString CSmartPlaylistRule::TranslateOrder(SortBy order)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (order == fields[i].sort) return fields[i].string;
  return "none";
}

CSmartPlaylistRule::SEARCH_OPERATOR CSmartPlaylistRule::TranslateOperator(const char *oper)
{
  for (unsigned int i = 0; i < NUM_OPERATORS; i++)
    if (strcmpi(oper, operators[i].string) == 0) return operators[i].op;
  return OPERATOR_CONTAINS;
}

CStdString CSmartPlaylistRule::TranslateOperator(SEARCH_OPERATOR oper)
{
  for (unsigned int i = 0; i < NUM_OPERATORS; i++)
    if (oper == operators[i].op) return operators[i].string;
  return "contains";
}

CStdString CSmartPlaylistRule::GetLocalizedField(Field field)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return g_localizeStrings.Get(fields[i].localizedString);
  return g_localizeStrings.Get(16018);
}

CStdString CSmartPlaylistRule::GetLocalizedOrder(SortBy order)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (order == fields[i].sort) return g_localizeStrings.Get(fields[i].localizedString);
  return g_localizeStrings.Get(16018);
}

CSmartPlaylistRule::FIELD_TYPE CSmartPlaylistRule::GetFieldType(Field field)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return fields[i].type;
  return TEXT_FIELD;
}

vector<Field> CSmartPlaylistRule::GetFields(const CStdString &type)
{
  vector<Field> fields;
  bool isVideo = false;
  if (type == "songs")
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
    fields.push_back(FieldComment);
    fields.push_back(FieldDateAdded);
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
    fields.push_back(FieldAlbumType);
    fields.push_back(FieldMusicLabel);
    fields.push_back(FieldRating);
  }
  else if (type == "tvshows")
  {
    fields.push_back(FieldTitle);
    fields.push_back(FieldPlot);
    fields.push_back(FieldTvShowStatus);
    fields.push_back(FieldVotes);
    fields.push_back(FieldRating);
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
  }
  else if (type == "episodes")
  {
    fields.push_back(FieldTitle);
    fields.push_back(FieldTvShowTitle);
    fields.push_back(FieldPlot);
    fields.push_back(FieldVotes);
    fields.push_back(FieldRating);
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
    fields.push_back(FieldTime);
    fields.push_back(FieldDirector);
    fields.push_back(FieldStudio);
    fields.push_back(FieldPlot);
    fields.push_back(FieldDateAdded);
    isVideo = true;
  }
  if (isVideo)
  {
    fields.push_back(FieldVideoResolution);
    fields.push_back(FieldAudioChannels);
    fields.push_back(FieldVideoCodec);
    fields.push_back(FieldAudioCodec);
    fields.push_back(FieldAudioLanguage);
    fields.push_back(FieldSubtitleLanguage);
    fields.push_back(FieldVideoAspectRatio);
  }
  fields.push_back(FieldPlaylist);
  
  return fields;
}

std::vector<SortBy> CSmartPlaylistRule::GetOrders(const CStdString &type)
{
  vector<SortBy> orders;
  orders.push_back(SortByNone);
  if (type == "songs")
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
  }
  else if (type == "tvshows")
  {
    orders.push_back(SortBySortTitle);
    orders.push_back(SortByTvShowStatus);
    orders.push_back(SortByVotes);
    orders.push_back(SortByRating);
    orders.push_back(SortByYear);
    orders.push_back(SortByGenre);
    orders.push_back(SortByNumberOfEpisodes);
    orders.push_back(SortByNumberOfWatchedEpisodes);
    //orders.push_back(SortByPlaycount);
    orders.push_back(SortByPath);
    orders.push_back(SortByStudio);
    orders.push_back(SortByMPAA);
    orders.push_back(SortByDateAdded);
  }
  else if (type == "episodes")
  {
    orders.push_back(SortByTitle);
    orders.push_back(SortByTvShowTitle);
    orders.push_back(SortByVotes);
    orders.push_back(SortByRating);
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
    orders.push_back(SortByStudio);
    orders.push_back(SortByDateAdded);
  }
  orders.push_back(SortByRandom);
	
  return orders;
}

CStdString CSmartPlaylistRule::GetLocalizedOperator(SEARCH_OPERATOR oper)
{
  for (unsigned int i = 0; i < NUM_OPERATORS; i++)
    if (oper == operators[i].op) return g_localizeStrings.Get(operators[i].localizedString);
  return g_localizeStrings.Get(16018);
}

CStdString CSmartPlaylistRule::GetLocalizedRule() const
{
  CStdString rule;
  rule.Format("%s %s %s", GetLocalizedField(m_field).c_str(), GetLocalizedOperator(m_operator).c_str(), m_parameter.c_str());
  return rule;
}

CStdString CSmartPlaylistRule::GetVideoResolutionQuery(void) const
{
  CStdString retVal(" in (select distinct idFile from streamdetails where iVideoWidth ");
  int iRes = atoi(m_parameter.c_str());

  int min, max;
  if (iRes >= 1080)     { min = 1281; max = INT_MAX; }
  else if (iRes >= 720) { min =  961; max = 1280; }
  else if (iRes >= 540) { min =  721; max =  960; }
  else                  { min =    0; max =  720; }

  switch (m_operator)
  {
    case OPERATOR_EQUALS:
      retVal.AppendFormat(">= %i and iVideoWidth <= %i)", min, max);
      break;
    case OPERATOR_DOES_NOT_EQUAL:
      retVal.AppendFormat("< %i or iVideoWidth > %i)", min, max);
      break;
    case OPERATOR_LESS_THAN:
      retVal.AppendFormat("< %i)", min);
      break;
    case OPERATOR_GREATER_THAN:
      retVal.AppendFormat("> %i)", max);
      break;
    default:
      retVal += ")";
      break;
  }
  return retVal;
}

CStdString CSmartPlaylistRule::GetWhereClause(CDatabase &db, const CStdString& strType) const
{
  SEARCH_OPERATOR op = m_operator;
  if ((strType == "tvshows" || strType == "episodes") && m_field == FieldYear)
  { // special case for premiered which is a date rather than a year
    // TODO: SMARTPLAYLISTS do we really need this, or should we just make this field the premiered date and request a date?
    if (op == OPERATOR_EQUALS)
      op = OPERATOR_CONTAINS;
    else if (op == OPERATOR_DOES_NOT_EQUAL)
      op = OPERATOR_DOES_NOT_CONTAIN;
  }
  CStdString operatorString, negate;
  CStdString parameter;
  if (GetFieldType(m_field) == TEXTIN_FIELD)
  {
    CStdStringArray split;
    StringUtils::SplitString(m_parameter, ",", split);
    for (CStdStringArray::iterator it=split.begin(); it!=split.end(); ++it)
    {
      if (!parameter.IsEmpty())
        parameter += ",";
      parameter += db.PrepareSQL("'%s'", (*it).Trim().c_str());
    }
    parameter = " IN (" + parameter + ")";
    if (op == OPERATOR_DOES_NOT_EQUAL)
      negate = " NOT";
  }
  else
  {
    // the comparison piece
    switch (op)
    {
    case OPERATOR_CONTAINS:
      operatorString = " LIKE '%%%s%%'"; break;
    case OPERATOR_DOES_NOT_CONTAIN:
      negate = " NOT"; operatorString = " LIKE '%%%s%%'"; break;
    case OPERATOR_EQUALS:
      operatorString = " LIKE '%s'"; break;
    case OPERATOR_DOES_NOT_EQUAL:
      negate = " NOT"; operatorString = " LIKE '%s'"; break;
    case OPERATOR_STARTS_WITH:
      operatorString = " LIKE '%s%%'"; break;
    case OPERATOR_ENDS_WITH:
      operatorString = " LIKE '%%%s'"; break;
    case OPERATOR_AFTER:
    case OPERATOR_GREATER_THAN:
    case OPERATOR_IN_THE_LAST:
      operatorString = " > '%s'"; break;
    case OPERATOR_BEFORE:
    case OPERATOR_LESS_THAN:
    case OPERATOR_NOT_IN_THE_LAST:
      operatorString = " < '%s'"; break;
    case OPERATOR_TRUE:
      operatorString = " = 1"; break;
    case OPERATOR_FALSE:
      negate = " NOT "; operatorString = " = 0"; break;
    default:
      break;
    }

    parameter = db.PrepareSQL(operatorString.c_str(), m_parameter.c_str());
  }

  if (GetFieldType(m_field) == DATE_FIELD)
  {
    if (m_operator == OPERATOR_IN_THE_LAST || m_operator == OPERATOR_NOT_IN_THE_LAST)
    { // translate time period
      CDateTime date=CDateTime::GetCurrentDateTime();
      CDateTimeSpan span;
      span.SetFromPeriod(m_parameter);
      date-=span;
      parameter = db.PrepareSQL(operatorString.c_str(), date.GetAsDBDate().c_str());
    }
  }
  else if (m_field == FieldTime)
  { // translate time to seconds
    CStdString seconds; seconds.Format("%i", StringUtils::TimeStringToSeconds(m_parameter));
    parameter = db.PrepareSQL(operatorString.c_str(), seconds.c_str());
  }

  // now the query parameter
  CStdString query;
  if (strType == "songs")
  {
    if (m_field == FieldGenre)
      query = negate + " ((strGenre" + parameter + ") or idSong IN (select idSong from genre,exgenresong where exgenresong.idGenre = genre.idGenre and genre.strGenre" + parameter + "))";
    else if (m_field == FieldArtist)
      query = negate + " ((strArtist" + parameter + ") or idSong IN (select idSong from artist,exartistsong where exartistsong.idArtist = artist.idArtist and artist.strArtist" + parameter + "))";
    else if (m_field == FieldAlbumArtist)
      query = negate + "idAlbum IN (SELECT idAlbum FROM album_artist, artist WHERE album_artist.idArtist = artist.idArtist AND artist.strArtist" + parameter + ")";
    else if (m_field == FieldLastPlayed && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = "lastPlayed is NULL or lastPlayed" + parameter;
  }
  else if (strType == "albums")
  {
    if (m_field == FieldGenre)
      query = negate + " (idAlbum in (select song.idAlbum from song join genre on song.idGenre=genre.idGenre where genre.strGenre" + parameter + ") or "
              "idAlbum in (select song.idAlbum from song join exgenresong on song.idSong=exgenresong.idSong join genre on exgenresong.idGenre=genre.idGenre where genre.strGenre" + parameter + "))";
    else if (m_field == FieldArtist)
      query = negate + " (idAlbum in (select song.idAlbum from song join artist on song.idArtist=artist.idArtist where artist.strArtist" + parameter + ") or "
              "idAlbum in (select song.idAlbum from song join exartistsong on song.idSong=exartistsong.idSong join artist on exartistsong.idArtist=artist.idArtist where artist.strArtist" + parameter + "))";
    else if (m_field == FieldAlbumArtist)
      query = negate + "idAlbum IN (SELECT album_artist.idAlbum FROM album_artist, artist WHERE album_artist.idArtist = artist.idArtist AND artist.strArtist" + parameter + ")";
  }
  else if (strType == "movies")
  {
    if (m_field == FieldGenre)
      query = "idMovie" + negate + " in (select idMovie from genrelinkmovie join genre on genre.idGenre=genrelinkmovie.idGenre where genre.strGenre" + parameter + ")";
    else if (m_field == FieldDirector)
      query = "idMovie" + negate + " in (select idMovie from directorlinkmovie join actors on actors.idActor=directorlinkmovie.idDirector where actors.strActor" + parameter + ")";
    else if (m_field == FieldActor)
      query = "idMovie" + negate + " in (select idMovie from actorlinkmovie join actors on actors.idActor=actorlinkmovie.idActor where actors.strActor" + parameter + ")";
    else if (m_field == FieldWriter)
      query = "idMovie" + negate + " in (select idMovie from writerlinkmovie join actors on actors.idActor=writerlinkmovie.idWriter where actors.strActor" + parameter + ")";
    else if (m_field == FieldStudio)
      query = "idMovie" + negate + " in (select idMovie from studiolinkmovie join studio on studio.idStudio=studiolinkmovie.idStudio where studio.strStudio" + parameter + ")";
    else if (m_field == FieldCountry)
      query = "idMovie" + negate + " in (select idMovie from countrylinkmovie join country on country.idCountry=countrylinkmovie.idCountry where country.strCountry" + parameter + ")";
    else if (m_field == FieldTrailer)
      query = negate + GetField(m_field, strType) + "!= ''";
    else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = GetField(m_field, strType) + " is NULL or " + GetField(m_field, strType) + parameter;
    else if (m_field == FieldInProgress)
      query = "idFile " + negate + " in (select idFile from bookmark where type = 1)";
    else if (m_field == FieldSet)
      query = "idMovie" + negate + " in (select idMovie from setlinkmovie join sets on sets.idSet=setlinkmovie.idSet where sets.strSet" + parameter + ")";
  }
  else if (strType == "musicvideos")
  {
    if (m_field == FieldGenre)
      query = "idMVideo" + negate + " in (select idMVideo from genrelinkmusicvideo join genre on genre.idGenre=genrelinkmusicvideo.idGenre where genre.strGenre" + parameter + ")";
    else if (m_field == FieldArtist)
      query = "idMVideo" + negate + " in (select idMVideo from artistlinkmusicvideo join actors on actors.idActor=artistlinkmusicvideo.idArtist where actors.strActor" + parameter + ")";
    else if (m_field == FieldStudio)
      query = "idMVideo" + negate + " in (select idMVideo from studiolinkmusicvideo join studio on studio.idStudio=studiolinkmusicvideo.idStudio where studio.strStudio" + parameter + ")";
    else if (m_field == FieldDirector)
      query = "idMVideo" + negate + " in (select idMVideo from directorlinkmusicvideo join actors on actors.idActor=directorlinkmusicvideo.idDirector where actors.strActor" + parameter + ")";
    else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = GetField(m_field, strType) + " is NULL or " + GetField(m_field, strType) + parameter;
  }
  else if (strType == "tvshows")
  {
    if (m_field == FieldGenre)
      query = "idShow" + negate + " in (select idShow from genrelinktvshow join genre on genre.idGenre=genrelinktvshow.idGenre where genre.strGenre" + parameter + ")";
    else if (m_field == FieldDirector)
      query = "idShow" + negate + " in (select idShow from directorlinktvshow join actors on actors.idActor=directorlinktvshow.idDirector where actors.strActor" + parameter + ")";
    else if (m_field == FieldActor)
      query = "idShow" + negate + " in (select idShow from actorlinktvshow join actors on actors.idActor=actorlinktvshow.idActor where actors.strActor" + parameter + ")";
    else if (m_field == FieldStudio)
      query = "idShow" + negate + " IN (SELECT idShow FROM tvshowview WHERE " + GetField(m_field, strType) + parameter + ")";
    else if (m_field == FieldMPAA)
      query = "idShow" + negate + " IN (SELECT idShow FROM tvshowview WHERE " + GetField(m_field, strType) + parameter + ")";
    else if (m_field == FieldDateAdded && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = "dateAdded is NULL or dateAdded" + parameter;
  }
  else if (strType == "episodes")
  {
    if (m_field == FieldGenre)
      query = "idShow" + negate + " in (select idShow from genrelinktvshow join genre on genre.idGenre=genrelinktvshow.idGenre where genre.strGenre" + parameter + ")";
    else if (m_field == FieldDirector)
      query = "idEpisode" + negate + " in (select idEpisode from directorlinkepisode join actors on actors.idActor=directorlinkepisode.idDirector where actors.strActor" + parameter + ")";
    else if (m_field == FieldActor)
      query = "idEpisode" + negate + " in (select idEpisode from actorlinkepisode join actors on actors.idActor=actorlinkepisode.idActor where actors.strActor" + parameter + ")";
    else if (m_field == FieldWriter)
      query = "idEpisode" + negate + " in (select idEpisode from writerlinkepisode join actors on actors.idActor=writerlinkepisode.idWriter where actors.strActor" + parameter + ")";
    else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = GetField(m_field, strType) + " is NULL or " + GetField(m_field, strType) + parameter;
    else if (m_field == FieldInProgress)
      query = "idFile " + negate + " in (select idFile from bookmark where type = 1)";
    else if (m_field == FieldStudio)
      query = "idEpisode" + negate + " IN (SELECT idEpisode FROM episodeview WHERE strStudio" + parameter + ")";
    else if (m_field == FieldMPAA)
      query = "idEpisode" + negate + " IN (SELECT idEpisode FROM episodeview WHERE mpaa" + parameter + ")";
  }
  if (m_field == FieldVideoResolution)
    query = "idFile" + negate + GetVideoResolutionQuery();
  else if (m_field == FieldAudioChannels)
    query = "idFile" + negate + " in (select distinct idFile from streamdetails where iAudioChannels " + parameter + ")";
  else if (m_field == FieldVideoCodec)
    query = "idFile" + negate + " in (select distinct idFile from streamdetails where strVideoCodec " + parameter + ")";
  else if (m_field == FieldAudioCodec)
    query = "idFile" + negate + " in (select distinct idFile from streamdetails where strAudioCodec " + parameter + ")";
  else if (m_field == FieldAudioLanguage)
    query = "idFile" + negate + " in (select distinct idFile from streamdetails where strAudioLanguage " + parameter + ")";
  else if (m_field == FieldSubtitleLanguage)
    query = "idFile" + negate + " in (select distinct idFile from streamdetails where strSubtitleLanguage " + parameter + ")";
  else if (m_field == FieldVideoAspectRatio)
    query = "idFile" + negate + " in (select distinct idFile from streamdetails where fVideoAspect " + parameter + ")";
  if (m_field == FieldPlaycount && strType != "songs" && strType != "albums")
  { // playcount is stored as NULL or number in video db
    if ((m_operator == OPERATOR_EQUALS && m_parameter == "0") ||
        (m_operator == OPERATOR_DOES_NOT_EQUAL && m_parameter != "0") ||
        (m_operator == OPERATOR_LESS_THAN))
    {
      CStdString field = GetField(FieldPlaycount, strType);
      query = field + " is NULL or " + field + parameter;
    }
  }
  if (query.IsEmpty() && m_field != FieldNone)
    query = GetField(m_field,strType) + negate + parameter;
  // if we fail to get a dbfield, we empty query so it doesn't fail
  if (query.Equals(negate + parameter))
    query = "";
  return query;
}

CStdString CSmartPlaylistRule::GetField(Field field, const CStdString& type)
{
  return DatabaseUtils::GetField(field, DatabaseUtils::MediaTypeFromString(type), DatabaseQueryPartWhere);
}

CSmartPlaylist::CSmartPlaylist()
{
  m_matchAllRules = true;
  m_limit = 0;
  m_orderField = SortByNone;
  m_orderAscending = true;
  m_playlistType = "songs"; // sane default
}

TiXmlElement *CSmartPlaylist::OpenAndReadName(const CStdString &path)
{
  CFileStream file;
  if (!file.Open(path))
  {
    CLog::Log(LOGERROR, "Error loading Smart playlist %s (failed to read file)", path.c_str());
    return NULL;
  }

  m_xmlDoc.Clear();
  file >> m_xmlDoc;

  if (m_xmlDoc.Error())
  {
    CLog::Log(LOGERROR, "Error loading Smart playlist (failed to parse xml: %s)", m_xmlDoc.ErrorDesc());
    return NULL;
  }

  TiXmlElement *root = m_xmlDoc.RootElement();
  if (!root || strcmpi(root->Value(),"smartplaylist") != 0)
  {
    CLog::Log(LOGERROR, "Error loading Smart playlist %s", path.c_str());
    return NULL;
  }
  // load the playlist type
  const char* type = root->Attribute("type");
  if (type)
    m_playlistType = type;
  // backward compatibility:
  if (m_playlistType == "music")
    m_playlistType = "songs";
  if (m_playlistType == "video")
    m_playlistType = "musicvideos";

  // load the playlist name
  TiXmlHandle name = ((TiXmlHandle)root->FirstChild("name")).FirstChild();
  if (name.Node())
    m_playlistName = name.Node()->Value();
  else
  {
    m_playlistName = CUtil::GetTitleFromPath(path);
    if (URIUtils::GetExtension(m_playlistName) == ".xsp")
      URIUtils::RemoveExtension(m_playlistName);
  }
  return root;
}

bool CSmartPlaylist::Load(const CStdString &path)
{
  TiXmlElement *root = OpenAndReadName(path);
  if (!root)
    return false;

  // encoding:
  CStdString encoding;
  XMLUtils::GetEncoding(&m_xmlDoc, encoding);
  
  // from here we decode from XML
  return LoadFromXML(root, encoding);
}

bool CSmartPlaylist::LoadFromXML(TiXmlElement *root, const CStdString &encoding)
{
  if (!root)
    return false;

  TiXmlHandle match = ((TiXmlHandle)root->FirstChild("match")).FirstChild();
  if (match.Node())
    m_matchAllRules = strcmpi(match.Node()->Value(), "all") == 0;
  // now the rules
  TiXmlElement *rule = root->FirstChildElement("rule");
  while (rule)
  {
    // format is:
    // <rule field="Genre" operator="contains">parameter</rule>
    const char *field = rule->Attribute("field");
    const char *oper = rule->Attribute("operator");
    TiXmlNode *parameter = rule->FirstChild();
    if (field && oper)
    { // valid rule
      CStdString utf8Parameter;
      if (parameter)
      {
        if (encoding.IsEmpty()) // utf8
          utf8Parameter = parameter->Value();
        else
          g_charsetConverter.stringCharsetToUtf8(encoding, parameter->Value(), utf8Parameter);
      }
      CSmartPlaylistRule rule;
      rule.TranslateStrings(field, oper, utf8Parameter.c_str());
      m_playlistRules.push_back(rule);
    }
    rule = rule->NextSiblingElement("rule");
  }
  // now any limits
  // format is <limit>25</limit>
  TiXmlHandle limit = ((TiXmlHandle)root->FirstChild("limit")).FirstChild();
  if (limit.Node())
    m_limit = atoi(limit.Node()->Value());
  // and order
  // format is <order direction="ascending">field</order>
  TiXmlElement *order = root->FirstChildElement("order");
  if (order && order->FirstChild())
  {
    const char *direction = order->Attribute("direction");
    if (direction)
      m_orderAscending = strcmpi(direction, "ascending") == 0;
    m_orderField = CSmartPlaylistRule::TranslateOrder(order->FirstChild()->Value());
  }
  return true;
}

bool CSmartPlaylist::Save(const CStdString &path)
{
  CXBMCTinyXML doc;
  TiXmlDeclaration decl("1.0", "UTF-8", "yes");
  doc.InsertEndChild(decl);

  TiXmlElement xmlRootElement("smartplaylist");
  xmlRootElement.SetAttribute("type",m_playlistType.c_str());
  TiXmlNode *pRoot = doc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;
  // add the <name> tag
  TiXmlText name(m_playlistName.c_str());
  TiXmlElement nodeName("name");
  nodeName.InsertEndChild(name);
  pRoot->InsertEndChild(nodeName);
  // add the <match> tag
  TiXmlText match(m_matchAllRules ? "all" : "one");
  TiXmlElement nodeMatch("match");
  nodeMatch.InsertEndChild(match);
  pRoot->InsertEndChild(nodeMatch);
  // add <rule> tags
  for (vector<CSmartPlaylistRule>::iterator it = m_playlistRules.begin(); it != m_playlistRules.end(); ++it)
  {
    pRoot->InsertEndChild((*it).GetAsElement());
  }
  // add <limit> tag
  if (m_limit)
  {
    CStdString limitFormat;
    limitFormat.Format("%i", m_limit);
    TiXmlText limit(limitFormat);
    TiXmlElement nodeLimit("limit");
    nodeLimit.InsertEndChild(limit);
    pRoot->InsertEndChild(nodeLimit);
  }
  // add <order> tag
  if (m_orderField != SortByNone)
  {
    TiXmlText order(CSmartPlaylistRule::TranslateOrder(m_orderField).c_str());
    TiXmlElement nodeOrder("order");
    nodeOrder.SetAttribute("direction", m_orderAscending ? "ascending" : "descending");
    nodeOrder.InsertEndChild(order);
    pRoot->InsertEndChild(nodeOrder);
  }
  return doc.SaveFile(path);
}

void CSmartPlaylist::SetName(const CStdString &name)
{
  m_playlistName = name;
}

void CSmartPlaylist::SetType(const CStdString &type)
{
  m_playlistType = type;
}

void CSmartPlaylist::AddRule(const CSmartPlaylistRule &rule)
{
  m_playlistRules.push_back(rule);
}

CStdString CSmartPlaylist::GetWhereClause(CDatabase &db, set<CStdString> &referencedPlaylists) const
{
  CStdString rule, currentRule;
  for (vector<CSmartPlaylistRule>::const_iterator it = m_playlistRules.begin(); it != m_playlistRules.end(); ++it)
  {
    if (it != m_playlistRules.begin())
      rule += m_matchAllRules ? " AND " : " OR ";
    rule += "(";
    CStdString currentRule;
    if (it->m_field == FieldPlaylist)
    {
      CStdString playlistFile = CSmartPlaylistDirectory::GetPlaylistByName(it->m_parameter, GetType());
      if (!playlistFile.IsEmpty() && referencedPlaylists.find(playlistFile) == referencedPlaylists.end())
      {
        referencedPlaylists.insert(playlistFile);
        CSmartPlaylist playlist;
        playlist.Load(playlistFile);
        CStdString playlistQuery;
        // only playlists of same type will be part of the query
        if (playlist.GetType().Equals(GetType()) || (playlist.GetType().Equals("mixed") && (GetType() == "songs" || GetType() == "musicvideos")) || playlist.GetType().IsEmpty())
        {
          playlist.SetType(GetType());
          playlistQuery = playlist.GetWhereClause(db, referencedPlaylists);
        }
        if (playlist.GetType().Equals(GetType()))
        {
          if (it->m_operator == CSmartPlaylistRule::OPERATOR_DOES_NOT_EQUAL)
            currentRule.Format("NOT (%s)", playlistQuery.c_str());
          else
            currentRule = playlistQuery;
        }
      }
    }
    else
      currentRule = (*it).GetWhereClause(db, GetType());
    // if we don't get a rule, we add '1' or '0' so the query is still valid and doesn't fail
    if (currentRule.IsEmpty())
      currentRule = m_matchAllRules ? "'1'" : "'0'";
    rule += currentRule;
    rule += ")";
  }
  return rule;
}

const vector<CSmartPlaylistRule> &CSmartPlaylist::GetRules() const
{
  return m_playlistRules;
}

CStdString CSmartPlaylist::GetSaveLocation() const
{
  if (m_playlistType == "songs" || m_playlistType == "albums")
    return "music";
  else if (m_playlistType == "mixed")
    return "mixed";
  // all others are video
  return "video";
}

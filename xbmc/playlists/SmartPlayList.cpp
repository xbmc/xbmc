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
  CSmartPlaylistRule::DATABASE_FIELD field;
  CSmartPlaylistRule::FIELD_TYPE type;
  int localizedString;
} translateField;

static const translateField fields[] = { { "none", CSmartPlaylistRule::FIELD_NONE, CSmartPlaylistRule::TEXT_FIELD, 231 },
                                         { "genre", CSmartPlaylistRule::FIELD_GENRE, CSmartPlaylistRule::BROWSEABLE_FIELD, 515 },
                                         { "album", CSmartPlaylistRule::FIELD_ALBUM, CSmartPlaylistRule::BROWSEABLE_FIELD, 558 },
                                         { "albumartist", CSmartPlaylistRule::FIELD_ALBUMARTIST, CSmartPlaylistRule::BROWSEABLE_FIELD, 566 },
                                         { "artist", CSmartPlaylistRule::FIELD_ARTIST, CSmartPlaylistRule::BROWSEABLE_FIELD, 557 },
                                         { "title", CSmartPlaylistRule::FIELD_TITLE, CSmartPlaylistRule::TEXT_FIELD, 556 },
                                         { "year", CSmartPlaylistRule::FIELD_YEAR, CSmartPlaylistRule::NUMERIC_FIELD, 562 },
                                         { "time", CSmartPlaylistRule::FIELD_TIME, CSmartPlaylistRule::SECONDS_FIELD, 180 },
                                         { "tracknumber", CSmartPlaylistRule::FIELD_TRACKNUMBER, CSmartPlaylistRule::NUMERIC_FIELD, 554 },
                                         { "filename", CSmartPlaylistRule::FIELD_FILENAME, CSmartPlaylistRule::TEXT_FIELD, 561 },
                                         { "path", CSmartPlaylistRule::FIELD_PATH, CSmartPlaylistRule::BROWSEABLE_FIELD, 573 },
                                         { "playcount", CSmartPlaylistRule::FIELD_PLAYCOUNT, CSmartPlaylistRule::NUMERIC_FIELD, 567 },
                                         { "lastplayed", CSmartPlaylistRule::FIELD_LASTPLAYED, CSmartPlaylistRule::DATE_FIELD, 568 },
                                         { "inprogress", CSmartPlaylistRule::FIELD_INPROGRESS, CSmartPlaylistRule::BOOLEAN_FIELD, 575 },
                                         { "rating", CSmartPlaylistRule::FIELD_RATING, CSmartPlaylistRule::NUMERIC_FIELD, 563 },
                                         { "comment", CSmartPlaylistRule::FIELD_COMMENT, CSmartPlaylistRule::TEXT_FIELD, 569 },
                                         { "dateadded", CSmartPlaylistRule::FIELD_DATEADDED, CSmartPlaylistRule::DATE_FIELD, 570 },
                                         { "plot", CSmartPlaylistRule::FIELD_PLOT, CSmartPlaylistRule::TEXT_FIELD, 207 },
                                         { "plotoutline", CSmartPlaylistRule::FIELD_PLOTOUTLINE, CSmartPlaylistRule::TEXT_FIELD, 203 },
                                         { "tagline", CSmartPlaylistRule::FIELD_TAGLINE, CSmartPlaylistRule::TEXT_FIELD, 202 },
                                         { "mpaarating", CSmartPlaylistRule::FIELD_MPAA, CSmartPlaylistRule::TEXT_FIELD, 20074 },
                                         { "top250", CSmartPlaylistRule::FIELD_TOP250, CSmartPlaylistRule::NUMERIC_FIELD, 13409 },
                                         { "status", CSmartPlaylistRule::FIELD_STATUS, CSmartPlaylistRule::TEXT_FIELD, 126 },
                                         { "votes", CSmartPlaylistRule::FIELD_VOTES, CSmartPlaylistRule::TEXT_FIELD, 205 },
                                         { "director", CSmartPlaylistRule::FIELD_DIRECTOR, CSmartPlaylistRule::BROWSEABLE_FIELD, 20339 },
                                         { "actor", CSmartPlaylistRule::FIELD_ACTOR, CSmartPlaylistRule::BROWSEABLE_FIELD, 20337 },
                                         { "studio", CSmartPlaylistRule::FIELD_STUDIO, CSmartPlaylistRule::BROWSEABLE_FIELD, 572 },
                                         { "country", CSmartPlaylistRule::FIELD_COUNTRY, CSmartPlaylistRule::BROWSEABLE_FIELD, 574 },
                                         { "numepisodes", CSmartPlaylistRule::FIELD_NUMEPISODES, CSmartPlaylistRule::NUMERIC_FIELD, 20360 },
                                         { "numwatched", CSmartPlaylistRule::FIELD_NUMWATCHED, CSmartPlaylistRule::NUMERIC_FIELD, 21441 },
                                         { "writers", CSmartPlaylistRule::FIELD_WRITER, CSmartPlaylistRule::BROWSEABLE_FIELD, 20417 },
                                         { "airdate", CSmartPlaylistRule::FIELD_AIRDATE, CSmartPlaylistRule::DATE_FIELD, 20416 },
                                         { "episode", CSmartPlaylistRule::FIELD_EPISODE, CSmartPlaylistRule::NUMERIC_FIELD, 20359 },
                                         { "season", CSmartPlaylistRule::FIELD_SEASON, CSmartPlaylistRule::NUMERIC_FIELD, 20373 },
                                         { "tvshow", CSmartPlaylistRule::FIELD_TVSHOWTITLE, CSmartPlaylistRule::BROWSEABLE_FIELD, 20364 },
                                         { "episodetitle", CSmartPlaylistRule::FIELD_EPISODETITLE, CSmartPlaylistRule::TEXT_FIELD, 21442 },
                                         { "review", CSmartPlaylistRule::FIELD_REVIEW, CSmartPlaylistRule::TEXT_FIELD, 183 },
                                         { "themes", CSmartPlaylistRule::FIELD_THEMES, CSmartPlaylistRule::TEXT_FIELD, 21895 },
                                         { "moods", CSmartPlaylistRule::FIELD_MOODS, CSmartPlaylistRule::TEXT_FIELD, 175 },
                                         { "styles", CSmartPlaylistRule::FIELD_STYLES, CSmartPlaylistRule::TEXT_FIELD, 176 },
                                         { "type", CSmartPlaylistRule::FIELD_ALBUMTYPE, CSmartPlaylistRule::TEXT_FIELD, 564 },
                                         { "label", CSmartPlaylistRule::FIELD_LABEL, CSmartPlaylistRule::TEXT_FIELD, 21899 },
                                         { "hastrailer", CSmartPlaylistRule::FIELD_HASTRAILER, CSmartPlaylistRule::BOOLEAN_FIELD, 20423 },
                                         { "videoresolution", CSmartPlaylistRule::FIELD_VIDEORESOLUTION, CSmartPlaylistRule::NUMERIC_FIELD, 21443 },
                                         { "audiochannels", CSmartPlaylistRule::FIELD_AUDIOCHANNELS, CSmartPlaylistRule::NUMERIC_FIELD, 21444 },
                                         { "videocodec", CSmartPlaylistRule::FIELD_VIDEOCODEC, CSmartPlaylistRule::TEXTIN_FIELD, 21445 },
                                         { "audiocodec", CSmartPlaylistRule::FIELD_AUDIOCODEC, CSmartPlaylistRule::TEXTIN_FIELD, 21446 },
                                         { "audiolanguage", CSmartPlaylistRule::FIELD_AUDIOLANGUAGE, CSmartPlaylistRule::TEXTIN_FIELD, 21447 },
                                         { "subtitlelanguage", CSmartPlaylistRule::FIELD_SUBTITLELANGUAGE, CSmartPlaylistRule::TEXTIN_FIELD, 21448 },
                                         { "videoaspect", CSmartPlaylistRule::FIELD_VIDEOASPECT, CSmartPlaylistRule::NUMERIC_FIELD, 21374 },
                                         { "random", CSmartPlaylistRule::FIELD_RANDOM, CSmartPlaylistRule::TEXT_FIELD, 590 },
                                         { "playlist", CSmartPlaylistRule::FIELD_PLAYLIST, CSmartPlaylistRule::PLAYLIST_FIELD, 559 },
                                         { "set", CSmartPlaylistRule::FIELD_SET, CSmartPlaylistRule::BROWSEABLE_FIELD, 20457 }
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
  m_field = FIELD_NONE;
  m_operator = OPERATOR_CONTAINS;
  m_parameter = "";
}

void CSmartPlaylistRule::TranslateStrings(const char *field, const char *oper, const char *parameter)
{
  m_field = TranslateField(field);
  m_operator = TranslateOperator(oper);
  m_parameter = parameter;
}

TiXmlElement CSmartPlaylistRule::GetAsElement()
{
  TiXmlElement rule("rule");
  TiXmlText parameter(m_parameter.c_str());
  rule.InsertEndChild(parameter);
  rule.SetAttribute("field", TranslateField(m_field).c_str());
  rule.SetAttribute("operator", TranslateOperator(m_operator).c_str());
  return rule;
}

CSmartPlaylistRule::DATABASE_FIELD CSmartPlaylistRule::TranslateField(const char *field)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (strcmpi(field, fields[i].string) == 0) return fields[i].field;
  return FIELD_NONE;
}

CStdString CSmartPlaylistRule::TranslateField(DATABASE_FIELD field)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return fields[i].string;
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

CStdString CSmartPlaylistRule::GetLocalizedField(DATABASE_FIELD field)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return g_localizeStrings.Get(fields[i].localizedString);
  return g_localizeStrings.Get(16018);
}

CSmartPlaylistRule::FIELD_TYPE CSmartPlaylistRule::GetFieldType(DATABASE_FIELD field)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return fields[i].type;
  return TEXT_FIELD;
}

vector<CSmartPlaylistRule::DATABASE_FIELD> CSmartPlaylistRule::GetFields(const CStdString &type, bool sortOrders)
{
  vector<DATABASE_FIELD> fields;
  if (sortOrders)
    fields.push_back(FIELD_NONE);
  bool isVideo = false;
  if (type == "songs")
  {
    fields.push_back(FIELD_GENRE);
    fields.push_back(FIELD_ALBUM);
    fields.push_back(FIELD_ARTIST);
    fields.push_back(FIELD_ALBUMARTIST);
    fields.push_back(FIELD_TITLE);
    fields.push_back(FIELD_YEAR);
    fields.push_back(FIELD_TIME);
    fields.push_back(FIELD_TRACKNUMBER);
    fields.push_back(FIELD_FILENAME);
    fields.push_back(FIELD_PATH);
    fields.push_back(FIELD_PLAYCOUNT);
    fields.push_back(FIELD_LASTPLAYED);
    fields.push_back(FIELD_RATING);
    fields.push_back(FIELD_COMMENT);
//    fields.push_back(FIELD_DATEADDED);  // no date added yet in db
  }
  else if (type == "albums")
  {
    fields.push_back(FIELD_GENRE);
    fields.push_back(FIELD_ALBUM);
    fields.push_back(FIELD_ARTIST);        // any artist
    fields.push_back(FIELD_ALBUMARTIST);  // album artist
    fields.push_back(FIELD_YEAR);
    fields.push_back(FIELD_REVIEW);
    fields.push_back(FIELD_THEMES);
    fields.push_back(FIELD_MOODS);
    fields.push_back(FIELD_STYLES);
    fields.push_back(FIELD_ALBUMTYPE);
    fields.push_back(FIELD_LABEL);
    fields.push_back(FIELD_RATING);
  }
  else if (type == "tvshows")
  {
    fields.push_back(FIELD_TVSHOWTITLE);
    fields.push_back(FIELD_PLOT);
    fields.push_back(FIELD_STATUS);
    fields.push_back(FIELD_VOTES);
    fields.push_back(FIELD_RATING);
    fields.push_back(FIELD_YEAR);
    fields.push_back(FIELD_GENRE);
    if (!sortOrders)
    {
      fields.push_back(FIELD_DIRECTOR);
      fields.push_back(FIELD_ACTOR);
    }
    fields.push_back(FIELD_NUMEPISODES);
    fields.push_back(FIELD_NUMWATCHED);
    fields.push_back(FIELD_PLAYCOUNT);
    fields.push_back(FIELD_PATH);
    fields.push_back(FIELD_STUDIO);
    fields.push_back(FIELD_MPAA);
//    fields.push_back(FIELD_DATEADDED);  // no date added yet in db
  }
  else if (type == "episodes")
  {
    fields.push_back(FIELD_EPISODETITLE);
    fields.push_back(FIELD_TVSHOWTITLE);
    fields.push_back(FIELD_PLOT);
    fields.push_back(FIELD_VOTES);
    fields.push_back(FIELD_RATING);
    fields.push_back(FIELD_TIME);
    fields.push_back(FIELD_WRITER);
    fields.push_back(FIELD_AIRDATE);
    fields.push_back(FIELD_PLAYCOUNT);
    fields.push_back(FIELD_LASTPLAYED);
    if (!sortOrders)
    {
      fields.push_back(FIELD_INPROGRESS);
      fields.push_back(FIELD_GENRE);
    }
    fields.push_back(FIELD_YEAR); // premiered
    fields.push_back(FIELD_DIRECTOR);
    if (!sortOrders)
      fields.push_back(FIELD_ACTOR);
    fields.push_back(FIELD_EPISODE);
    fields.push_back(FIELD_SEASON);
    fields.push_back(FIELD_FILENAME);
    fields.push_back(FIELD_PATH);
    fields.push_back(FIELD_STUDIO);
    fields.push_back(FIELD_MPAA);
    isVideo = true;
//    fields.push_back(FIELD_DATEADDED);  // no date added yet in db
  }
  else if (type == "movies")
  {
    fields.push_back(FIELD_TITLE);
    fields.push_back(FIELD_PLOT);
    fields.push_back(FIELD_PLOTOUTLINE);
    fields.push_back(FIELD_TAGLINE);
    fields.push_back(FIELD_VOTES);
    fields.push_back(FIELD_RATING);
    fields.push_back(FIELD_TIME);
    fields.push_back(FIELD_WRITER);
    fields.push_back(FIELD_PLAYCOUNT);
    fields.push_back(FIELD_LASTPLAYED);
    if (!sortOrders)
      fields.push_back(FIELD_INPROGRESS);
    fields.push_back(FIELD_GENRE);
    fields.push_back(FIELD_COUNTRY);
    fields.push_back(FIELD_YEAR); // premiered
    fields.push_back(FIELD_DIRECTOR);
    if (!sortOrders)
      fields.push_back(FIELD_ACTOR);
    fields.push_back(FIELD_MPAA);
    fields.push_back(FIELD_TOP250);
    fields.push_back(FIELD_STUDIO);
    fields.push_back(FIELD_HASTRAILER);
    fields.push_back(FIELD_FILENAME);
    fields.push_back(FIELD_PATH);
    if (!sortOrders)
      fields.push_back(FIELD_SET);
    isVideo = true;
//    fields.push_back(FIELD_DATEADDED);  // no date added yet in db
  }
  else if (type == "musicvideos")
  {
    fields.push_back(FIELD_TITLE);
    fields.push_back(FIELD_GENRE);
    fields.push_back(FIELD_ALBUM);
    fields.push_back(FIELD_YEAR);
    fields.push_back(FIELD_ARTIST);
    fields.push_back(FIELD_FILENAME);
    fields.push_back(FIELD_PATH);
    fields.push_back(FIELD_PLAYCOUNT);
    fields.push_back(FIELD_LASTPLAYED);
    fields.push_back(FIELD_TIME);
    fields.push_back(FIELD_DIRECTOR);
    fields.push_back(FIELD_STUDIO);
    fields.push_back(FIELD_PLOT);
    isVideo = true;
//    fields.push_back(FIELD_DATEADDED);  // no date added yet in db
  }
  if (isVideo)
  {
    fields.push_back(FIELD_VIDEORESOLUTION);
    fields.push_back(FIELD_AUDIOCHANNELS);
    fields.push_back(FIELD_VIDEOCODEC);
    fields.push_back(FIELD_AUDIOCODEC);
    fields.push_back(FIELD_AUDIOLANGUAGE);
    fields.push_back(FIELD_SUBTITLELANGUAGE);
    fields.push_back(FIELD_VIDEOASPECT);
  }
  if (sortOrders)
    fields.push_back(FIELD_RANDOM);
  else
    fields.push_back(FIELD_PLAYLIST);
  return fields;
}

CStdString CSmartPlaylistRule::GetLocalizedOperator(SEARCH_OPERATOR oper)
{
  for (unsigned int i = 0; i < NUM_OPERATORS; i++)
    if (oper == operators[i].op) return g_localizeStrings.Get(operators[i].localizedString);
  return g_localizeStrings.Get(16018);
}

CStdString CSmartPlaylistRule::GetLocalizedRule()
{
  CStdString rule;
  rule.Format("%s %s %s", GetLocalizedField(m_field).c_str(), GetLocalizedOperator(m_operator).c_str(), m_parameter.c_str());
  return rule;
}

CStdString CSmartPlaylistRule::GetVideoResolutionQuery(void)
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

CStdString CSmartPlaylistRule::GetWhereClause(CDatabase &db, const CStdString& strType)
{
  SEARCH_OPERATOR op = m_operator;
  if ((strType == "tvshows" || strType == "episodes") && m_field == FIELD_YEAR)
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

  if (m_field == FIELD_LASTPLAYED || m_field == FIELD_AIRDATE)
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
  else if (m_field == FIELD_TIME)
  { // translate time to seconds
    CStdString seconds; seconds.Format("%i", StringUtils::TimeStringToSeconds(m_parameter));
    parameter = db.PrepareSQL(operatorString.c_str(), seconds.c_str());
  }

  // now the query parameter
  CStdString query;
  if (strType == "songs")
  {
    if (m_field == FIELD_GENRE)
      query = negate + " ((strGenre" + parameter + ") or idSong IN (select idSong from genre,exgenresong where exgenresong.idGenre = genre.idGenre and genre.strGenre" + parameter + "))";
    else if (m_field == FIELD_ARTIST)
      query = negate + " ((strArtist" + parameter + ") or idSong IN (select idSong from artist,exartistsong where exartistsong.idArtist = artist.idArtist and artist.strArtist" + parameter + "))";
    else if (m_field == FIELD_ALBUMARTIST)
      query = negate + " (idalbum in (select idalbum from artist,album where album.idArtist=artist.idArtist and artist.strArtist" + parameter + ") or idalbum in (select idalbum from artist,exartistalbum where exartistalbum.idArtist = artist.idArtist and artist.strArtist" + parameter + "))";
    else if (m_field == FIELD_LASTPLAYED && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = "lastPlayed is NULL or lastPlayed" + parameter;
  }
  else if (strType == "albums")
  {
    if (m_field == FIELD_GENRE)
      query = negate + " (idAlbum in (select song.idAlbum from song join genre on song.idGenre=genre.idGenre where genre.strGenre" + parameter + ") or "
              "idAlbum in (select song.idAlbum from song join exgenresong on song.idSong=exgenresong.idSong join genre on exgenresong.idGenre=genre.idGenre where genre.strGenre" + parameter + "))";
    else if (m_field == FIELD_ARTIST)
      query = negate + " (idAlbum in (select song.idAlbum from song join artist on song.idArtist=artist.idArtist where artist.strArtist" + parameter + ") or "
              "idAlbum in (select song.idAlbum from song join exartistsong on song.idSong=exartistsong.idSong join artist on exartistsong.idArtist=artist.idArtist where artist.strArtist" + parameter + "))";
    else if (m_field == FIELD_ALBUMARTIST)
      query = negate + " (idalbum in (select idalbum from artist,album where album.idArtist=artist.idArtist and artist.strArtist" + parameter + ") or idalbum in (select idalbum from artist,exartistalbum where exartistalbum.idArtist = artist.idArtist and artist.strArtist" + parameter + "))";
  }
  else if (strType == "movies")
  {
    if (m_field == FIELD_GENRE)
      query = "idMovie" + negate + " in (select idMovie from genrelinkmovie join genre on genre.idGenre=genrelinkmovie.idGenre where genre.strGenre" + parameter + ")";
    else if (m_field == FIELD_DIRECTOR)
      query = "idMovie" + negate + " in (select idMovie from directorlinkmovie join actors on actors.idActor=directorlinkmovie.idDirector where actors.strActor" + parameter + ")";
    else if (m_field == FIELD_ACTOR)
      query = "idMovie" + negate + " in (select idMovie from actorlinkmovie join actors on actors.idActor=actorlinkmovie.idActor where actors.strActor" + parameter + ")";
    else if (m_field == FIELD_WRITER)
      query = "idMovie" + negate + " in (select idMovie from writerlinkmovie join actors on actors.idActor=writerlinkmovie.idWriter where actors.strActor" + parameter + ")";
    else if (m_field == FIELD_STUDIO)
      query = "idMovie" + negate + " in (select idMovie from studiolinkmovie join studio on studio.idStudio=studiolinkmovie.idStudio where studio.strStudio" + parameter + ")";
    else if (m_field == FIELD_COUNTRY)
      query = "idMovie" + negate + " in (select idMovie from countrylinkmovie join country on country.idCountry=countrylinkmovie.idCountry where country.strCountry" + parameter + ")";
    else if (m_field == FIELD_HASTRAILER)
      query = negate + GetDatabaseField(m_field, strType) + "!= ''";
    else if (m_field == FIELD_LASTPLAYED && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = "lastPlayed is NULL or lastPlayed" + parameter;
    else if (m_field == FIELD_INPROGRESS)
      query = "idFile " + negate + " in (select idFile from bookmark where type = 1)";
    else if (m_field == FIELD_SET)
      query = "idMovie" + negate + " in (select idMovie from setlinkmovie join sets on sets.idSet=setlinkmovie.idSet where sets.strSet" + parameter + ")";
  }
  else if (strType == "musicvideos")
  {
    if (m_field == FIELD_GENRE)
      query = "idMVideo" + negate + " in (select idMVideo from genrelinkmusicvideo join genre on genre.idGenre=genrelinkmusicvideo.idGenre where genre.strGenre" + parameter + ")";
    else if (m_field == FIELD_ARTIST)
      query = "idMVideo" + negate + " in (select idMVideo from artistlinkmusicvideo join actors on actors.idActor=artistlinkmusicvideo.idArtist where actors.strActor" + parameter + ")";
    else if (m_field == FIELD_STUDIO)
      query = "idMVideo" + negate + " in (select idMVideo from studiolinkmusicvideo join studio on studio.idStudio=studiolinkmusicvideo.idStudio where studio.strStudio" + parameter + ")";
    else if (m_field == FIELD_DIRECTOR)
      query = "idMVideo" + negate + " in (select idMVideo from directorlinkmusicvideo join actors on actors.idActor=directorlinkmusicvideo.idDirector where actors.strActor" + parameter + ")";
    else if (m_field == FIELD_LASTPLAYED && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = "lastPlayed is NULL or lastPlayed" + parameter;
  }
  else if (strType == "tvshows")
  {
    if (m_field == FIELD_GENRE)
      query = "idShow" + negate + " in (select idShow from genrelinktvshow join genre on genre.idGenre=genrelinktvshow.idGenre where genre.strGenre" + parameter + ")";
    else if (m_field == FIELD_DIRECTOR)
      query = "idShow" + negate + " in (select idShow from directorlinktvshow join actors on actors.idActor=directorlinktvshow.idDirector where actors.strActor" + parameter + ")";
    else if (m_field == FIELD_ACTOR)
      query = "idShow" + negate + " in (select idShow from actorlinktvshow join actors on actors.idActor=actorlinktvshow.idActor where actors.strActor" + parameter + ")";
    else if (m_field == FIELD_STUDIO)
      query = "idShow" + negate + " IN (SELECT idShow FROM tvshowview WHERE " + GetDatabaseField(m_field, strType) + parameter + ")";
    else if (m_field == FIELD_MPAA)
      query = "idShow" + negate + " IN (SELECT idShow FROM tvshowview WHERE " + GetDatabaseField(m_field, strType) + parameter + ")";
  }
  else if (strType == "episodes")
  {
    if (m_field == FIELD_GENRE)
      query = "idShow" + negate + " in (select idShow from genrelinktvshow join genre on genre.idGenre=genrelinktvshow.idGenre where genre.strGenre" + parameter + ")";
    else if (m_field == FIELD_DIRECTOR)
      query = "idEpisode" + negate + " in (select idEpisode from directorlinkepisode join actors on actors.idActor=directorlinkepisode.idDirector where actors.strActor" + parameter + ")";
    else if (m_field == FIELD_ACTOR)
      query = "idEpisode" + negate + " in (select idEpisode from actorlinkepisode join actors on actors.idActor=actorlinkepisode.idActor where actors.strActor" + parameter + ")";
    else if (m_field == FIELD_WRITER)
      query = "idEpisode" + negate + " in (select idEpisode from writerlinkepisode join actors on actors.idActor=writerlinkepisode.idWriter where actors.strActor" + parameter + ")";
    else if (m_field == FIELD_LASTPLAYED && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = "lastPlayed is NULL or lastPlayed" + parameter;
    else if (m_field == FIELD_INPROGRESS)
      query = "idFile " + negate + " in (select idFile from bookmark where type = 1)";
    else if (m_field == FIELD_STUDIO)
      query = "idEpisode" + negate + " IN (SELECT idEpisode FROM episodeview WHERE strStudio" + parameter + ")";
    else if (m_field == FIELD_MPAA)
      query = "idEpisode" + negate + " IN (SELECT idEpisode FROM episodeview WHERE mpaa" + parameter + ")";
  }
  if (m_field == FIELD_VIDEORESOLUTION)
    query = "idFile" + negate + GetVideoResolutionQuery();
  else if (m_field == FIELD_AUDIOCHANNELS)
    query = "idFile" + negate + " in (select distinct idFile from streamdetails where iAudioChannels " + parameter + ")";
  else if (m_field == FIELD_VIDEOCODEC)
    query = "idFile" + negate + " in (select distinct idFile from streamdetails where strVideoCodec " + parameter + ")";
  else if (m_field == FIELD_AUDIOCODEC)
    query = "idFile" + negate + " in (select distinct idFile from streamdetails where strAudioCodec " + parameter + ")";
  else if (m_field == FIELD_AUDIOLANGUAGE)
    query = "idFile" + negate + " in (select distinct idFile from streamdetails where strAudioLanguage " + parameter + ")";
  else if (m_field == FIELD_SUBTITLELANGUAGE)
    query = "idFile" + negate + " in (select distinct idFile from streamdetails where strSubtitleLanguage " + parameter + ")";
  else if (m_field == FIELD_VIDEOASPECT)
    query = "idFile" + negate + " in (select distinct idFile from streamdetails where fVideoAspect " + parameter + ")";
  else if (m_field == FIELD_PLAYLIST)
  { // playlist field - grab our playlist and add to our where clause
    CStdString playlistFile = CSmartPlaylistDirectory::GetPlaylistByName(m_parameter, strType);
    if (!playlistFile.IsEmpty())
    {
      CSmartPlaylist playlist;
      playlist.Load(playlistFile);
      CStdString playlistQuery;
      // only playlists of same type will be part of the query
      if (playlist.GetType().Equals(strType) || (playlist.GetType().Equals("mixed") && (strType == "songs" || strType == "musicvideos")) || playlist.GetType().IsEmpty())
      {
        playlist.SetType(strType);
        playlistQuery = playlist.GetWhereClause(db, false);
      }
      if (m_operator == OPERATOR_DOES_NOT_EQUAL && playlist.GetType().Equals(strType))
        query.Format("NOT (%s)", playlistQuery.c_str());
      else if (m_operator == OPERATOR_EQUALS && playlist.GetType().Equals(strType))
        query = playlistQuery;
    }
  }
  if (m_field == FIELD_PLAYCOUNT && strType != "songs" && strType != "albums")
  { // playcount is stored as NULL or number in video db
    if ((m_operator == OPERATOR_EQUALS && m_parameter == "0") ||
        (m_operator == OPERATOR_DOES_NOT_EQUAL && m_parameter != "0") ||
        (m_operator == OPERATOR_LESS_THAN))
    {
      CStdString field = GetDatabaseField(FIELD_PLAYCOUNT, strType);
      query = field + " is NULL or " + field + parameter;
    }
  }
  if (query.IsEmpty() && m_field != FIELD_NONE)
    query = GetDatabaseField(m_field,strType) + negate + parameter;
  // if we fail to get a dbfield, we empty query so it doesn't fail
  if (query.Equals(negate + parameter))
    query = "";
  return query;
}

CStdString CSmartPlaylistRule::GetDatabaseField(DATABASE_FIELD field, const CStdString& type)
{
  if (type == "songs")
  {
    if (field == FIELD_TITLE) return "strTitle";
    else if (field == FIELD_GENRE) return "strGenre";
    else if (field == FIELD_ALBUM) return "strAlbum";
    else if (field == FIELD_YEAR) return "iYear";
    else if (field == FIELD_ARTIST || field == FIELD_ALBUMARTIST) return "strArtist";
    else if (field == FIELD_TIME) return "iDuration";
    else if (field == FIELD_PLAYCOUNT) return "iTimesPlayed";
    else if (field == FIELD_FILENAME) return "strFilename";
    else if (field == FIELD_PATH) return "strPath";
    else if (field == FIELD_TRACKNUMBER) return "iTrack";
    else if (field == FIELD_LASTPLAYED) return "lastPlayed";
    else if (field == FIELD_RATING) return "rating";
    else if (field == FIELD_COMMENT) return "comment";
    else if (field == FIELD_RANDOM) return "RANDOM()";      // only used for order clauses
    else if (field == FIELD_DATEADDED) return "idSong";     // only used for order clauses
  }
  else if (type == "albums")
  {
    if (field == FIELD_ALBUM) return "strAlbum";
    else if (field == FIELD_GENRE) return "strGenre"; // join required
    else if (field == FIELD_ARTIST) return "strArtist"; // join required
    else if (field == FIELD_ALBUMARTIST) return "strArtist"; // join required
    else if (field == FIELD_YEAR) return "iYear";
    else if (field == FIELD_REVIEW) return "strReview";
    else if (field == FIELD_THEMES) return "strThemes";
    else if (field == FIELD_MOODS) return "strMoods";
    else if (field == FIELD_STYLES) return "strStyles";
    else if (field == FIELD_ALBUMTYPE) return "strType";
    else if (field == FIELD_LABEL) return "strLabel";
    else if (field == FIELD_RATING) return "iRating";
    else if (field == FIELD_RANDOM) return "RANDOM()";      // only used for order clauses
    else if (field == FIELD_DATEADDED) return "idalbum";    // only used for order clauses
  }
  else if (type == "movies")
  {
    CStdString result;
    if (field == FIELD_TITLE) result.Format("c%02d", VIDEODB_ID_TITLE);
    else if (field == FIELD_PLOT) result.Format("c%02d", VIDEODB_ID_PLOT);
    else if (field == FIELD_PLOTOUTLINE) result.Format("c%02d", VIDEODB_ID_PLOTOUTLINE);
    else if (field == FIELD_TAGLINE) result.Format("c%02d", VIDEODB_ID_TAGLINE);
    else if (field == FIELD_VOTES) result.Format("c%02d", VIDEODB_ID_VOTES);
    else if (field == FIELD_RATING) result.Format("CAST(c%02d as DECIMAL(5,3))", VIDEODB_ID_RATING);
    else if (field == FIELD_TIME) result.Format("c%02d", VIDEODB_ID_RUNTIME);
    else if (field == FIELD_WRITER) result.Format("c%02d", VIDEODB_ID_CREDITS);   // join required
    else if (field == FIELD_PLAYCOUNT) result = "playCount";
    else if (field == FIELD_LASTPLAYED) result = "lastPlayed";
    else if (field == FIELD_INPROGRESS) result = "cant_order_by_inprogress";    // join required
    else if (field == FIELD_GENRE) result.Format("c%02d", VIDEODB_ID_GENRE);    // join required
    else if (field == FIELD_YEAR) result.Format("c%02d", VIDEODB_ID_YEAR);
    else if (field == FIELD_DIRECTOR) result.Format("c%02d", VIDEODB_ID_DIRECTOR); // join required
    else if (field == FIELD_ACTOR) result = "cant_order_by_actor";    // join required
    else if (field == FIELD_MPAA) result.Format("c%02d", VIDEODB_ID_MPAA);
    else if (field == FIELD_TOP250) result.Format("c%02d", VIDEODB_ID_TOP250);
    else if (field == FIELD_STUDIO) result.Format("c%02d", VIDEODB_ID_STUDIOS);   // join required
    else if (field == FIELD_COUNTRY) result.Format("c%02d", VIDEODB_ID_COUNTRY);    // join required
    else if (field == FIELD_HASTRAILER) result.Format("c%02d", VIDEODB_ID_TRAILER);
    else if (field == FIELD_FILENAME) result = "strFilename";
    else if (field == FIELD_PATH) result = "strPath";
    else if (field == FIELD_RANDOM) result = "RANDOM()";      // only used for order clauses
    else if (field == FIELD_DATEADDED) result = "idMovie";       // only used for order clauses
    else if (field == FIELD_SET) result = "cant_order_by_set";
    return result;
  }
  else if (type == "musicvideos")
  {
    CStdString result;
    if (field == FIELD_TITLE) result.Format("c%02d",VIDEODB_ID_MUSICVIDEO_TITLE);
    else if (field == FIELD_GENRE) result.Format("c%02d", VIDEODB_ID_MUSICVIDEO_GENRE);  // join required
    else if (field == FIELD_ALBUM) result.Format("c%02d",VIDEODB_ID_MUSICVIDEO_ALBUM);
    else if (field == FIELD_YEAR) result.Format("c%02d",VIDEODB_ID_MUSICVIDEO_YEAR);
    else if (field == FIELD_ARTIST) result.Format("c%02d", VIDEODB_ID_MUSICVIDEO_ARTIST);  // join required;
    else if (field == FIELD_FILENAME) result = "strFilename";
    else if (field == FIELD_PATH) result = "strPath";
    else if (field == FIELD_PLAYCOUNT) result = "playCount";
    else if (field == FIELD_LASTPLAYED) result = "lastPlayed";
    else if (field == FIELD_TIME) result.Format("c%02d", VIDEODB_ID_MUSICVIDEO_RUNTIME);
    else if (field == FIELD_DIRECTOR) result.Format("c%02d", VIDEODB_ID_MUSICVIDEO_DIRECTOR);   // join required
    else if (field == FIELD_STUDIO) result.Format("c%02d", VIDEODB_ID_MUSICVIDEO_STUDIOS);     // join required
    else if (field == FIELD_PLOT) result.Format("c%02d", VIDEODB_ID_MUSICVIDEO_PLOT);
    else if (field == FIELD_RANDOM) result = "RANDOM()";      // only used for order clauses
    else if (field == FIELD_DATEADDED) result = "idMVideo";        // only used for order clauses
    return result;
  }
  if (type == "tvshows")
  {
    CStdString result;
    if (field == FIELD_TVSHOWTITLE) result.Format("c%02d", VIDEODB_ID_TV_TITLE);
    else if (field == FIELD_PLOT) result.Format("c%02d", VIDEODB_ID_TV_PLOT);
    else if (field == FIELD_STATUS) result.Format("c%02d", VIDEODB_ID_TV_STATUS);
    else if (field == FIELD_VOTES) result.Format("c%02d", VIDEODB_ID_TV_VOTES);
    else if (field == FIELD_RATING) result.Format("c%02d", VIDEODB_ID_TV_RATING);
    else if (field == FIELD_YEAR) result.Format("c%02d", VIDEODB_ID_TV_PREMIERED);
    else if (field == FIELD_GENRE) result.Format("c%02d", VIDEODB_ID_TV_GENRE);
    else if (field == FIELD_MPAA) result.Format("c%02d", VIDEODB_ID_TV_MPAA);
    else if (field == FIELD_STUDIO) result.Format("c%02d", VIDEODB_ID_TV_STUDIOS);
    else if (field == FIELD_DIRECTOR) result = "cant_order_by_director"; // join required
    else if (field == FIELD_ACTOR) result = "cant_order_by_actor";    // join required
    else if (field == FIELD_NUMEPISODES) result = "totalcount";
    else if (field == FIELD_NUMWATCHED) result = "watchedcount";
    else if (field == FIELD_PLAYCOUNT) result = "watched";
    else if (field == FIELD_RANDOM) result = "RANDOM()";      // only used for order clauses
    else if (field == FIELD_DATEADDED) result = "idShow";       // only used for order clauses
    else if (field == FIELD_PATH) result = "strPath";
    return result;
  }
  if (type == "episodes")
  {
    CStdString result;
    if (field == FIELD_EPISODETITLE) result.Format("c%02d", VIDEODB_ID_EPISODE_TITLE);
    else if (field == FIELD_TVSHOWTITLE) result = "strTitle";
    else if (field == FIELD_PLOT) result.Format("c%02d", VIDEODB_ID_EPISODE_PLOT);
    else if (field == FIELD_VOTES) result.Format("c%02d", VIDEODB_ID_EPISODE_VOTES);
    else if (field == FIELD_RATING) result.Format("c%02d", VIDEODB_ID_EPISODE_RATING);
    else if (field == FIELD_TIME) result.Format("c%02d", VIDEODB_ID_EPISODE_RUNTIME);
    else if (field == FIELD_WRITER) result.Format("c%02d", VIDEODB_ID_EPISODE_CREDITS);   // join required
    else if (field == FIELD_AIRDATE) result.Format("c%02d", VIDEODB_ID_EPISODE_AIRED);
    else if (field == FIELD_PLAYCOUNT) result = "playCount";
    else if (field == FIELD_LASTPLAYED) result = "lastPlayed";
    else if (field == FIELD_INPROGRESS) result = "cant_order_by_inprogress";    // join required
    else if (field == FIELD_GENRE) result = "cant_order_by_genre";    // join required
    else if (field == FIELD_YEAR) result = "premiered";
    else if (field == FIELD_DIRECTOR) result.Format("c%02d", VIDEODB_ID_EPISODE_DIRECTOR); // join required
    else if (field == FIELD_ACTOR) result = "cant_order_by_actor";    // join required
    else if (field == FIELD_EPISODE) result.Format("c%02d", VIDEODB_ID_EPISODE_EPISODE);
    else if (field == FIELD_SEASON) result.Format("c%02d", VIDEODB_ID_EPISODE_SEASON);
    else if (field == FIELD_FILENAME) result = "strFilename";
    else if (field == FIELD_PATH) result = "strPath";
    else if (field == FIELD_RANDOM) result = "RANDOM()";      // only used for order clauses
    else if (field == FIELD_DATEADDED) result = "idEpisode";       // only used for order clauses
    return result;
  }

  return "";
}

CSmartPlaylist::CSmartPlaylist()
{
  m_matchAllRules = true;
  m_limit = 0;
  m_orderField = CSmartPlaylistRule::FIELD_NONE;
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
    m_orderField = CSmartPlaylistRule::TranslateField(order->FirstChild()->Value());
  }
  return true;
}

bool CSmartPlaylist::Save(const CStdString &path)
{
  TiXmlDocument doc;
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
  if (m_orderField != CSmartPlaylistRule::FIELD_NONE)
  {
    TiXmlText order(CSmartPlaylistRule::TranslateField(m_orderField).c_str());
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

CStdString CSmartPlaylist::GetWhereClause(CDatabase &db, bool needWhere /* = true */)
{
  CStdString rule, currentRule;
  for (vector<CSmartPlaylistRule>::iterator it = m_playlistRules.begin(); it != m_playlistRules.end(); ++it)
  {
    if (it != m_playlistRules.begin())
      rule += m_matchAllRules ? " AND " : " OR ";
    else if (needWhere)
      rule += "WHERE ";
    rule += "(";
    currentRule = (*it).GetWhereClause(db, GetType());
    // if we don't get a rule, we add '1' or '0' so the query is still valid and doesn't fail
    if (currentRule.IsEmpty())
      currentRule = m_matchAllRules ? "'1'" : "'0'";
    rule += currentRule;
    rule += ")";
  }
  return rule;
}

CStdString CSmartPlaylist::GetOrderClause(CDatabase &db)
{
  CStdString order;
  if (m_orderField != CSmartPlaylistRule::FIELD_NONE)
  {
    if (CSmartPlaylistRule::GetFieldType(m_orderField) == CSmartPlaylistRule::NUMERIC_FIELD)
      order.Format("ORDER BY 1*%s", CSmartPlaylistRule::GetDatabaseField(m_orderField,GetType()));
    else
      order = db.PrepareSQL("ORDER BY %s", CSmartPlaylistRule::GetDatabaseField(m_orderField,GetType()).c_str());
    if (!m_orderAscending)
      order += " DESC";
  }
  if (m_limit)
  {
    CStdString limit;
    limit.Format(" LIMIT %i", m_limit);
    order += limit;
  }
  return order;
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

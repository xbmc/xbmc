/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "SmartPlaylist.h"
#include "utils/log.h"
#include "StringUtils.h"
#include "FileSystem/SmartPlaylistDirectory.h"
#include "utils/CharsetConverter.h"
#include "XMLUtils.h"
#include "Database.h"
#include "VideoDatabase.h"
#include "LocalizeStrings.h"
#include "Util.h"

using namespace DIRECTORY;

typedef struct
{
  char string[13];
  CSmartPlaylistRule::DATABASE_FIELD field;
  int localizedString;
} translateField;

static const translateField fields[] = { "none", CSmartPlaylistRule::FIELD_NONE, 231,
                                         "genre", CSmartPlaylistRule::SONG_GENRE, 515,
                                         "album", CSmartPlaylistRule::SONG_ALBUM, 558,
                                         "albumartist", CSmartPlaylistRule::SONG_ALBUM_ARTIST, 566,
                                         "artist", CSmartPlaylistRule::SONG_ARTIST, 557,
                                         "title", CSmartPlaylistRule::SONG_TITLE, 556,
                                         "year", CSmartPlaylistRule::SONG_YEAR, 562,
                                         "time", CSmartPlaylistRule::SONG_TIME, 180,
                                         "tracknumber", CSmartPlaylistRule::SONG_TRACKNUMBER, 554,
                                         "filename", CSmartPlaylistRule::SONG_FILENAME, 561,
                                         "playcount", CSmartPlaylistRule::SONG_PLAYCOUNT, 567,
                                         "lastplayed", CSmartPlaylistRule::SONG_LASTPLAYED, 568,
                                         "rating", CSmartPlaylistRule::SONG_RATING, 563,
                                         "comment", CSmartPlaylistRule::SONG_COMMENT, 569,
                                         "dateadded", CSmartPlaylistRule::SONG_DATEADDED, 570,
                                         "random", CSmartPlaylistRule::FIELD_RANDOM, 590,
                                         "playlist", CSmartPlaylistRule::FIELD_PLAYLIST, 559,
                                       };

#define NUM_FIELDS sizeof(fields) / sizeof(translateField)

typedef struct
{
  char string[15];
  CSmartPlaylistRule::SEARCH_OPERATOR op;
  int localizedString;
} operatorField;

static const operatorField operators[] = { "contains", CSmartPlaylistRule::OPERATOR_CONTAINS, 21400,
                                           "doesnotcontain", CSmartPlaylistRule::OPERATOR_DOES_NOT_CONTAIN, 21401,
                                           "is", CSmartPlaylistRule::OPERATOR_EQUALS, 21402,
                                           "isnot", CSmartPlaylistRule::OPERATOR_DOES_NOT_EQUAL, 21403,
                                           "startswith", CSmartPlaylistRule::OPERATOR_STARTS_WITH, 21404,
                                           "endswith", CSmartPlaylistRule::OPERATOR_ENDS_WITH, 21405,
                                           "greaterthan", CSmartPlaylistRule::OPERATOR_GREATER_THAN, 21406,
                                           "lessthan", CSmartPlaylistRule::OPERATOR_LESS_THAN, 21407,
                                           "after", CSmartPlaylistRule::OPERATOR_AFTER, 21408,
                                           "before", CSmartPlaylistRule::OPERATOR_BEFORE, 21409,
                                           "inthelast", CSmartPlaylistRule::OPERATOR_IN_THE_LAST, 21410,
                                           "notinthelast", CSmartPlaylistRule::OPERATOR_NOT_IN_THE_LAST, 21411
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
  for (int i = 0; i < NUM_FIELDS; i++)
    if (strcmpi(field, fields[i].string) == 0) return fields[i].field;
  return FIELD_NONE;
}

CStdString CSmartPlaylistRule::TranslateField(DATABASE_FIELD field)
{
  for (int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return fields[i].string;
  return "none";
}

CSmartPlaylistRule::SEARCH_OPERATOR CSmartPlaylistRule::TranslateOperator(const char *oper)
{
  for (int i = 0; i < NUM_OPERATORS; i++)
    if (strcmpi(oper, operators[i].string) == 0) return operators[i].op;
  return OPERATOR_CONTAINS;
}

CStdString CSmartPlaylistRule::TranslateOperator(SEARCH_OPERATOR oper)
{
  for (int i = 0; i < NUM_OPERATORS; i++)
    if (oper == operators[i].op) return operators[i].string;
  return "contains";
}

CStdString CSmartPlaylistRule::GetLocalizedField(DATABASE_FIELD field)
{
  for (int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return g_localizeStrings.Get(fields[i].localizedString);
  return g_localizeStrings.Get(16018);
}

CStdString CSmartPlaylistRule::GetLocalizedOperator(SEARCH_OPERATOR oper)
{
  for (int i = 0; i < NUM_OPERATORS; i++)
    if (oper == operators[i].op) return g_localizeStrings.Get(operators[i].localizedString);
  return g_localizeStrings.Get(16018);
}

CStdString CSmartPlaylistRule::GetLocalizedRule()
{
  CStdString rule;
  rule.Format("%s %s %s", GetLocalizedField(m_field).c_str(), GetLocalizedOperator(m_operator).c_str(), m_parameter.c_str());
  return rule;
}

CStdString CSmartPlaylistRule::GetWhereClause(const CStdString& strType)
{
  // the comparison piece
  CStdString operatorString;
  switch (m_operator)
  {
  case OPERATOR_CONTAINS:
    operatorString = " LIKE '%%%s%%'"; break;
  case OPERATOR_DOES_NOT_CONTAIN:
    operatorString = " NOT LIKE '%%%s%%'"; break;
  case OPERATOR_EQUALS:
    operatorString = " LIKE '%s'"; break;
  case OPERATOR_DOES_NOT_EQUAL:
    operatorString = " NOT LIKE '%s'"; break;
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
  }
  CStdString parameter = CDatabase::FormatSQL(operatorString.c_str(), m_parameter.c_str());
  if (m_field == SONG_LASTPLAYED)
  {
    if (m_operator == OPERATOR_IN_THE_LAST || m_operator == OPERATOR_NOT_IN_THE_LAST)
    { // translate time period
      CDateTime date=CDateTime::GetCurrentDateTime();
      CDateTimeSpan span;
      span.SetFromPeriod(m_parameter);
      date-=span;
      parameter = CDatabase::FormatSQL(operatorString.c_str(), date.GetAsDBDate().c_str());
    }
  }
  else if (m_field == SONG_TIME)
  { // translate time to seconds
    CStdString seconds; seconds.Format("%i", StringUtils::TimeStringToSeconds(m_parameter));
    parameter = CDatabase::FormatSQL(operatorString.c_str(), seconds.c_str());
  }

  // now the query parameter
  CStdString query;
  if (strType == "music")
  {
    if (m_field == SONG_GENRE)
      query = "(strGenre" + parameter + ") or (idsong IN (select idsong from genre,exgenresong where exgenresong.idgenre = genre.idgenre and genre.strGenre" + parameter + "))";
    else if (m_field == SONG_ARTIST)
      query = "(strArtist" + parameter + ") or (idsong IN (select idsong from artist,exartistsong where exartistsong.idartist = artist.idartist and artist.strArtist" + parameter + "))";
    else if (m_field == SONG_ALBUM_ARTIST)
      query = "idalbum in (select idalbum from artist,album where album.idartist=artist.idartist and artist.strArtist" + parameter + ") or idalbum in (select idalbum from artist,exartistalbum where exartistalbum.idartist = artist.idartist and artist.strArtist" + parameter + ")";
    else if (m_field == SONG_LASTPLAYED && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = "lastPlayed is NULL or lastPlayed" + parameter;
  }
  if (strType == "video")
  {
    if (m_field == SONG_GENRE)
      query = "(strGenre" + parameter + ") or (idmvideo IN (select genrelinkmusicvideo.idmvideo from genre,genrelinkmusicvideo where genrelinkmusicvideo.idgenre = genre.idgenre and genre.strGenre" + parameter + "))";
    else if (m_field == SONG_ARTIST)
      query = "(strArtist" + parameter + ") or (idmvideo IN (select genrelinkmusicvideo.idmvideo from actors,artistlinkmusicvideo where artistlinkmusicvideo.idartist = actors.idActor and actors.strActor" + parameter + "))";

  }
  if (m_field == FIELD_PLAYLIST)
  { // playlist field - grab our playlist and add to our where clause
    CStdString playlistFile = CSmartPlaylistDirectory::GetPlaylistByName(m_parameter);
    if (!playlistFile.IsEmpty())
    {
      CSmartPlaylist playlist;
      playlist.Load(playlistFile);
      CStdString playlistQuery = playlist.GetWhereClause(false);
      if (m_operator == OPERATOR_DOES_NOT_EQUAL)
        query.Format("NOT (%s)", playlistQuery.c_str());
      else if (m_operator == OPERATOR_EQUALS)
        query = playlistQuery;
    }
  }
  else if (m_field != FIELD_NONE)
    query = GetDatabaseField(m_field,strType) + parameter;
  return query;
}

CStdString CSmartPlaylistRule::GetDatabaseField(DATABASE_FIELD field, const CStdString& type)
{
  if (type == "music")
  {
    if (field == SONG_TITLE) return "strTitle";
    else if (field == SONG_GENRE) return "strGenre";
    else if (field == SONG_ALBUM) return "strAlbum";
    else if (field == SONG_YEAR) return "iYear";
    else if (field == SONG_ARTIST || field == SONG_ALBUM_ARTIST) return "strArtist";
    else if (field == SONG_TIME) return "iDuration";
    else if (field == SONG_PLAYCOUNT) return "iTimesPlayed";
    else if (field == SONG_FILENAME) return "strFilename";
    else if (field == SONG_TRACKNUMBER) return "iTrack";
    else if (field == SONG_LASTPLAYED) return "lastplayed";
    else if (field == SONG_RATING) return "rating";
    else if (field == SONG_COMMENT) return "comment";
    else if (field == FIELD_RANDOM) return "random()";      // only used for order clauses
    else if (field == SONG_DATEADDED) return "idsong";         // only used for order clauses
  }
  if (type == "video")
  {
    CStdString result;
    if (field == SONG_TITLE) result.Format("musicvideo.c%02d",VIDEODB_ID_MUSICVIDEO_TITLE);
    else if (field == SONG_GENRE) result = "genre.strgenre";
    else if (field == SONG_ALBUM) result.Format("musicvideo.c%02d",VIDEODB_ID_MUSICVIDEO_ALBUM);
    else if (field == SONG_YEAR) result.Format("musicvideo.c%02d",VIDEODB_ID_MUSICVIDEO_YEAR);
    else if (field == SONG_ARTIST) result.Format("actors.strActor");
//    else if (field == SONG_TIME) return "iDuration";
//    else if (field == SONG_PLAYCOUNT) return "iTimesPlayed";
    else if (field == SONG_FILENAME) result = "strFilename";
//    else if (field == SONG_TRACKNUMBER) return "iTrack";
//    else if (field == SONG_LASTPLAYED) return "lastplayed";
//    else if (field == SONG_RATING) return "rating";
//    else if (field == SONG_COMMENT) return "comment";
    else if (field == FIELD_RANDOM) result = "random()";      // only used for order clauses
//    else if (field == SONG_DATEADDED) return "idsong";         // only used for order clauses
  
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
  m_playlistType = "music"; // sane default
}

TiXmlElement *CSmartPlaylist::OpenAndReadName(const CStdString &path)
{
  if (!m_xmlDoc.LoadFile(path))
  {
    CLog::Log(LOGERROR, "Error loading Smart playlist %s", path.c_str());
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
  // load the playlist name
  TiXmlHandle name = ((TiXmlHandle)root->FirstChild("name")).FirstChild();
  if (name.Node())
    m_playlistName = name.Node()->Value();
  else
  {
    m_playlistName = CUtil::GetTitleFromPath(path);
    if (CUtil::GetExtension(m_playlistName) == ".xsp")
      CUtil::RemoveExtension(m_playlistName);
  }
  return root;
}

bool CSmartPlaylist::Load(const CStdString &path)
{
  TiXmlElement *root = OpenAndReadName(path);
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
    if (field && oper && parameter)
    { // valid rule
      // TODO UTF8: We assume these are from string charset so far.
      // Once they're constructible from the GUI, we'll check for encoding.
      CStdString utf8Parameter;
      g_charsetConverter.stringCharsetToUtf8(parameter->Value(), utf8Parameter);
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
  // TODO: Format strings in UTF8?
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

CStdString CSmartPlaylist::GetWhereClause(bool needWhere /* = true */)
{
  CStdString rule;
  for (vector<CSmartPlaylistRule>::iterator it = m_playlistRules.begin(); it != m_playlistRules.end(); ++it)
  {
    if (it != m_playlistRules.begin())
      rule += m_matchAllRules ? " AND " : " OR ";
    else if (needWhere)
      rule += "WHERE ";
    rule += "(";
    rule += (*it).GetWhereClause(GetType());
    rule += ")";
  }
  return rule;
}

CStdString CSmartPlaylist::GetOrderClause()
{
  CStdString order;
  if (m_orderField != CSmartPlaylistRule::FIELD_NONE)
    order.Format("ORDER BY %s%s", CSmartPlaylistRule::GetDatabaseField(m_orderField,GetType()), m_orderAscending ? "" : " DESC");
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
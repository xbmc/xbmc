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

#include "SmartPlaylist.h"
#include "utils/log.h"
#include "StringUtils.h"
#include "FileSystem/SmartPlaylistDirectory.h"
#include "utils/CharsetConverter.h"
#include "XMLUtils.h"
#include "Database.h"

using namespace DIRECTORY;

namespace PLAYLIST
{

typedef struct
{
  char string[13];
  CSmartPlaylistRule::DATABASE_FIELD field;
} translateField;

static const translateField fields[] = { "none", CSmartPlaylistRule::FIELD_NONE,
                                         "genre", CSmartPlaylistRule::SONG_GENRE,
                                         "album", CSmartPlaylistRule::SONG_ALBUM,
                                         "albumartist", CSmartPlaylistRule::SONG_ALBUM_ARTIST,
                                         "artist", CSmartPlaylistRule::SONG_ARTIST,
                                         "title", CSmartPlaylistRule::SONG_TITLE,
                                         "year", CSmartPlaylistRule::SONG_YEAR,
                                         "time", CSmartPlaylistRule::SONG_TIME,
                                         "tracknumber", CSmartPlaylistRule::SONG_TRACKNUMBER,
                                         "filename", CSmartPlaylistRule::SONG_FILENAME,
                                         "playcount", CSmartPlaylistRule::SONG_PLAYCOUNT,
                                         "lastplayed", CSmartPlaylistRule::SONG_LASTPLAYED,
                                         "rating", CSmartPlaylistRule::SONG_RATING,
                                         "comment", CSmartPlaylistRule::SONG_COMMENT,
                                         "random", CSmartPlaylistRule::FIELD_RANDOM,
                                         "playlist", CSmartPlaylistRule::FIELD_PLAYLIST
                                       };

#define NUM_FIELDS sizeof(fields) / sizeof(translateField)

CSmartPlaylistRule::CSmartPlaylistRule()
{
  m_field = SONG_ARTIST;
  m_operator = OPERATOR_CONTAINS;
  m_parameter = "";
}

void CSmartPlaylistRule::TranslateStrings(const char *field, const char *oper, const char *parameter)
{
  m_field = TranslateField(field);
  m_operator = TranslateOperator(oper);
  m_parameter = parameter;
  if (m_field == SONG_TIME)
  { // translate time to seconds
    m_parameter.Format("%i", StringUtils::TimeStringToSeconds(m_parameter));
  }
  if (m_field == SONG_LASTPLAYED)
  {
    if (m_operator == OPERATOR_IN_THE_LAST || m_operator == OPERATOR_NOT_IN_THE_LAST)
    { // translate time period
      CDateTime date=CDateTime::GetCurrentDateTime();
      CDateTimeSpan span;
      span.SetFromPeriod(m_parameter);
      date-=span;
      m_parameter = date.GetAsDBDate();
      m_operator = (m_operator == OPERATOR_IN_THE_LAST) ? OPERATOR_GREATER_THAN : OPERATOR_LESS_THAN;
    }
  }
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
  if (strcmpi(oper, "contains") == 0) return OPERATOR_CONTAINS;
  else if (strcmpi(oper, "doesnotcontain") == 0) return OPERATOR_DOES_NOT_CONTAIN;
  else if (strcmpi(oper, "is") == 0) return OPERATOR_EQUALS;
  else if (strcmpi(oper, "isnot") == 0) return OPERATOR_DOES_NOT_EQUAL;
  else if (strcmpi(oper, "startswith") == 0) return OPERATOR_STARTS_WITH;
  else if (strcmpi(oper, "endswith") == 0) return OPERATOR_ENDS_WITH;
  else if (strcmpi(oper, "greaterthan") == 0) return OPERATOR_GREATER_THAN;
  else if (strcmpi(oper, "lessthan") == 0) return OPERATOR_LESS_THAN;
  else if (strcmpi(oper, "inthelast") == 0) return OPERATOR_IN_THE_LAST;
  else if (strcmpi(oper, "notinthelast") == 0) return OPERATOR_NOT_IN_THE_LAST;
  return OPERATOR_CONTAINS;
}

CStdString CSmartPlaylistRule::TranslateOperator(SEARCH_OPERATOR oper)
{
  if (oper == OPERATOR_CONTAINS) return "contains";
  else if (oper == OPERATOR_DOES_NOT_CONTAIN) return "contains";
  else if (oper == OPERATOR_EQUALS) return "is";
  else if (oper == OPERATOR_DOES_NOT_EQUAL) return "isnot";
  else if (oper == OPERATOR_STARTS_WITH) return "startswith";
  else if (oper == OPERATOR_ENDS_WITH) return "endswith";
  else if (oper == OPERATOR_GREATER_THAN) return "greaterthan";
  else if (oper == OPERATOR_LESS_THAN) return "lessthan";
  else if (oper == OPERATOR_IN_THE_LAST) return "inthelast";
  else if (oper == OPERATOR_NOT_IN_THE_LAST) return "notinthelast";
  return "contains";
}

CStdString CSmartPlaylistRule::GetWhereClause()
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
  case OPERATOR_GREATER_THAN:
    operatorString = " > '%s'"; break;
  case OPERATOR_LESS_THAN:
    operatorString = " < '%s'"; break;
  }
  CStdString parameter = CDatabase::FormatSQL(operatorString.c_str(), m_parameter.c_str());
  // now the query parameter
  CStdString query;
  if (m_field == SONG_GENRE)
    query = "(strGenre" + parameter + ") or (idsong IN (select idsong from genre,exgenresong where exgenresong.idgenre = genre.idgenre and genre.strGenre" + parameter + "))";
  else if (m_field == SONG_ARTIST)
    query = "(strArtist" + parameter + ") or (idsong IN (select idsong from artist,exartistsong where exartistsong.idartist = artist.idartist and artist.strArtist" + parameter + "))";
  else if (m_field == SONG_ALBUM_ARTIST)
    query = "idalbum in (select idalbum from artist,album where album.idartist=artist.idartist and artist.strArtist" + parameter + ") or idalbum in (select idalbum from artist,exartistalbum where exartistalbum.idartist = artist.idartist and artist.strArtist" + parameter + ")";
  else if (m_field == SONG_LASTPLAYED && m_operator == OPERATOR_LESS_THAN)
    query = "lastPlayed is NULL or lastPlayed" + parameter;
  else if (m_field == FIELD_PLAYLIST)
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
    query = GetDatabaseField(m_field) + parameter;
  return query;
}

CStdString CSmartPlaylistRule::GetDatabaseField(DATABASE_FIELD field)
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
  return "";
}

CSmartPlaylist::CSmartPlaylist()
{
  m_matchAllRules = true;
  m_limit = 0;
  m_orderField = CSmartPlaylistRule::FIELD_NONE;
  m_orderAscending = true;
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
  // load the playlist name
  TiXmlHandle name = ((TiXmlHandle)root->FirstChild("name")).FirstChild();
  if (name.Node())
    m_playlistName = name.Node()->Value();
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
    rule += (*it).GetWhereClause();
    rule += ")";
  }
  return rule;
}

CStdString CSmartPlaylist::GetOrderClause()
{
  CStdString order;
  if (m_orderField != CSmartPlaylistRule::FIELD_NONE)
    order.Format("ORDER BY %s%s", CSmartPlaylistRule::GetDatabaseField(m_orderField), m_orderAscending ? "" : " DESC");
  if (m_limit)
  {
    CStdString limit;
    limit.Format(" LIMIT %i", m_limit);
    order += limit;
  }
  return order;
}

}
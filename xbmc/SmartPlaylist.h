#pragma once
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

#include "StdString.h"
#include "tinyXML/tinyxml.h"
#include <vector>

class CDatabase;

class CSmartPlaylistRule
{
public:
  CSmartPlaylistRule();

  enum DATABASE_FIELD { FIELD_NONE = 0,
                        FIELD_GENRE = 1,
                        FIELD_ALBUM,
                        FIELD_ARTIST,
                        FIELD_ALBUMARTIST,
                        FIELD_TITLE,
                        FIELD_YEAR,
                        FIELD_TIME,
                        FIELD_TRACKNUMBER,
                        FIELD_FILENAME,
                        FIELD_PATH,
                        FIELD_PLAYCOUNT,
                        FIELD_LASTPLAYED,
                        FIELD_INPROGRESS,
                        FIELD_RATING,
                        FIELD_COMMENT,
                        FIELD_DATEADDED,
                        FIELD_TVSHOWTITLE,
                        FIELD_EPISODETITLE,
                        FIELD_PLOT,
                        FIELD_PLOTOUTLINE,
                        FIELD_TAGLINE,
                        FIELD_STATUS,
                        FIELD_VOTES,
                        FIELD_DIRECTOR,
                        FIELD_ACTOR,
                        FIELD_STUDIO,
                        FIELD_COUNTRY,
                        FIELD_MPAA,
                        FIELD_TOP250,
                        FIELD_NUMEPISODES,
                        FIELD_NUMWATCHED,
                        FIELD_WRITER,
                        FIELD_AIRDATE,
                        FIELD_EPISODE,
                        FIELD_SEASON,
                        FIELD_REVIEW,
                        FIELD_THEMES,
                        FIELD_MOODS,
                        FIELD_STYLES,
                        FIELD_ALBUMTYPE,
                        FIELD_LABEL,
                        FIELD_HASTRAILER,
                        FIELD_VIDEORESOLUTION,
                        FIELD_AUDIOCHANNELS,
                        FIELD_VIDEOCODEC,
                        FIELD_AUDIOCODEC,
                        FIELD_AUDIOLANGUAGE,
                        FIELD_SUBTITLELANGUAGE,
                        FIELD_VIDEOASPECT,
                        FIELD_PLAYLIST,
                        FIELD_RANDOM
                      };

  enum SEARCH_OPERATOR { OPERATOR_START = 0,
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
                         OPERATOR_END
                       };

  enum FIELD_TYPE { TEXT_FIELD = 0,
                    BROWSEABLE_FIELD,
                    NUMERIC_FIELD,
                    DATE_FIELD,
                    PLAYLIST_FIELD,
                    SECONDS_FIELD,
                    BOOLEAN_FIELD,
                    TEXTIN_FIELD
                  };

  CStdString GetWhereClause(CDatabase &db, const CStdString& strType);
  void TranslateStrings(const char *field, const char *oper, const char *parameter);
  static DATABASE_FIELD TranslateField(const char *field);
  static CStdString     TranslateField(DATABASE_FIELD field);
  static CStdString     GetDatabaseField(DATABASE_FIELD field, const CStdString& strType);
  static CStdString     TranslateOperator(SEARCH_OPERATOR oper);

  static CStdString     GetLocalizedField(DATABASE_FIELD field);
  static CStdString     GetLocalizedOperator(SEARCH_OPERATOR oper);
  static std::vector<DATABASE_FIELD> GetFields(const CStdString &type, bool sortOrders = false);
  static FIELD_TYPE     GetFieldType(DATABASE_FIELD field);

  CStdString            GetLocalizedRule();

  TiXmlElement GetAsElement();

  DATABASE_FIELD     m_field;
  SEARCH_OPERATOR    m_operator;
  CStdString         m_parameter;
private:
  SEARCH_OPERATOR    TranslateOperator(const char *oper);

  CStdString GetVideoResolutionQuery(void);
};

class CSmartPlaylist
{
public:
  CSmartPlaylist();

  bool Load(const CStdString &path);
  bool Save(const CStdString &path);

  TiXmlElement *OpenAndReadName(const CStdString &path);

  void SetName(const CStdString &name);
  void SetType(const CStdString &type); // music, video, mixed
  const CStdString& GetName() const { return m_playlistName; };
  const CStdString& GetType() const { return m_playlistType; };

  void SetMatchAllRules(bool matchAll) { m_matchAllRules = matchAll; };
  bool GetMatchAllRules() const { return m_matchAllRules; };

  void SetLimit(unsigned int limit) { m_limit = limit; };
  unsigned int GetLimit() const { return m_limit; };

  void SetOrder(CSmartPlaylistRule::DATABASE_FIELD order) { m_orderField = order; };
  CSmartPlaylistRule::DATABASE_FIELD GetOrder() const { return m_orderField; };

  void SetOrderAscending(bool orderAscending) { m_orderAscending = orderAscending; };
  bool GetOrderAscending() const { return m_orderAscending; };

  void AddRule(const CSmartPlaylistRule &rule);
  CStdString GetWhereClause(CDatabase &db, bool needWhere = true);
  CStdString GetOrderClause(CDatabase &db);

  const std::vector<CSmartPlaylistRule> &GetRules() const;

  CStdString GetSaveLocation() const;
private:
  friend class CGUIDialogSmartPlaylistEditor;

  std::vector<CSmartPlaylistRule> m_playlistRules;
  CStdString m_playlistName;
  CStdString m_playlistType;
  bool m_matchAllRules;
  // order information
  unsigned int m_limit;
  CSmartPlaylistRule::DATABASE_FIELD m_orderField;
  bool m_orderAscending;

  TiXmlDocument m_xmlDoc;
};


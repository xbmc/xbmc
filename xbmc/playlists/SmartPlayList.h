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

#include <set>
#include <vector>

#include "utils/SortUtils.h"
#include "utils/StdString.h"
#include "utils/XBMCTinyXML.h"

class CDatabase;

class CSmartPlaylistRule
{
public:
  CSmartPlaylistRule();

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

  CStdString GetWhereClause(CDatabase &db, const CStdString& strType) const;
  void TranslateStrings(const char *field, const char *oper, const char *parameter);

  static Field                TranslateField(const char *field);
  static CStdString           TranslateField(Field field);
  static SortBy               TranslateOrder(const char *order);
  static CStdString           TranslateOrder(SortBy order);
  static CStdString           GetField(Field field, const CStdString& strType);
  static CStdString           TranslateOperator(SEARCH_OPERATOR oper);

  static CStdString           GetLocalizedField(Field field);
  static CStdString           GetLocalizedOrder(SortBy order);
  static CStdString           GetLocalizedOperator(SEARCH_OPERATOR oper);
  static std::vector<Field>   GetFields(const CStdString &type);
  static std::vector<SortBy>  GetOrders(const CStdString &type);
  static FIELD_TYPE           GetFieldType(Field field);

  CStdString                  GetLocalizedRule() const;

  TiXmlElement                GetAsElement() const;

  Field                       m_field;
  SEARCH_OPERATOR             m_operator;
  CStdString                  m_parameter;
private:
  static SEARCH_OPERATOR TranslateOperator(const char *oper);

  CStdString GetVideoResolutionQuery(void) const;
};

class CSmartPlaylist
{
public:
  CSmartPlaylist();

  bool Load(const CStdString &path);
  bool Save(const CStdString &path);

  TiXmlElement *OpenAndReadName(const CStdString &path);
  bool LoadFromXML(TiXmlElement *root, const CStdString &encoding = "UTF-8");

  void SetName(const CStdString &name);
  void SetType(const CStdString &type); // music, video, mixed
  const CStdString& GetName() const { return m_playlistName; };
  const CStdString& GetType() const { return m_playlistType; };

  void SetMatchAllRules(bool matchAll) { m_matchAllRules = matchAll; };
  bool GetMatchAllRules() const { return m_matchAllRules; };

  void SetLimit(unsigned int limit) { m_limit = limit; };
  unsigned int GetLimit() const { return m_limit; };

  void SetOrder(SortBy order) { m_orderField = order; };
  SortBy GetOrder() const { return m_orderField; };

  void SetOrderAscending(bool orderAscending) { m_orderAscending = orderAscending; };
  bool GetOrderAscending() const { return m_orderAscending; };

  void AddRule(const CSmartPlaylistRule &rule);

  /*! \brief get the where clause for a playlist
   We handle playlists inside playlists separately in order to ensure we don't introduce infinite loops
   by playlist A including playlist B which also (perhaps via other playlists) then includes playlistA.
   
   \param db the database to use to format up results
   \param referencedPlaylists a set of playlists to know when we reach a cycle
   \param needWhere whether we need to prepend the where clause with "WHERE "
   */
  CStdString GetWhereClause(CDatabase &db, std::set<CStdString> &referencedPlaylists) const;

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
  SortBy m_orderField;
  bool m_orderAscending;

  CXBMCTinyXML m_xmlDoc;
};


#pragma once

#include "StdString.h"
#include "tinyxml/tinyxml.h"
#include <vector>

class CSmartPlaylistRule
{
public:
  CSmartPlaylistRule();

  enum DATABASE_FIELD { FIELD_NONE = 0,
                        SONG_GENRE = 1,
                        SONG_ALBUM,
                        SONG_ARTIST,
                        SONG_ALBUM_ARTIST,
                        SONG_TITLE,
                        SONG_YEAR,
                        SONG_TIME,
                        SONG_TRACKNUMBER,
                        SONG_FILENAME,
                        SONG_PLAYCOUNT,
                        SONG_LASTPLAYED,
                        SONG_RATING,
                        SONG_COMMENT,
                        SONG_DATEADDED,
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
                         OPERATOR_END
                       };

  CStdString GetWhereClause(const CStdString& strType);
  void TranslateStrings(const char *field, const char *oper, const char *parameter);
  static DATABASE_FIELD TranslateField(const char *field);
  static CStdString     TranslateField(DATABASE_FIELD field);
  static CStdString     GetDatabaseField(DATABASE_FIELD field, const CStdString& strType);
  static CStdString     TranslateOperator(SEARCH_OPERATOR oper);

  static CStdString     GetLocalizedField(DATABASE_FIELD field);
  static CStdString     GetLocalizedOperator(SEARCH_OPERATOR oper);
  CStdString            GetLocalizedRule();

  TiXmlElement GetAsElement();

  DATABASE_FIELD     m_field;
  SEARCH_OPERATOR    m_operator;
  CStdString         m_parameter;
private:
  SEARCH_OPERATOR    TranslateOperator(const char *oper);
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
  CStdString GetWhereClause(bool needWhere = true);
  CStdString GetOrderClause();

  const vector<CSmartPlaylistRule> &GetRules() const;

private:
  friend class CGUIDialogSmartPlaylistEditor;

  vector<CSmartPlaylistRule> m_playlistRules;
  CStdString m_playlistName;
  CStdString m_playlistType;
  bool m_matchAllRules;
  // order information
  unsigned int m_limit;
  CSmartPlaylistRule::DATABASE_FIELD m_orderField;
  bool m_orderAscending;

  TiXmlDocument m_xmlDoc;
};

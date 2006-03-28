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
                        SONG_TITLE,
                        SONG_YEAR,
                        SONG_TIME,
                        SONG_TRACKNUMBER,
                        SONG_FILENAME,
                        SONG_PLAYCOUNT,
                        SONG_LASTPLAYED,
                        FIELD_RANDOM };

  enum SEARCH_OPERATOR { OPERATOR_CONTAINS = 1,
                         OPERATOR_DOES_NOT_CONTAIN,
                         OPERATOR_EQUALS,
                         OPERATOR_DOES_NOT_EQUAL,
                         OPERATOR_STARTS_WITH,
                         OPERATOR_ENDS_WITH,
                         OPERATOR_GREATER_THAN,
                         OPERATOR_LESS_THAN,
                         OPERATOR_IN_THE_LAST,
                         OPERATOR_NOT_IN_THE_LAST };

  CStdString GetWhereClause();
  void TranslateStrings(const char *field, const char *oper, const char *parameter);
  static DATABASE_FIELD TranslateField(const char *field);
  static CStdString     TranslateField(DATABASE_FIELD field);
  static CStdString     GetDatabaseField(DATABASE_FIELD field);

  TiXmlElement GetAsElement();

  DATABASE_FIELD     m_field;
  SEARCH_OPERATOR    m_operator;
  CStdString         m_parameter;
private:
  SEARCH_OPERATOR    TranslateOperator(const char *oper);
  CStdString         TranslateOperator(SEARCH_OPERATOR oper);
};

class CSmartPlaylist
{
public:
  CSmartPlaylist();

  bool Load(const CStdString &path);
  bool Save(const CStdString &path);

  TiXmlElement *OpenAndReadName(const CStdString &path);

  void SetName(const CStdString &name);
  const CStdString& GetName() const { return m_playlistName; };

  void AddRule(const CSmartPlaylistRule &rule);
  CStdString GetWhereClause();
  CStdString GetOrderClause();

private:
  vector<CSmartPlaylistRule> m_playlistRules;
  CStdString m_playlistName;
  bool m_matchAllRules;
  // order information
  unsigned int m_limit;
  CSmartPlaylistRule::DATABASE_FIELD m_orderField;
  bool m_orderAscending;
};
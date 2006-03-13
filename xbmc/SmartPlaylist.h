#pragma once

#include "StdString.h"
#include "tinyxml/tinyxml.h"
#include <vector>

class CSmartPlaylistRule
{
public:
  CSmartPlaylistRule();

  enum SEARCH_FIELD { SONG_GENRE = 1,
                      SONG_ALBUM,
                      SONG_ARTIST,
                      SONG_TITLE,
                      SONG_YEAR,
                      SONG_TIME,
                      SONG_TRACKNUMBER,
                      SONG_FILENAME,
                      SONG_PLAYCOUNT };

  enum SEARCH_OPERATOR { OPERATOR_CONTAINS = 1,
                         OPERATOR_DOES_NOT_CONTAIN,
                         OPERATOR_EQUALS,
                         OPERATOR_DOES_NOT_EQUAL,
                         OPERATOR_STARTS_WITH,
                         OPERATOR_ENDS_WITH,
                         OPERATOR_GREATER_THAN,
                         OPERATOR_LESS_THAN };

  CStdString GetWhereClause();
  void TranslateStrings(const char *field, const char *oper, const char *parameter);

  TiXmlElement GetAsElement();

  SEARCH_FIELD       m_field;
  SEARCH_OPERATOR    m_operator;
  CStdString         m_parameter;
private:
  SEARCH_FIELD       TranslateField(const char *field);
  SEARCH_OPERATOR    TranslateOperator(const char *oper);
  CStdString         TranslateField(SEARCH_FIELD field);
  CStdString         TranslateOperator(SEARCH_OPERATOR oper);
};

class CSmartPlaylist
{
public:
  CSmartPlaylist();

  bool Load(const CStdString &path);
  bool Save(const CStdString &path);

  void SetName(const CStdString &name);
  void AddRule(const CSmartPlaylistRule &rule);
  CStdString GetWhereClause();

private:
  vector<CSmartPlaylistRule> m_playlistRules;
  CStdString m_playlistName;
  bool m_matchAllRules;
};
#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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

#include <set>
#include <string>
#include <vector>
#include <memory>

#include "../dbwrappers/CommonDatabase.h"
#include <odb/odb_gen/ODBMovie.h>
#include <odb/odb_gen/ODBMovie_odb.h>
#include <odb/odb_gen/ODBSong.h>
#include <odb/odb_gen/ODBSong_odb.h>

#define DATABASEQUERY_RULE_VALUE_SEPARATOR  " / "

class CDatabase;
class CVariant;
class TiXmlNode;

class CDatabaseQueryRule
{
public:
  CDatabaseQueryRule();
  virtual ~CDatabaseQueryRule() = default;

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
                         OPERATOR_BETWEEN,
                         OPERATOR_END
                       };

  enum FIELD_TYPE { TEXT_FIELD = 0,
                    REAL_FIELD,
                    NUMERIC_FIELD,
                    DATE_FIELD,
                    PLAYLIST_FIELD,
                    SECONDS_FIELD,
                    BOOLEAN_FIELD,
                    TEXTIN_FIELD
                  };

  virtual bool Load(const TiXmlNode *node, const std::string &encoding = "UTF-8");
  virtual bool Load(const CVariant &obj);
  virtual bool Save(TiXmlNode *parent) const;
  virtual bool Save(CVariant &obj) const;

  static std::string          GetLocalizedOperator(SEARCH_OPERATOR oper);
  static void                 GetAvailableOperators(std::vector<std::string> &operatorList);

  std::string                 GetParameter() const;
  void                        SetParameter(const std::string &value);
  void                        SetParameter(const std::vector<std::string> &values);

  virtual std::string         GetWhereClause(const CDatabase &db, const std::string& strType) const;
  virtual odb::query<ODBView_Movie> GetMovieWhereClause(const std::string& strType);
  virtual odb::query<ODBView_TVShow> GetTVShowWhereClause(const std::string& strType);
  virtual odb::query<ODBView_Episode> GetEpisodeWhereClause(const std::string& strType);
  virtual odb::query<ODBView_Song_Artists> GetArtistWhereClause(const std::string& strType);
  virtual odb::query<ODBView_Album> GetAlbumWhereClause(const std::string& strType);
  virtual odb::query<ODBView_Song> GetSongWhereClause(const std::string& strType);

  int                         m_field;
  SEARCH_OPERATOR             m_operator;
  std::vector<std::string>    m_parameter;
  
  mutable bool m_hasRoleRule;
  void SetHasRoleRule(bool val) const { m_hasRoleRule = val; };
  bool GetHasRoleRule() const { return m_hasRoleRule; };
  
protected:
  virtual std::string         GetField(int field, const std::string& type) const=0;
  virtual FIELD_TYPE          GetFieldType(int field) const=0;
  virtual int                 TranslateField(const char *field) const=0;
  virtual std::string         TranslateField(int field) const=0;
  std::string                 ValidateParameter(const std::string &parameter) const;
  virtual std::string         FormatParameter(const std::string &negate, const std::string &oper, const CDatabase &db, const std::string &type) const;
  virtual std::string         FormatWhereClause(const std::string &negate, const std::string &oper, const std::string &param,
                                                const CDatabase &db, const std::string &type) const;
  virtual odb::query<ODBView_Movie> FormatMovieWhereClause(const bool &negate,
                                                           const SEARCH_OPERATOR &oper,
                                                           const std::string &param,
                                                           const std::string &strType) const;
  virtual odb::query<ODBView_Movie> FormatMovieWhereBetweenClause(const bool &negate,
                                                           const SEARCH_OPERATOR &oper,
                                                           const std::string &param1,
                                                           const std::string &param2,
                                                           const std::string &strType) const;
  virtual odb::query<ODBView_TVShow> FormatTVShowWhereClause(const bool &negate,
                                                           const SEARCH_OPERATOR &oper,
                                                           const std::string &param,
                                                           const std::string &strType) const;
  virtual odb::query<ODBView_TVShow> FormatTVShowWhereBetweenClause(const bool &negate,
                                                                  const SEARCH_OPERATOR &oper,
                                                                  const std::string &param1,
                                                                  const std::string &param2,
                                                                  const std::string &strType) const;
  virtual odb::query<ODBView_Episode> FormatEpisodeWhereClause(const bool &negate,
                                                               const SEARCH_OPERATOR &oper,
                                                               const std::string &param,
                                                               const std::string &strType) const;
  virtual odb::query<ODBView_Episode> FormatEpisodeWhereBetweenClause(const bool &negate,
                                                                      const SEARCH_OPERATOR &oper,
                                                                      const std::string &param1,
                                                                      const std::string &param2,
                                                                      const std::string &strType) const;
  virtual odb::query<ODBView_Song_Artists> FormatArtistWhereClause(const bool &negate,
                                                                   const SEARCH_OPERATOR &oper,
                                                                   const std::string &param,
                                                                   const std::string &strType) const;
  virtual odb::query<ODBView_Song_Artists> FormatArtistWhereBetweenClause(const bool &negate,
                                                                      const SEARCH_OPERATOR &oper,
                                                                      const std::string &param1,
                                                                      const std::string &param2,
                                                                      const std::string &strType) const;
  virtual odb::query<ODBView_Album> FormatAlbumWhereClause(const bool &negate,
                                                           const SEARCH_OPERATOR &oper,
                                                           const std::string &param,
                                                           const std::string &strType) const;
  virtual odb::query<ODBView_Album> FormatAlbumWhereBetweenClause(const bool &negate,
                                                                  const SEARCH_OPERATOR &oper,
                                                                  const std::string &param1,
                                                                  const std::string &param2,
                                                                  const std::string &strType) const;
  virtual odb::query<ODBView_Song> FormatSongWhereClause(const bool &negate,
                                                         const SEARCH_OPERATOR &oper,
                                                         const std::string &param,
                                                         const std::string &strType) const;
  virtual odb::query<ODBView_Song> FormatSongWhereBetweenClause(const bool &negate,
                                                                const SEARCH_OPERATOR &oper,
                                                                const std::string &param1,
                                                                const std::string &param2,
                                                                const std::string &strType) const;
  
  virtual SEARCH_OPERATOR     GetOperator(const std::string &type) const { return m_operator; };
  virtual std::string         GetOperatorString(SEARCH_OPERATOR op) const;
  virtual std::string         GetBooleanQuery(const std::string &negate, const std::string &strType) const { return ""; }
  virtual odb::query<ODBView_Movie> GetMovieBooleanQuery(const bool &negate, const std::string &strType) { return odb::query<ODBView_Movie>(); }
  virtual odb::query<ODBView_TVShow> GetTVShowBooleanQuery(const bool &negate, const std::string &strType) { return odb::query<ODBView_TVShow>(); }
  virtual odb::query<ODBView_Episode> GetEpisodeBooleanQuery(const bool &negate, const std::string &strType) { return odb::query<ODBView_TVShow>(); }
  virtual odb::query<ODBView_Song_Artists> GetArtistBooleanQuery(const bool &negate, const std::string &strType) { return odb::query<ODBView_Song_Artists>(); }
  virtual odb::query<ODBView_Album> GetAlbumBooleanQuery(const bool &negate, const std::string &strType) { return odb::query<ODBView_Album>(); }
  virtual odb::query<ODBView_Song> GetSongBooleanQuery(const bool &negate, const std::string &strType) { return odb::query<ODBView_Song>(); }

  static SEARCH_OPERATOR      TranslateOperator(const char *oper);
  static std::string          TranslateOperator(SEARCH_OPERATOR oper);
};

class CDatabaseQueryRuleCombination;

typedef std::vector< std::shared_ptr<CDatabaseQueryRule> > CDatabaseQueryRules;
typedef std::vector< std::shared_ptr<CDatabaseQueryRuleCombination> > CDatabaseQueryRuleCombinations;

class IDatabaseQueryRuleFactory
{
public:
  virtual ~IDatabaseQueryRuleFactory() = default;
  virtual CDatabaseQueryRule *CreateRule() const=0;
  virtual CDatabaseQueryRuleCombination *CreateCombination() const=0;
};

class CDatabaseQueryRuleCombination
{
public:
  CDatabaseQueryRuleCombination();
  virtual ~CDatabaseQueryRuleCombination() = default;

  typedef enum {
    CombinationOr = 0,
    CombinationAnd
  } Combination;

  void clear();
  virtual bool Load(const TiXmlNode *node, const std::string &encoding = "UTF-8") { return false; }
  virtual bool Load(const CVariant &obj, const IDatabaseQueryRuleFactory *factory);
  virtual bool Save(TiXmlNode *parent) const;
  virtual bool Save(CVariant &obj) const;

  std::string GetWhereClause(const CDatabase &db, const std::string& strType) const;
  std::string TranslateCombinationType() const;

  Combination GetType() const { return m_type; }
  void SetType(Combination combination) { m_type = combination; }

  bool empty() const { return m_combinations.empty() && m_rules.empty(); }
  
  void SetHasRoleRule(bool val) const { m_hasRoleRule = val; };
  bool GetHasRoleRule() const { return m_hasRoleRule; };

protected:
  friend class CGUIDialogSmartPlaylistEditor;
  friend class CGUIDialogMediaFilter;

  Combination m_type;
  CDatabaseQueryRuleCombinations m_combinations;
  CDatabaseQueryRules m_rules;
  
  mutable bool m_hasRoleRule;
};

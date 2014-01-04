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
#include <vector>
#include <boost/shared_ptr.hpp>

#include "utils/StdString.h"

class CDatabase;
class CVariant;
class TiXmlNode;

class CDatabaseQueryRule
{
public:
  CDatabaseQueryRule();
  virtual ~CDatabaseQueryRule() { };

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

  static CStdString           GetLocalizedOperator(SEARCH_OPERATOR oper);
  static void                 GetAvailableOperators(std::vector<std::string> &operatorList);

  CStdString                  GetParameter() const;
  void                        SetParameter(const CStdString &value);
  void                        SetParameter(const std::vector<CStdString> &values);

  virtual CStdString          GetWhereClause(const CDatabase &db, const CStdString& strType) const;

  int                         m_field;
  SEARCH_OPERATOR             m_operator;
  std::vector<CStdString>     m_parameter;

protected:
  virtual CStdString          GetField(int field, const CStdString& type) const=0;
  virtual FIELD_TYPE          GetFieldType(int field) const=0;
  virtual int                 TranslateField(const char *field) const=0;
  virtual CStdString          TranslateField(int field) const=0;
  CStdString                  ValidateParameter(const CStdString &parameter) const;
  virtual CStdString          FormatParameter(const CStdString &negate, const CStdString &oper, const CDatabase &db, const CStdString &type) const;
  virtual CStdString          FormatWhereClause(const CStdString &negate, const CStdString &oper, const CStdString &param,
                                                const CDatabase &db, const CStdString &type) const;
  virtual SEARCH_OPERATOR     GetOperator(const CStdString &type) const { return m_operator; };
  virtual CStdString          GetOperatorString(SEARCH_OPERATOR op) const;
  virtual CStdString          GetBooleanQuery(const CStdString &negate, const CStdString &strType) const { return ""; }

  static SEARCH_OPERATOR      TranslateOperator(const char *oper);
  static CStdString           TranslateOperator(SEARCH_OPERATOR oper);
};

class CDatabaseQueryRuleCombination;

typedef std::vector< boost::shared_ptr<CDatabaseQueryRule> > CDatabaseQueryRules;
typedef std::vector< boost::shared_ptr<CDatabaseQueryRuleCombination> > CDatabaseQueryRuleCombinations;

class IDatabaseQueryRuleFactory
{
public:
  virtual CDatabaseQueryRule *CreateRule() const=0;
  virtual CDatabaseQueryRuleCombination *CreateCombination() const=0;
};

class CDatabaseQueryRuleCombination
{
public:
  CDatabaseQueryRuleCombination();
  virtual ~CDatabaseQueryRuleCombination() { };

  typedef enum {
    CombinationOr = 0,
    CombinationAnd
  } Combination;

  void clear();
  virtual bool Load(const TiXmlNode *node, const std::string &encoding = "UTF-8") { return false; }
  virtual bool Load(const CVariant &obj, const IDatabaseQueryRuleFactory *factory);
  virtual bool Save(TiXmlNode *parent) const;
  virtual bool Save(CVariant &obj) const;

  CStdString GetWhereClause(const CDatabase &db, const CStdString& strType) const;
  std::string TranslateCombinationType() const;

  Combination GetType() const { return m_type; }
  void SetType(Combination combination) { m_type = combination; }

  bool empty() const { return m_combinations.empty() && m_rules.empty(); }

protected:
  friend class CGUIDialogSmartPlaylistEditor;
  friend class CGUIDialogMediaFilter;

  Combination m_type;
  CDatabaseQueryRuleCombinations m_combinations;
  CDatabaseQueryRules m_rules;
};

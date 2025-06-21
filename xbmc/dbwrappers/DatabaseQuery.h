/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

constexpr const char* DATABASEQUERY_RULE_VALUE_SEPARATOR = " / ";

class CDatabase;
class CVariant;
class TiXmlNode;

class CDatabaseQueryRule
{
public:
  CDatabaseQueryRule() = default;
  virtual ~CDatabaseQueryRule() = default;

  enum class SearchOperator
  {
    OPERATOR_START = 0,
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

  enum class FieldType
  {
    TEXT_FIELD = 0,
    REAL_FIELD,
    NUMERIC_FIELD,
    DATE_FIELD,
    PLAYLIST_FIELD,
    SECONDS_FIELD,
    BOOLEAN_FIELD,
    TEXTIN_FIELD
  };

  virtual bool Load(const TiXmlNode* node, const std::string& encoding = "UTF-8");
  virtual bool Load(const CVariant& obj);
  virtual bool Save(TiXmlNode* parent) const;
  virtual bool Save(CVariant& obj) const;

  static std::string GetLocalizedOperator(SearchOperator oper);
  static void GetAvailableOperators(std::vector<std::string>& operatorList);

  std::string GetParameter() const;
  void SetParameter(const std::string& value);
  void SetParameter(const std::vector<std::string>& values);

  virtual std::string GetWhereClause(const CDatabase& db, const std::string& strType) const;

  int m_field{0};
  SearchOperator m_operator{SearchOperator::OPERATOR_CONTAINS};
  std::vector<std::string> m_parameter;

protected:
  virtual std::string GetField(int field, const std::string& type) const = 0;
  virtual CDatabaseQueryRule::FieldType GetFieldType(int field) const = 0;
  virtual int TranslateField(const char* field) const = 0;
  virtual std::string TranslateField(int field) const = 0;
  std::string ValidateParameter(const std::string& parameter) const;
  virtual std::string FormatParameter(const std::string& negate,
                                      const std::string& oper,
                                      const CDatabase& db,
                                      const std::string& type) const;
  virtual std::string FormatWhereClause(const std::string& negate,
                                        const std::string& oper,
                                        const std::string& param,
                                        const CDatabase& db,
                                        const std::string& type) const;
  virtual SearchOperator GetOperator(const std::string& type) const { return m_operator; }
  virtual std::string GetOperatorString(SearchOperator op) const;
  virtual std::string GetBooleanQuery(const std::string& negate, const std::string& strType) const
  {
    return "";
  }

  static SearchOperator TranslateOperator(const char* oper);
  static std::string TranslateOperator(SearchOperator oper);
};

class CDatabaseQueryRuleCombination;

using CDatabaseQueryRules = std::vector<std::shared_ptr<CDatabaseQueryRule>>;
using CDatabaseQueryRuleCombinations = std::vector<std::shared_ptr<CDatabaseQueryRuleCombination>>;

class IDatabaseQueryRuleFactory
{
public:
  virtual ~IDatabaseQueryRuleFactory() = default;
  virtual CDatabaseQueryRule* CreateRule() const = 0;
  virtual CDatabaseQueryRuleCombination* CreateCombination() const = 0;
};

class CDatabaseQueryRuleCombination
{
public:
  virtual ~CDatabaseQueryRuleCombination() = default;

  enum class Type
  {
    COMBINATION_OR = 0,
    COMBINATION_AND
  };

  void clear();
  virtual bool Load(const TiXmlNode* node, const std::string& encoding = "UTF-8") { return false; }
  virtual bool Load(const CVariant& obj, const IDatabaseQueryRuleFactory* factory);
  virtual bool Save(TiXmlNode* parent) const;
  virtual bool Save(CVariant& obj) const;

  std::string GetWhereClause(const CDatabase& db, const std::string& strType) const;
  std::string TranslateCombinationType() const;

  Type GetType() const { return m_type; }
  void SetType(Type combination) { m_type = combination; }

  const CDatabaseQueryRuleCombinations& GetCombinations() const { return m_combinations; }
  bool empty() const { return m_combinations.empty() && m_rules.empty(); }

  size_t GetRulesAmount() const { return m_rules.size(); }
  const CDatabaseQueryRules& GetRules() const { return m_rules; }
  void AddRule(const std::shared_ptr<CDatabaseQueryRule>& rule);
  void RemoveRule(const std::shared_ptr<CDatabaseQueryRule>& rule);
  void RemoveRule(int index);
  void Reserve(size_t amount);

private:
  Type m_type{Type::COMBINATION_AND};
  CDatabaseQueryRuleCombinations m_combinations;
  CDatabaseQueryRules m_rules;
};

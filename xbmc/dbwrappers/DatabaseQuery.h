/*
 *  Copyright (C) 2013-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

class CDatabase;
class CVariant;

namespace tinyxml2
{
class XMLNode;
} // namespace tinyxml2

/*!
 * \brief A filter rule: a field, an operator and the values to compare with. This is what every
 * rule shares whatever its fields are; CDatabaseQueryRuleOf adds the fields of one user.
 */
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
    SECONDS_FIELD,
    BOOLEAN_FIELD,
    TEXTIN_FIELD
  };

  bool Load(const tinyxml2::XMLNode* node);
  bool Load(const CVariant& obj);
  bool Save(tinyxml2::XMLNode* parent) const;
  bool Save(CVariant& obj) const;

  static std::string GetLocalizedOperator(SearchOperator oper);
  static void GetAvailableOperators(std::vector<std::string>& operatorList);

  SearchOperator m_operator{SearchOperator::OPERATOR_CONTAINS};
  std::vector<std::string> m_parameter;

protected:
  virtual void SetFieldName(const char* name) = 0;
  virtual std::string GetFieldName() const = 0;
  //! The kind of value the rule's field holds
  virtual FieldType GetFieldType() const = 0;

  std::string ValidateParameter(const std::string& parameter) const;
  //! \return a value as the comparison \p operatorString makes of it, or nullopt if it is refused
  virtual std::optional<std::string> FormatParameter(const std::string& operatorString,
                                                     const std::string& param,
                                                     const CDatabase& db) const;
  std::string GetOperatorString(SearchOperator op) const;
  //! Whether the operator goes into SQL as the NOT of its opposite
  bool IsNegated(SearchOperator op) const;
  /*!
   * \brief Whether the rule asks if its field is true or false, by its operator or by a value of
   * "true" or "false", instead of comparing the field with values.
   * \param[in,out] negate turned around when the rule asks for false by a value
   */
  bool AsksTruth(SearchOperator op, std::string& negate) const;
  //! \return the SQL of a between rule on this column, or nullopt if a value cannot go into it
  std::optional<std::string> FormatBetween(const std::string& column, const CDatabase& db) const;
  /*!
   * \brief The SQL comparing a column with one of the rule's values.
   * \param hasField whether the rule names a field
   * \param parameter the value as FormatParameter() gave it
   */
  std::string FormatComparison(bool hasField,
                               const std::string& column,
                               const std::string& negate,
                               const std::string& param,
                               const std::string& parameter) const;

  static SearchOperator TranslateOperator(const char* oper);
  static std::string TranslateOperator(SearchOperator oper);
};

//! \brief The context of a user whose rules need only the database to be put into SQL.
struct CDatabaseQueryNoContext
{
};

/*!
 * \brief A filter rule over the fields of one user of the rule engine.
 * \tparam FIELD the user's fields; its zero value is no field
 * \tparam CONTEXT what the user's rules need to know to be put into SQL
 *
 * The member definitions are in DatabaseQueryImpl.h, for the source file that instantiates the
 * user's rule.
 */
template<typename FIELD, typename CONTEXT>
class CDatabaseQueryRuleOf : public CDatabaseQueryRule
{
public:
  using Context = CONTEXT;

  //! \return the SQL for the rule, or nullopt if one of its values cannot go into it
  virtual std::optional<std::string> GetWhereClause(const CDatabase& db,
                                                    const CONTEXT& context) const;

  //! Whether the rule filters; one that does not is left out of its group's SQL
  virtual bool IsFilter() const { return true; }

  FIELD m_field{};

protected:
  //! The column the field is in
  virtual std::string GetField(FIELD field, const CONTEXT& context) const = 0;
  virtual FieldType GetFieldType(FIELD field) const = 0;
  virtual FIELD TranslateField(const char* field) const = 0;
  virtual std::string TranslateField(FIELD field) const = 0;
  virtual std::optional<std::string> FormatWhereClause(const std::string& negate,
                                                       const std::string& oper,
                                                       const std::string& param,
                                                       const CDatabase& db,
                                                       const CONTEXT& context) const;
  virtual SearchOperator GetOperator(const CONTEXT& context) const { return m_operator; }
  virtual std::string GetBooleanQuery(const std::string& negate, const CONTEXT& context) const
  {
    return "";
  }

  void SetFieldName(const char* name) final { m_field = TranslateField(name); }
  std::string GetFieldName() const final { return TranslateField(m_field); }
  FieldType GetFieldType() const final { return GetFieldType(m_field); }
};

/*!
 * \brief Rules and nested groups of rules, all or any of which have to match.
 * \tparam RULE the user's rule, a CDatabaseQueryRuleOf
 *
 * The member definitions are in DatabaseQueryImpl.h, for the source file that instantiates the
 * user's group.
 */
template<typename RULE>
class CDatabaseQueryRuleCombination
{
public:
  using Context = typename RULE::Context;
  using Rules = std::vector<std::shared_ptr<RULE>>;
  using Combinations = std::vector<std::shared_ptr<CDatabaseQueryRuleCombination>>;

  enum class Type
  {
    COMBINATION_OR = 0,
    COMBINATION_AND
  };

  void clear();
  bool Load(const CVariant& obj);
  bool Save(tinyxml2::XMLNode* parent) const;
  bool Save(CVariant& obj) const;

  //! \return the SQL for the group, or nullopt if a value of one of its rules cannot go into it
  std::optional<std::string> GetWhereClause(const CDatabase& db, const Context& context) const;
  std::string TranslateCombinationType() const;

  Type GetType() const { return m_type; }
  void SetType(Type combination) { m_type = combination; }

  const Combinations& GetCombinations() const { return m_combinations; }
  bool empty() const { return m_combinations.empty() && m_rules.empty(); }

  size_t GetRulesAmount() const { return m_rules.size(); }
  const Rules& GetRules() const { return m_rules; }
  void AddRule(const std::shared_ptr<RULE>& rule);
  void RemoveRule(const std::shared_ptr<RULE>& rule);
  void RemoveRule(int index);

private:
  Type m_type{Type::COMBINATION_AND};
  Combinations m_combinations;
  Rules m_rules;
};

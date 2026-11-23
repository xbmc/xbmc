/*
 *  Copyright (C) 2013-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DatabaseQuery.h"
#include "utils/Variant.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>

template<typename FIELD, typename CONTEXT>
std::optional<std::string> CDatabaseQueryRuleOf<FIELD, CONTEXT>::GetWhereClause(
    const CDatabase& db, const CONTEXT& context) const
{
  const SearchOperator op{GetOperator(context)};
  const std::string operatorString{GetOperatorString(op)};
  std::string negate{IsNegated(op) ? " NOT " : ""};

  if (AsksTruth(op, negate))
  {
    return GetBooleanQuery(negate, context);
  }

  if (op == SearchOperator::OPERATOR_BETWEEN)
  {
    return FormatBetween(GetField(m_field, context), db);
  }

  std::string wholeQuery;
  for (auto it = m_parameter.begin(); it != m_parameter.end(); ++it)
  {
    const std::optional<std::string> clause{
        FormatWhereClause(negate, operatorString, *it, db, context)};
    if (!clause)
    {
      return std::nullopt;
    }
    wholeQuery += '(' + *clause + ')';
    if (it + 1 != m_parameter.end())
    {
      wholeQuery += negate.empty() ? " OR " : " AND ";
    }
  }
  return wholeQuery;
}

template<typename FIELD, typename CONTEXT>
std::optional<std::string> CDatabaseQueryRuleOf<FIELD, CONTEXT>::FormatWhereClause(
    const std::string& negate,
    const std::string& oper,
    const std::string& param,
    const CDatabase& db,
    const CONTEXT& context) const
{
  const std::optional<std::string> parameter{FormatParameter(oper, param, db)};
  if (!parameter)
  {
    return std::nullopt;
  }
  const bool hasField{m_field != FIELD{}};
  return FormatComparison(hasField, hasField ? GetField(m_field, context) : std::string{}, negate,
                          param, *parameter);
}

template<typename RULE>
void CDatabaseQueryRuleCombination<RULE>::clear()
{
  m_combinations.clear();
  m_rules.clear();
  m_type = Type::COMBINATION_AND;
}

template<typename RULE>
std::optional<std::string> CDatabaseQueryRuleCombination<RULE>::GetWhereClause(
    const CDatabase& db, const Context& context) const
{
  const char* const join{m_type == Type::COMBINATION_AND ? " AND " : " OR "};
  std::string where;

  for (auto it = m_combinations.begin(); it != m_combinations.end(); ++it)
  {
    if (it != m_combinations.begin())
    {
      where += join;
    }
    const std::optional<std::string> clause{(*it)->GetWhereClause(db, context)};
    if (!clause)
    {
      return std::nullopt;
    }
    where += "(" + *clause + ")";
  }

  for (const auto& rule : m_rules)
  {
    if (!rule->IsFilter())
    {
      continue;
    }

    if (!where.empty())
    {
      where += join;
    }
    const std::optional<std::string> clause{rule->GetWhereClause(db, context)};
    if (!clause)
    {
      return std::nullopt;
    }
    // A rule without SQL of its own neither narrows a group of all nor widens a group of any
    where += "(";
    where += !clause->empty() ? *clause : m_type == Type::COMBINATION_AND ? "'1'" : "'0'";
    where += ")";
  }

  return where;
}

template<typename RULE>
bool CDatabaseQueryRuleCombination<RULE>::Load(const CVariant& obj)
{
  if (!obj.isObject() && !obj.isArray())
  {
    return false;
  }

  CVariant child;
  if (obj.isObject())
  {
    if (obj.isMember("and") && obj["and"].isArray())
    {
      m_type = Type::COMBINATION_AND;
      child = obj["and"];
    }
    else if (obj.isMember("or") && obj["or"].isArray())
    {
      m_type = Type::COMBINATION_OR;
      child = obj["or"];
    }
    else
    {
      return false;
    }
  }
  else
  {
    child = obj;
  }

  for (CVariant::const_iterator_array it = child.begin_array(); it != child.end_array(); ++it)
  {
    if (!it->isObject())
    {
      continue;
    }

    if (it->isMember("and") || it->isMember("or"))
    {
      const auto combination{std::make_shared<CDatabaseQueryRuleCombination>()};
      if (combination->Load(*it))
      {
        m_combinations.push_back(combination);
      }
    }
    else
    {
      const auto rule{std::make_shared<RULE>()};
      if (rule->Load(*it))
      {
        m_rules.push_back(rule);
      }
    }
  }

  return true;
}

template<typename RULE>
bool CDatabaseQueryRuleCombination<RULE>::Save(tinyxml2::XMLNode* parent) const
{
  for (const auto& rule : m_rules)
  {
    rule->Save(parent);
  }
  return true;
}

template<typename RULE>
bool CDatabaseQueryRuleCombination<RULE>::Save(CVariant& obj) const
{
  if (!obj.isObject() || (m_combinations.empty() && m_rules.empty()))
  {
    return false;
  }

  CVariant comboArray(CVariant::VariantTypeArray);
  for (const auto& combination : m_combinations)
  {
    CVariant comboObj(CVariant::VariantTypeObject);
    if (combination->Save(comboObj))
    {
      comboArray.push_back(comboObj);
    }
  }
  for (const auto& rule : m_rules)
  {
    CVariant ruleObj(CVariant::VariantTypeObject);
    if (rule->Save(ruleObj))
    {
      comboArray.push_back(ruleObj);
    }
  }

  obj[TranslateCombinationType()] = comboArray;

  return true;
}

template<typename RULE>
std::string CDatabaseQueryRuleCombination<RULE>::TranslateCombinationType() const
{
  return m_type == Type::COMBINATION_AND ? "and" : "or";
}

template<typename RULE>
void CDatabaseQueryRuleCombination<RULE>::AddRule(const std::shared_ptr<RULE>& rule)
{
  m_rules.emplace_back(rule);
}

template<typename RULE>
void CDatabaseQueryRuleCombination<RULE>::RemoveRule(const std::shared_ptr<RULE>& rule)
{
  const auto it = std::ranges::find(m_rules, rule);
  if (it != m_rules.cend())
  {
    m_rules.erase(it);
  }
}

template<typename RULE>
void CDatabaseQueryRuleCombination<RULE>::RemoveRule(int index)
{
  if (index < 0 || index >= static_cast<int>(m_rules.size()))
  {
    return;
  }

  m_rules.erase(m_rules.begin() + index);
}

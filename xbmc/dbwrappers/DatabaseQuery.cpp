/*
 *  Copyright (C) 2013-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DatabaseQuery.h"

#include "Database.h"
#include "ServiceBroker.h"
#include "XBDateTime.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using enum CDatabaseQueryRule::SearchOperator;
using enum CDatabaseQueryRule::FieldType;

namespace
{
struct OperatorField
{
  std::string name;
  CDatabaseQueryRule::SearchOperator op{OPERATOR_START};
  int localizedName{0};
};

// clang-format off
const std::array<OperatorField, 15> operators = {{
  {"contains",        OPERATOR_CONTAINS,          21400},
  {"doesnotcontain",  OPERATOR_DOES_NOT_CONTAIN,  21401},
  {"is",              OPERATOR_EQUALS,            21402},
  {"isnot",           OPERATOR_DOES_NOT_EQUAL,    21403},
  {"startswith",      OPERATOR_STARTS_WITH,       21404},
  {"endswith",        OPERATOR_ENDS_WITH,         21405},
  {"greaterthan",     OPERATOR_GREATER_THAN,      21406},
  {"lessthan",        OPERATOR_LESS_THAN,         21407},
  {"after",           OPERATOR_AFTER,             21408},
  {"before",          OPERATOR_BEFORE,            21409},
  {"inthelast",       OPERATOR_IN_THE_LAST,       21410},
  {"notinthelast",    OPERATOR_NOT_IN_THE_LAST,   21411},
  {"true",            OPERATOR_TRUE,              20122},
  {"false",           OPERATOR_FALSE,             20424},
  {"between",         OPERATOR_BETWEEN,           21398},
}};
// clang-format on

bool HoldsNumbers(CDatabaseQueryRule::FieldType type)
{
  return type == REAL_FIELD || type == NUMERIC_FIELD || type == SECONDS_FIELD;
}

//! Whether \p value is a decimal number, as it goes into the SQL unquoted. Logs a refusal.
bool AcceptNumber(std::string_view field, std::string_view value)
{
  std::string_view digits{value};
  if (digits.starts_with('-'))
  {
    digits.remove_prefix(1);
  }
  bool point{false};
  bool digit{false};
  for (const char c : digits)
  {
    if (c >= '0' && c <= '9')
    {
      digit = true;
    }
    else if (c == '.' && !point)
    {
      point = true;
    }
    else
    {
      digit = false;
      break;
    }
  }

  if (!digit)
  {
    CLog::LogF(LOGERROR, "Refusing the rule on field \"{}\": \"{}\" is not a number", field, value);
  }
  return digit;
}

} // unnamed namespace

bool CDatabaseQueryRule::Load(const tinyxml2::XMLNode* node)
{
  if (!node)
    return false;

  const tinyxml2::XMLElement* element = node->ToElement();
  if (!element)
    return false;

  // format is:
  // <rule field="Genre" operator="contains">parameter</rule>
  // where parameter can either be a string or a list of
  // <value> tags containing a string
  const char* field = element->Attribute("field");
  const char* oper = element->Attribute("operator");
  if (!field || !oper)
    return false;

  SetFieldName(field);
  m_operator = TranslateOperator(oper);

  if (m_operator == OPERATOR_TRUE || m_operator == OPERATOR_FALSE)
    return true;

  const tinyxml2::XMLNode* parameter = element->FirstChild();
  if (!parameter)
    return false;

  if (parameter->ToText())
  {
    if (*parameter->Value() != '\0')
    {
      m_parameter.emplace_back(parameter->Value());
    }
  }
  else if (parameter->ToElement())
  {
    const tinyxml2::XMLElement* valueNode = element->FirstChildElement("value");
    while (valueNode)
    {
      const tinyxml2::XMLNode* value = valueNode->FirstChild();
      if (value && value->ToText())
      {
        if (*value->Value() != '\0')
        {
          m_parameter.emplace_back(value->Value());
        }
      }

      valueNode = valueNode->NextSiblingElement("value");
    }
  }
  else
    return false;

  return true;
}

bool CDatabaseQueryRule::Load(const CVariant& obj)
{
  if (!obj.isMember("field") || !obj["field"].isString() || !obj.isMember("operator") ||
      !obj["operator"].isString())
    return false;

  SetFieldName(obj["field"].asString().c_str());
  m_operator = TranslateOperator(obj["operator"].asString().c_str());

  if (m_operator == OPERATOR_TRUE || m_operator == OPERATOR_FALSE)
    return true;

  if (!obj.isMember("value") || (!obj["value"].isString() && !obj["value"].isArray()))
    return false;

  const CVariant& value = obj["value"];
  if (value.isString())
    m_parameter.push_back(value.asString());
  else if (value.isArray())
  {
    for (CVariant::const_iterator_array val = value.begin_array(); val != value.end_array(); ++val)
    {
      if (val->isString() && !val->asString().empty())
        m_parameter.push_back(val->asString());
    }
    if (m_parameter.empty())
      m_parameter.emplace_back("");
  }
  else
    return false;

  return true;
}

bool CDatabaseQueryRule::Save(tinyxml2::XMLNode* parent) const
{
  if (!parent ||
      (m_parameter.empty() && m_operator != OPERATOR_TRUE && m_operator != OPERATOR_FALSE))
    return false;

  tinyxml2::XMLDocument* doc = parent->GetDocument();
  tinyxml2::XMLElement* rule = doc->NewElement("rule");
  rule->SetAttribute("field", GetFieldName().c_str());
  rule->SetAttribute("operator", TranslateOperator(m_operator).c_str());

  for (const auto& it : m_parameter)
  {
    tinyxml2::XMLElement* value = doc->NewElement("value");
    value->InsertEndChild(doc->NewText(it.c_str()));
    rule->InsertEndChild(value);
  }

  parent->InsertEndChild(rule);

  return true;
}

bool CDatabaseQueryRule::Save(CVariant& obj) const
{
  if (obj.isNull() ||
      (m_parameter.empty() && m_operator != OPERATOR_TRUE && m_operator != OPERATOR_FALSE))
    return false;

  obj["field"] = GetFieldName();
  obj["operator"] = TranslateOperator(m_operator);
  obj["value"] = m_parameter;

  return true;
}

CDatabaseQueryRule::SearchOperator CDatabaseQueryRule::TranslateOperator(const char* oper)
{
  for (const auto& o : operators)
    if (StringUtils::EqualsNoCase(oper, o.name))
      return o.op;
  return OPERATOR_CONTAINS;
}

std::string CDatabaseQueryRule::TranslateOperator(SearchOperator oper)
{
  for (const auto& o : operators)
    if (oper == o.op)
      return o.name;
  return "contains";
}

std::string CDatabaseQueryRule::GetLocalizedOperator(SearchOperator oper)
{
  for (const auto& o : operators)
    if (oper == o.op)
      return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(o.localizedName);
  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(16018);
}

void CDatabaseQueryRule::GetAvailableOperators(std::vector<std::string>& operatorList)
{
  for (const auto& o : operators)
    operatorList.emplace_back(o.name);
}

std::string CDatabaseQueryRule::ValidateParameter(const std::string& parameter) const
{
  if (HoldsNumbers(GetFieldType()) && parameter.empty())
    return "0"; // interpret empty fields as 0
  return parameter;
}

std::optional<std::string> CDatabaseQueryRule::FormatParameter(const std::string& operatorString,
                                                               const std::string& param,
                                                               const CDatabase& db) const
{
  const FieldType fieldType{GetFieldType()};
  std::string parameter;
  if (fieldType == TEXTIN_FIELD)
  {
    std::vector<std::string> split = StringUtils::Split(param, ',');
    for (std::string& itIn : split)
    {
      if (!parameter.empty())
        parameter += ",";
      parameter += db.PrepareSQL("'%s'", StringUtils::Trim(itIn).c_str());
    }
    parameter = " IN (" + parameter + ")";
  }
  else
  {
    const std::string value{ValidateParameter(param)};
    if (HoldsNumbers(fieldType) && !AcceptNumber(GetFieldName(), value))
    {
      return std::nullopt;
    }
    parameter = db.PrepareSQL(operatorString, value.c_str());
  }

  if (fieldType == DATE_FIELD)
  {
    if (m_operator == OPERATOR_IN_THE_LAST || m_operator == OPERATOR_NOT_IN_THE_LAST)
    { // translate time period
      CDateTime date = CDateTime::GetCurrentDateTime();
      CDateTimeSpan span;
      span.SetFromPeriod(param);
      date -= span;
      parameter = db.PrepareSQL(operatorString, date.GetAsDBDate().c_str());
    }
  }
  return parameter;
}

std::string CDatabaseQueryRule::GetOperatorString(SearchOperator op) const
{
  const FieldType fieldType{GetFieldType()};
  const bool numbers{HoldsNumbers(fieldType)};
  std::string operatorString;
  if (fieldType != TEXTIN_FIELD)
  {
    // the comparison piece
    switch (op)
    {
      case OPERATOR_CONTAINS:
        operatorString = " LIKE '%%%s%%'";
        break;
      case OPERATOR_DOES_NOT_CONTAIN:
        operatorString = " LIKE '%%%s%%'";
        break;
      case OPERATOR_EQUALS:
        if (numbers)
          operatorString = " = %s";
        else
          operatorString = " LIKE '%s'";
        break;
      case OPERATOR_DOES_NOT_EQUAL:
        if (numbers)
          operatorString = " != %s";
        else
          operatorString = " LIKE '%s'";
        break;
      case OPERATOR_STARTS_WITH:
        operatorString = " LIKE '%s%%'";
        break;
      case OPERATOR_ENDS_WITH:
        operatorString = " LIKE '%%%s'";
        break;
      case OPERATOR_AFTER:
      case OPERATOR_GREATER_THAN:
      case OPERATOR_IN_THE_LAST:
        operatorString = " > ";
        if (numbers)
          operatorString += "%s";
        else
          operatorString += "'%s'";
        break;
      case OPERATOR_BEFORE:
      case OPERATOR_LESS_THAN:
      case OPERATOR_NOT_IN_THE_LAST:
        operatorString = " < ";
        if (numbers)
          operatorString += "%s";
        else
          operatorString += "'%s'";
        break;
      case OPERATOR_TRUE:
        operatorString = " = 1";
        break;
      case OPERATOR_FALSE:
        operatorString = " = 0";
        break;
      default:
        break;
    }
  }
  return operatorString;
}

bool CDatabaseQueryRule::IsNegated(SearchOperator op) const
{
  return op == OPERATOR_DOES_NOT_CONTAIN || op == OPERATOR_FALSE ||
         (op == OPERATOR_DOES_NOT_EQUAL && !HoldsNumbers(GetFieldType()));
}

bool CDatabaseQueryRule::AsksTruth(SearchOperator op, std::string& negate) const
{
  // boolean operators don't have any values in m_parameter, they work on the operator
  if (m_operator == OPERATOR_FALSE || m_operator == OPERATOR_TRUE)
  {
    return true;
  }

  // Process boolean field with (not) EQUAL/CONTAINS "true"/"false" parameter too
  if (GetFieldType() == BOOLEAN_FIELD && !m_parameter.empty() &&
      (m_parameter[0] == "true" || m_parameter[0] == "false") &&
      (op == OPERATOR_CONTAINS || op == OPERATOR_EQUALS || op == OPERATOR_DOES_NOT_CONTAIN ||
       op == OPERATOR_DOES_NOT_EQUAL))
  {
    if (m_parameter[0] == "false")
    {
      negate = negate.empty() ? " NOT " : "";
    }
    return true;
  }
  return false;
}

std::optional<std::string> CDatabaseQueryRule::FormatBetween(const std::string& column,
                                                             const CDatabase& db) const
{
  if (m_parameter.size() != 2)
  {
    return "";
  }

  const FieldType fieldType{GetFieldType()};
  if (HoldsNumbers(fieldType) && (!AcceptNumber(GetFieldName(), m_parameter[0]) ||
                                  !AcceptNumber(GetFieldName(), m_parameter[1])))
  {
    return std::nullopt;
  }

  const char* format{"%s BETWEEN '%s' AND '%s'"};
  if (fieldType == REAL_FIELD)
  {
    format = "%s BETWEEN %s AND %s";
  }
  else if (fieldType == NUMERIC_FIELD)
  {
    format = "CAST(%s as DECIMAL(5,1)) BETWEEN %s AND %s";
  }
  else if (fieldType == SECONDS_FIELD)
  {
    format = "CAST(%s as INTEGER) BETWEEN %s AND %s";
  }
  return db.PrepareSQL(format, column.c_str(), m_parameter[0].c_str(), m_parameter[1].c_str());
}

std::string CDatabaseQueryRule::FormatComparison(bool hasField,
                                                 const std::string& column,
                                                 const std::string& negate,
                                                 const std::string& param,
                                                 const std::string& parameter) const
{
  std::string query;
  if (hasField)
  {
    std::string fmt = "{}";
    if (GetFieldType() == NUMERIC_FIELD)
      fmt = "CAST({} as DECIMAL(6,1))";
    else if (GetFieldType() == SECONDS_FIELD)
      fmt = "CAST({} as INTEGER)";

    query = StringUtils::Format(fmt, column);
    query += negate + parameter;

    // special case for matching parameters in fields that might be either empty or NULL.
    if ((param.empty() && negate.empty()) || (!param.empty() && !negate.empty()))
      query += " OR " + column + " IS NULL";
  }

  if (query == negate + parameter)
    query = "1";
  return query;
}

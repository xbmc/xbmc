/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingDependency.h"

#include "ServiceBroker.h"
#include "Setting.h"
#include "SettingDefinitions.h"
#include "SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <memory>
#include <set>
#include <stdlib.h>
#include <string>

CSettingDependencyCondition::CSettingDependencyCondition(
    CSettingsManager* settingsManager /* = nullptr */)
  : CSettingConditionItem(settingsManager), CStaticLoggerBase("CSettingDependencyCondition")
{ }

CSettingDependencyCondition::CSettingDependencyCondition(
    const std::string& setting,
    const std::string& value,
    SettingDependencyOperator op,
    bool negated /* = false */,
    CSettingsManager* settingsManager /* = nullptr */)
  : CSettingConditionItem(settingsManager),
    CStaticLoggerBase("CSettingDependencyCondition"),
    m_target(SettingDependencyTarget::Setting),
    m_operator(op)
{
  m_setting = m_name = setting;
  m_value = value;
  m_negated = negated;
}

CSettingDependencyCondition::CSettingDependencyCondition(
    const std::string& strProperty,
    const std::string& value,
    const std::string& setting /* = "" */,
    bool negated /* = false */,
    CSettingsManager* settingsManager /* = nullptr */)
  : CSettingConditionItem(settingsManager),
    CStaticLoggerBase("CSettingDependencyCondition"),
    m_target(SettingDependencyTarget::Property)
{
  m_name = strProperty;
  m_value = value;
  m_setting  = setting;
  m_negated = negated;
}

bool CSettingDependencyCondition::Deserialize(const TiXmlNode *node)
{
  if (!CSettingConditionItem::Deserialize(node))
    return false;

  auto elem = node->ToElement();
  if (elem == nullptr)
    return false;

  m_target = SettingDependencyTarget::Setting;
  auto strTarget = elem->Attribute(SETTING_XML_ATTR_ON);
  if (strTarget != nullptr && !setTarget(strTarget))
  {
    s_logger->warn("unknown target \"{}\"", strTarget);
    return false;
  }

  if (m_target != SettingDependencyTarget::Setting && m_name.empty())
  {
    s_logger->warn("missing name for dependency");
    return false;
  }

  if (m_target == SettingDependencyTarget::Setting)
  {
    if (m_setting.empty())
    {
      s_logger->warn("missing setting for dependency");
      return false;
    }

    m_name = m_setting;
  }

  m_operator = SettingDependencyOperator::Equals;
  auto strOperator = elem->Attribute(SETTING_XML_ATTR_OPERATOR);
  if (strOperator != nullptr && !setOperator(strOperator))
  {
    s_logger->warn("unknown operator \"{}\"", strOperator);
    return false;
  }

  return true;
}

bool CSettingDependencyCondition::Check() const
{
  if (m_name.empty() ||
      m_target == SettingDependencyTarget::Unknown ||
      m_operator == SettingDependencyOperator::Unknown ||
      m_settingsManager == nullptr)
    return false;

  bool result = false;
  switch (m_target)
  {
    case SettingDependencyTarget::Setting:
    {
      if (m_setting.empty())
        return false;

      auto setting = m_settingsManager->GetSetting(m_setting);
      if (setting == nullptr)
      {
        s_logger->warn("unable to check condition on unknown setting \"{}\"", m_setting);
        return false;
      }

      switch (m_operator)
      {
        case SettingDependencyOperator::Equals:
          result = setting->Equals(m_value);
          break;

        case SettingDependencyOperator::LessThan:
        {
          const auto value = setting->ToString();
          if (StringUtils::IsInteger(m_value))
            result = strtol(value.c_str(), nullptr, 0) < strtol(m_value.c_str(), nullptr, 0);
          else
            result = value.compare(m_value) < 0;
          break;
        }

        case SettingDependencyOperator::GreaterThan:
        {
          const auto value = setting->ToString();
          if (StringUtils::IsInteger(m_value))
            result = strtol(value.c_str(), nullptr, 0) > strtol(m_value.c_str(), nullptr, 0);
          else
            result = value.compare(m_value) > 0;
          break;
        }

        case SettingDependencyOperator::Contains:
          result = (setting->ToString().find(m_value) != std::string::npos);
          break;

        case SettingDependencyOperator::Unknown:
        default:
          break;
      }

      break;
    }

    case SettingDependencyTarget::Property:
    {
      SettingConstPtr setting;
      if (!m_setting.empty())
      {
        setting = m_settingsManager->GetSetting(m_setting);
        if (setting == nullptr)
        {
          s_logger->warn("unable to check condition on unknown setting \"{}\"", m_setting);
          return false;
        }
      }
      result = m_settingsManager->GetConditions().Check(m_name, m_value, setting);
      break;
    }

    default:
      return false;
  }

  return result == !m_negated;
}

bool CSettingDependencyCondition::setTarget(const std::string &target)
{
  if (StringUtils::EqualsNoCase(target, "setting"))
    m_target = SettingDependencyTarget::Setting;
  else if (StringUtils::EqualsNoCase(target, "property"))
    m_target = SettingDependencyTarget::Property;
  else
    return false;

  return true;
}

bool CSettingDependencyCondition::setOperator(const std::string &op)
{
  size_t length = 0;
  if (StringUtils::EndsWithNoCase(op, "is"))
  {
    m_operator = SettingDependencyOperator::Equals;
    length = 2;
  }
  else if (StringUtils::EndsWithNoCase(op, "lessthan"))
  {
    m_operator = SettingDependencyOperator::LessThan;
    length = 8;
  }
  else if (StringUtils::EndsWithNoCase(op, "lt"))
  {
    m_operator = SettingDependencyOperator::LessThan;
    length = 2;
  }
  else if (StringUtils::EndsWithNoCase(op, "greaterthan"))
  {
    m_operator = SettingDependencyOperator::GreaterThan;
    length = 11;
  }
  else if (StringUtils::EndsWithNoCase(op, "gt"))
  {
    m_operator = SettingDependencyOperator::GreaterThan;
    length = 2;
  }
  else if (StringUtils::EndsWithNoCase(op, "contains"))
  {
    m_operator = SettingDependencyOperator::Contains;
    length = 8;
  }

  if (op.size() > length + 1)
    return false;
  if (op.size() == length + 1)
  {
    if (!StringUtils::StartsWith(op, "!"))
      return false;
    m_negated = true;
  }

  return true;
}

bool CSettingDependencyConditionCombination::Deserialize(const TiXmlNode *node)
{
  if (node == nullptr)
    return false;

  size_t numOperations = m_operations.size();
  size_t numValues = m_values.size();

  if (!CSettingConditionCombination::Deserialize(node))
    return false;

  if (numOperations < m_operations.size())
  {
    for (size_t i = numOperations; i < m_operations.size(); i++)
    {
      if (m_operations[i] == nullptr)
        continue;

      auto combination = static_cast<CSettingDependencyConditionCombination*>(m_operations[i].get());
      if (combination == nullptr)
        continue;

      const std::set<std::string>& settings = combination->GetSettings();
      m_settings.insert(settings.begin(), settings.end());
    }
  }

  if (numValues < m_values.size())
  {
    for (size_t i = numValues; i < m_values.size(); i++)
    {
      if (m_values[i] == nullptr)
        continue;

      auto condition = static_cast<CSettingDependencyCondition*>(m_values[i].get());
      if (condition == nullptr)
        continue;

      auto settingId = condition->GetSetting();
      if (!settingId.empty())
        m_settings.insert(settingId);
    }
  }

  return true;
}

CSettingDependencyConditionCombination* CSettingDependencyConditionCombination::Add(
    const CSettingDependencyConditionPtr& condition)
{
  if (condition != nullptr)
  {
    m_values.push_back(condition);

    auto settingId = condition->GetSetting();
    if (!settingId.empty())
      m_settings.insert(settingId);
  }

  return this;
}

CSettingDependencyConditionCombination* CSettingDependencyConditionCombination::Add(
    const CSettingDependencyConditionCombinationPtr& operation)
{
  if (operation != nullptr)
  {
    m_operations.push_back(operation);

    const auto& settings = operation->GetSettings();
    m_settings.insert(settings.begin(), settings.end());
  }

  return this;
}

CSettingDependency::CSettingDependency(CSettingsManager* settingsManager /* = nullptr */)
  : CSettingCondition(settingsManager), CStaticLoggerBase("CSettingDependency")
{
  m_operation = CBooleanLogicOperationPtr(new CSettingDependencyConditionCombination(m_settingsManager));
}

CSettingDependency::CSettingDependency(SettingDependencyType type,
                                       CSettingsManager* settingsManager /* = nullptr */)
  : CSettingCondition(settingsManager), CStaticLoggerBase("CSettingDependency"), m_type(type)
{
  m_operation = CBooleanLogicOperationPtr(new CSettingDependencyConditionCombination(m_settingsManager));
}

bool CSettingDependency::Deserialize(const TiXmlNode *node)
{
  if (node == nullptr)
    return false;

  auto elem = node->ToElement();
  if (elem == nullptr)
    return false;

  auto strType = elem->Attribute(SETTING_XML_ATTR_TYPE);
  if (strType == nullptr || strlen(strType) <= 0 || !setType(strType))
  {
    s_logger->warn("missing or unknown dependency type definition");
    return false;
  }

  return CSettingCondition::Deserialize(node);
}

std::set<std::string> CSettingDependency::GetSettings() const
{
  if (m_operation == nullptr)
    return std::set<std::string>();

  auto combination = static_cast<CSettingDependencyConditionCombination*>(m_operation.get());
  if (combination == nullptr)
    return std::set<std::string>();

  return combination->GetSettings();
}

CSettingDependencyConditionCombinationPtr CSettingDependency::And()
{
  if (m_operation == nullptr)
    m_operation = CBooleanLogicOperationPtr(new CSettingDependencyConditionCombination(m_settingsManager));

  m_operation->SetOperation(BooleanLogicOperationAnd);

  return std::dynamic_pointer_cast<CSettingDependencyConditionCombination>(m_operation);
}

CSettingDependencyConditionCombinationPtr CSettingDependency::Or()
{
  if (m_operation == nullptr)
    m_operation = CBooleanLogicOperationPtr(new CSettingDependencyConditionCombination(m_settingsManager));

  m_operation->SetOperation(BooleanLogicOperationOr);

  return std::dynamic_pointer_cast<CSettingDependencyConditionCombination>(m_operation);
}

bool CSettingDependency::setType(const std::string &type)
{
  if (StringUtils::EqualsNoCase(type, "enable"))
    m_type = SettingDependencyType::Enable;
  else if (StringUtils::EqualsNoCase(type, "update"))
    m_type = SettingDependencyType::Update;
  else if (StringUtils::EqualsNoCase(type, "visible"))
    m_type = SettingDependencyType::Visible;
  else
    return false;

  return true;
}

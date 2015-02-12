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

#include <stdlib.h>

#include "SettingDependency.h"
#include "Setting.h"
#include "SettingDefinitions.h"
#include "SettingsManager.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

CSettingDependencyCondition::CSettingDependencyCondition(CSettingsManager *settingsManager /* = NULL */)
  : CSettingConditionItem(settingsManager),
    m_target(SettingDependencyTargetNone),
    m_operator(SettingDependencyOperatorEquals)
{ }

CSettingDependencyCondition::CSettingDependencyCondition(const std::string &setting, const std::string &value,
                            SettingDependencyOperator op, bool negated /* = false */,
                            CSettingsManager *settingsManager /* = NULL */)
  : CSettingConditionItem(settingsManager),
    m_target(SettingDependencyTargetSetting),
    m_operator(op)
{
  m_setting = m_name = setting;
  m_value = value;
  m_negated = negated;
}

CSettingDependencyCondition::CSettingDependencyCondition(const std::string &strProperty, const std::string &value,
                            const std::string &setting /* = "" */, bool negated /* = false */,
                            CSettingsManager *settingsManager /* = NULL */)
  : CSettingConditionItem(settingsManager),
    m_target(SettingDependencyTargetProperty),
    m_operator(SettingDependencyOperatorEquals)
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

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  m_target = SettingDependencyTargetSetting;
  const char *strTarget = elem->Attribute(SETTING_XML_ATTR_ON);
  if (strTarget != NULL && !setTarget(strTarget))
  {
    CLog::Log(LOGWARNING, "CSettingDependencyCondition: unknown target \"%s\"", strTarget);
    return false;
  }

  if (m_target != SettingDependencyTargetSetting && m_name.empty())
  {
    CLog::Log(LOGWARNING, "CSettingDependencyCondition: missing name for dependency");
    return false;
  }

  if (m_target == SettingDependencyTargetSetting)
  {
    if (m_setting.empty())
    {
      CLog::Log(LOGWARNING, "CSettingDependencyCondition: missing setting for dependency");
      return false;
    }

    m_name = m_setting;
  }

  m_operator = SettingDependencyOperatorEquals;
  const char *strOperator = elem->Attribute(SETTING_XML_ATTR_OPERATOR);
  if (strOperator != NULL && !setOperator(strOperator))
  {
    CLog::Log(LOGWARNING, "CSettingDependencyCondition: unknown operator \"%s\"", strOperator);
    return false;
  }

  return true;
}

bool CSettingDependencyCondition::Check() const
{
  if (m_name.empty() ||
      m_target == SettingDependencyTargetNone ||
      m_operator == SettingDependencyOperatorNone ||
      m_settingsManager == NULL)
    return false;
  
  bool result = false;
  switch (m_target)
  {
    case SettingDependencyTargetSetting:
    {
      if (m_setting.empty())
        return false;

      const CSetting *setting = m_settingsManager->GetSetting(m_setting);
      if (setting == NULL)
      {
        CLog::Log(LOGWARNING, "CSettingDependencyCondition: unable to check condition on unknown setting \"%s\"", m_setting.c_str());
        return false;
      }

      if (m_operator == SettingDependencyOperatorEquals)
        result = setting->Equals(m_value);
      else if (m_operator == SettingDependencyOperatorContains)
        result = (setting->ToString().find(m_value) != std::string::npos);

      break;
    }

    case SettingDependencyTargetProperty:
    {
      const CSetting *setting = NULL;
      if (!m_setting.empty())
      {
        setting = m_settingsManager->GetSetting(m_setting);
        if (setting == NULL)
        {
          CLog::Log(LOGWARNING, "CSettingDependencyCondition: unable to check condition on unknown setting \"%s\"", m_setting.c_str());
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
    m_target = SettingDependencyTargetSetting;
  else if (StringUtils::EqualsNoCase(target, "property"))
    m_target = SettingDependencyTargetProperty;
  else
    return false;

  return true;
}

bool CSettingDependencyCondition::setOperator(const std::string &op)
{
  size_t length = 0;
  if (StringUtils::EndsWithNoCase(op, "is"))
  {
    m_operator = SettingDependencyOperatorEquals;
    length = 2;
  }
  else if (StringUtils::EndsWithNoCase(op, "contains"))
  {
    m_operator = SettingDependencyOperatorContains;
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
  if (node == NULL)
    return false;

  size_t numOperations = m_operations.size();
  size_t numValues = m_values.size();

  if (!CSettingConditionCombination::Deserialize(node))
    return false;

  if (numOperations < m_operations.size())
  {
    for (size_t i = numOperations; i < m_operations.size(); i++)
    {
      if (m_operations[i] == NULL)
        continue;

      CSettingDependencyConditionCombination *combination = static_cast<CSettingDependencyConditionCombination*>(m_operations[i].get());
      if (combination == NULL)
        continue;

      const std::set<std::string>& settings = combination->GetSettings();
      m_settings.insert(settings.begin(), settings.end());
    }
  }

  if (numValues < m_values.size())
  {
    for (size_t i = numValues; i < m_values.size(); i++)
    {
      if (m_values[i] == NULL)
        continue;

      CSettingDependencyCondition *condition = static_cast<CSettingDependencyCondition*>(m_values[i].get());
      if (condition == NULL)
        continue;

      std::string settingId = condition->GetSetting();
      if (!settingId.empty())
        m_settings.insert(settingId);
    }
  }

  return true;
}

CSettingDependencyConditionCombination* CSettingDependencyConditionCombination::Add(CSettingDependencyConditionPtr condition)
{
  if (condition != NULL)
  {
    m_values.push_back(condition);

    std::string settingId = condition->GetSetting();
    if (!settingId.empty())
      m_settings.insert(settingId);
  }

  return this;
}

CSettingDependencyConditionCombination* CSettingDependencyConditionCombination::Add(CSettingDependencyConditionCombinationPtr operation)
{
  if (operation != NULL)
  {
    m_operations.push_back(operation);

    const std::set<std::string>& settings = operation->GetSettings();
    m_settings.insert(settings.begin(), settings.end());
  }

  return this;
}

CSettingDependency::CSettingDependency(CSettingsManager *settingsManager /* = NULL */)
  : CSettingCondition(settingsManager),
    m_type(SettingDependencyTypeNone)
{
  m_operation = CBooleanLogicOperationPtr(new CSettingDependencyConditionCombination(m_settingsManager));
}

CSettingDependency::CSettingDependency(SettingDependencyType type, CSettingsManager *settingsManager /* = NULL */)
  : CSettingCondition(settingsManager),
    m_type(type)
{
  m_operation = CBooleanLogicOperationPtr(new CSettingDependencyConditionCombination(m_settingsManager));
}

bool CSettingDependency::Deserialize(const TiXmlNode *node)
{
  if (node == NULL)
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;
  
  const char *strType = elem->Attribute(SETTING_XML_ATTR_TYPE);
  if (strType == NULL || strlen(strType) <= 0 || !setType(strType))
  {
    CLog::Log(LOGWARNING, "CSettingDependency: missing or unknown dependency type definition");
    return false;
  }

  return CSettingCondition::Deserialize(node);
}

std::set<std::string> CSettingDependency::GetSettings() const
{
  if (m_operation == NULL)
    return std::set<std::string>();

  CSettingDependencyConditionCombination *combination = static_cast<CSettingDependencyConditionCombination*>(m_operation.get());
  if (combination == NULL)
    return std::set<std::string>();

  return combination->GetSettings();
}

CSettingDependencyConditionCombinationPtr CSettingDependency::And()
{
  if (m_operation == NULL)
    m_operation = CBooleanLogicOperationPtr(new CSettingDependencyConditionCombination(m_settingsManager));

  m_operation->SetOperation(BooleanLogicOperationAnd);

  return std::dynamic_pointer_cast<CSettingDependencyConditionCombination>(m_operation);
}

CSettingDependencyConditionCombinationPtr CSettingDependency::Or()
{
  if (m_operation == NULL)
    m_operation = CBooleanLogicOperationPtr(new CSettingDependencyConditionCombination(m_settingsManager));

  m_operation->SetOperation(BooleanLogicOperationOr);

  return std::dynamic_pointer_cast<CSettingDependencyConditionCombination>(m_operation);
}

bool CSettingDependency::setType(const std::string &type)
{
  if (StringUtils::EqualsNoCase(type, "enable"))
    m_type = SettingDependencyTypeEnable;
  else if (StringUtils::EqualsNoCase(type, "update"))
    m_type = SettingDependencyTypeUpdate;
  else if (StringUtils::EqualsNoCase(type, "visible"))
    m_type = SettingDependencyTypeVisible;
  else
    return false;

  return true;
}

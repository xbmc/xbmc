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

#include "SettingConditions.h"
#include "SettingDefinitions.h"
#include "SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

bool CSettingConditionItem::Deserialize(const TiXmlNode *node)
{
  if (!CBooleanLogicValue::Deserialize(node))
    return false;

  const TiXmlElement *elem = node->ToElement();
  if (elem == NULL)
    return false;

  // get the "name" attribute
  const char *strAttribute = elem->Attribute(SETTING_XML_ATTR_NAME);
  if (strAttribute != NULL)
    m_name = strAttribute;

  // get the "setting" attribute
  strAttribute = elem->Attribute(SETTING_XML_ATTR_SETTING);
  if (strAttribute != NULL)
    m_setting = strAttribute;

  return true;
}

bool CSettingConditionItem::Check() const
{
  if (m_settingsManager == NULL)
    return false;

  return m_settingsManager->GetConditions().Check(m_name, m_value, m_settingsManager->GetSetting(m_setting)) == !m_negated;
}

bool CSettingConditionCombination::Check() const
{
  bool ok = false;
  for (CBooleanLogicOperations::const_iterator operation = m_operations.begin();
       operation != m_operations.end(); ++operation)
  {
    if (*operation == NULL)
      continue;

    CSettingConditionCombination *combination = static_cast<CSettingConditionCombination*>((*operation).get());
    if (combination == NULL)
      continue;
    
    if (combination->Check())
      ok = true;
    else if (m_operation == BooleanLogicOperationAnd)
      return false;
  }

  for (CBooleanLogicValues::const_iterator value = m_values.begin();
       value != m_values.end(); ++value)
  {
    if (*value == NULL)
      continue;

    CSettingConditionItem *condition = static_cast<CSettingConditionItem*>((*value).get());
    if (condition == NULL)
      continue;

    if (condition->Check())
      ok = true;
    else if (m_operation == BooleanLogicOperationAnd)
      return false;
  }

  return ok;
}

CSettingCondition::CSettingCondition(CSettingsManager *settingsManager /* = NULL */)
  : ISettingCondition(settingsManager)
{
  m_operation = CBooleanLogicOperationPtr(new CSettingConditionCombination(settingsManager));
}

bool CSettingCondition::Check() const
{
  CSettingConditionCombination *combination = static_cast<CSettingConditionCombination*>(m_operation.get());
  if (combination == NULL)
    return false;

  return combination->Check();
}

void CSettingConditionsManager::AddCondition(const std::string &condition)
{
  if (condition.empty())
    return;

  std::string tmpCondition = condition;
  StringUtils::ToLower(tmpCondition);

  m_defines.insert(tmpCondition);
}

void CSettingConditionsManager::AddCondition(const std::string &identifier, SettingConditionCheck condition, void *data /*= NULL*/)
{
  if (identifier.empty() || condition == NULL)
    return;

  std::string tmpIdentifier = identifier;
  StringUtils::ToLower(tmpIdentifier);

  m_conditions.insert(SettingConditionPair(tmpIdentifier, std::make_pair(condition, data)));
}

bool CSettingConditionsManager::Check(const std::string &condition, const std::string &value /* = "" */, const CSetting *setting /* = NULL */) const
{
  if (condition.empty())
    return false;

  std::string tmpCondition = condition;
  StringUtils::ToLower(tmpCondition);

  // special handling of "isdefined" conditions
  if (tmpCondition == "isdefined")
  {
    std::string tmpValue = value;
    StringUtils::ToLower(tmpValue);

    return m_defines.find(tmpValue) != m_defines.end();
  }

  SettingConditionMap::const_iterator conditionIt = m_conditions.find(tmpCondition);
  if (conditionIt == m_conditions.end())
    return false;

  return conditionIt->second.first(tmpCondition, value, setting, conditionIt->second.second);
}

CSettingConditionsManager::CSettingConditionsManager()
{ }

CSettingConditionsManager::~CSettingConditionsManager()
{
  m_conditions.clear();
  m_defines.clear();
}

/*
 *      Copyright (C) 2013 Team XBMC
 *      http://kodi.tv
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

  auto elem = node->ToElement();
  if (elem == nullptr)
    return false;

  // get the "name" attribute
  auto strAttribute = elem->Attribute(SETTING_XML_ATTR_NAME);
  if (strAttribute != nullptr)
    m_name = strAttribute;

  // get the "setting" attribute
  strAttribute = elem->Attribute(SETTING_XML_ATTR_SETTING);
  if (strAttribute != nullptr)
    m_setting = strAttribute;

  return true;
}

bool CSettingConditionItem::Check() const
{
  if (m_settingsManager == nullptr)
    return false;

  return m_settingsManager->GetConditions().Check(m_name, m_value, m_settingsManager->GetSetting(m_setting)) == !m_negated;
}

bool CSettingConditionCombination::Check() const
{
  bool ok = false;
  for (const auto& operation : m_operations)
  {
    if (operation == nullptr)
      continue;

    const auto combination = std::static_pointer_cast<const CSettingConditionCombination>(operation);
    if (combination == nullptr)
      continue;

    if (combination->Check())
      ok = true;
    else if (m_operation == BooleanLogicOperationAnd)
      return false;
  }

  for (const auto& value : m_values)
  {
    if (value == nullptr)
      continue;

    const auto condition = std::static_pointer_cast<const CSettingConditionItem>(value);
    if (condition == nullptr)
      continue;

    if (condition->Check())
      ok = true;
    else if (m_operation == BooleanLogicOperationAnd)
      return false;
  }

  return ok;
}

CSettingCondition::CSettingCondition(CSettingsManager *settingsManager /* = nullptr */)
  : ISettingCondition(settingsManager)
{
  m_operation = CBooleanLogicOperationPtr(new CSettingConditionCombination(settingsManager));
}

bool CSettingCondition::Check() const
{
  auto combination = std::static_pointer_cast<CSettingConditionCombination>(m_operation);
  if (combination == nullptr)
    return false;

  return combination->Check();
}

void CSettingConditionsManager::AddCondition(std::string condition)
{
  if (condition.empty())
    return;

  StringUtils::ToLower(condition);

  m_defines.insert(condition);
}

void CSettingConditionsManager::AddCondition(std::string identifier, SettingConditionCheck condition, void *data /*= nullptr*/)
{
  if (identifier.empty() || condition == nullptr)
    return;

  StringUtils::ToLower(identifier);

  m_conditions.emplace(identifier, std::make_pair(condition, data));
}

bool CSettingConditionsManager::Check(std::string condition, const std::string &value /* = "" */, std::shared_ptr<const CSetting> setting /* = nullptr */) const
{
  if (condition.empty())
    return false;

  StringUtils::ToLower(condition);

  // special handling of "isdefined" conditions
  if (condition == "isdefined")
  {
    std::string tmpValue = value;
    StringUtils::ToLower(tmpValue);

    return m_defines.find(tmpValue) != m_defines.end();
  }

  auto conditionIt = m_conditions.find(condition);
  if (conditionIt == m_conditions.end())
    return false;

  return conditionIt->second.first(condition, value, setting, conditionIt->second.second);
}

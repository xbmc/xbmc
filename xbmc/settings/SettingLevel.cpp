/*
 *      Copyright (C) 2013 Team XBMC
 *      http://www.xbmc.org
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

#include "SettingLevel.h"
#include "SettingsManager.h"
#include "utils/log.h"
#include "utils/XBMCTinyXML.h"

#include <algorithm>

#define XML_ATTR_DEFAULT   "default"
#define XML_ATTR_VALUE     "value"

using namespace std;

bool CSettingLevelCondition::Check() const
{
  if (m_settingsManager == NULL)
    return false;

  bool found = m_settingsManager->GetConditions().Check("IsDefined", m_value);
  if (m_negated)
    return !found;

  return found;
}

CLevelCondition::CLevelCondition(CSettingsManager *settingsManager /* = NULL */)
  : CSettingCondition(settingsManager), m_level(SETTINGS_LEVEL_DEFAULT)
{
  m_operation = CBooleanLogicOperationPtr(new CSettingLevelConditionCombination(m_settingsManager));
}

CLevelCondition::CLevelCondition(const CLevelCondition &rhs)
  : CSettingCondition(rhs.m_settingsManager), m_level(rhs.m_level)
{
  m_operation = rhs.m_operation;
}

bool CLevelCondition::Deserialize(const TiXmlNode *node)
{
  if (node == NULL)
    return false;

  const TiXmlElement *element = node->ToElement();
  if (element == NULL)
    return false;

  // get the level this condition is intended for
  int level = -1;
  if (element->QueryIntAttribute(XML_ATTR_VALUE, &level) != TIXML_SUCCESS ||
        level < SettingLevelBasic || level > SettingLevelInternal)
    return false;

  if (!CSettingCondition::Deserialize(node))
    return false;

  m_level = (SettingLevel)level;
  return true;
}

CSettingLevel::CSettingLevel(CSettingsManager *settingsManager /* = NULL */)
  : m_default(SETTINGS_LEVEL_DEFAULT), m_settingsManager(settingsManager)
{ }

bool CSettingLevel::Deserialize(const TiXmlNode *node)
{
  if (node == NULL)
    return false;

  const TiXmlElement *element = node->ToElement();
  if (element == NULL)
    return false;

  // get the default level
  int def_lvl = -1;
  if (element->QueryIntAttribute(XML_ATTR_DEFAULT, &def_lvl) != TIXML_SUCCESS || def_lvl < 0)
    return false;

  // traverse <level> tags
  vector<CLevelCondition> vec_lvl;
  const TiXmlElement *level = node->FirstChildElement(XML_ELM_LEVEL);
  while (level != NULL)
  {
    CLevelCondition levelCondition(m_settingsManager);
    if (!levelCondition.Deserialize(level))
      return false;
    vec_lvl.push_back(levelCondition);
    level = level->NextSiblingElement(XML_ELM_LEVEL);
  }

  m_default = std::min((SettingLevel)def_lvl, SettingLevelInternal);
  m_levelConditions = vec_lvl;
  return true;
}

void CSettingLevel::SetLevel(SettingLevel level)
{
  m_default = level;
  m_levelConditions.clear();
}

SettingLevel CSettingLevel::GetLevel() const
{
  for (vector<CLevelCondition>::const_iterator it = m_levelConditions.begin(); it != m_levelConditions.end(); ++it)
    if (it->Check())
      return it->GetLevel();
  return m_default;
}

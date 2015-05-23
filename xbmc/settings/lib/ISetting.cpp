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

#include "ISetting.h"
#include "SettingDefinitions.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

using namespace std;

ISetting::ISetting(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : m_id(id),
    m_settingsManager(settingsManager),
    m_visible(true),
    m_label(-1), m_help(-1),
    m_meetsRequirements(true),
    m_requirementCondition(settingsManager)
{ }
  
bool ISetting::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  if (node == NULL)
    return false;

  bool value;
  if (XMLUtils::GetBoolean(node, SETTING_XML_ELM_VISIBLE, value))
    m_visible = value;

  const TiXmlElement *element = node->ToElement();
  if (element == NULL)
    return false;

  int iValue = -1;
  if (element->QueryIntAttribute(SETTING_XML_ATTR_LABEL, &iValue) == TIXML_SUCCESS && iValue > 0)
    m_label = iValue;
  if (element->QueryIntAttribute(SETTING_XML_ATTR_HELP, &iValue) == TIXML_SUCCESS && iValue > 0)
    m_help = iValue;

  const TiXmlNode *requirementNode = node->FirstChild(SETTING_XML_ELM_REQUIREMENT);
  if (requirementNode == NULL)
    return true;

  return m_requirementCondition.Deserialize(requirementNode);
}

bool ISetting::DeserializeIdentification(const TiXmlNode *node, std::string &identification)
{
  if (node == NULL)
    return false;

  const TiXmlElement *element = node->ToElement();
  if (element == NULL)
    return false;

  const char *idAttribute = element->Attribute(SETTING_XML_ATTR_ID);
  if (idAttribute == NULL || strlen(idAttribute) <= 0)
    return false;

  identification = idAttribute;
  return true;
}

void ISetting::CheckRequirements()
{
  m_meetsRequirements = m_requirementCondition.Check();
}

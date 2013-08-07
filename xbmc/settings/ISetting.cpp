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
#include "utils/log.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

#define XML_VISIBLE     "visible"
#define XML_REQUIREMENT "requirement"
#define XML_CONDITION   "condition"

using namespace std;

ISetting::ISetting(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : m_id(id),
    m_settingsManager(settingsManager),
    m_visible(true),
    m_meetsRequirements(true),
    m_requirementCondition(settingsManager)
{ }
  
bool ISetting::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  if (node == NULL)
    return false;

  bool value;
  if (XMLUtils::GetBoolean(node, XML_VISIBLE, value))
    m_visible = value;

  const TiXmlNode *requirementNode = node->FirstChild(XML_REQUIREMENT);
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

  const char *idAttribute = element->Attribute(XML_ATTR_ID);
  if (idAttribute == NULL || strlen(idAttribute) <= 0)
    return false;

  identification = idAttribute;
  return true;
}

void ISetting::CheckRequirements()
{
  m_meetsRequirements = m_requirementCondition.Check();
}

/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ISetting.h"

#include "SettingDefinitions.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"

#include <string>

ISetting::ISetting(const std::string &id, CSettingsManager *settingsManager /* = nullptr */)
  : m_id(id)
  , m_settingsManager(settingsManager)
  , m_requirementCondition(settingsManager)
{ }

bool ISetting::Deserialize(const tinyxml2::XMLNode* node, bool update /* = false */)
{
  if (node == nullptr)
    return false;

  bool value;
  if (XMLUtils::GetBoolean(node, SETTING_XML_ELM_VISIBLE, value))
    m_visible = value;

  auto element = node->ToElement();
  if (element == nullptr)
    return false;

  int iValue = -1;
  if (element->QueryIntAttribute(SETTING_XML_ATTR_LABEL, &iValue) == tinyxml2::XML_SUCCESS &&
      iValue > 0)
    m_label = iValue;
  if (element->QueryIntAttribute(SETTING_XML_ATTR_HELP, &iValue) == tinyxml2::XML_SUCCESS &&
      iValue > 0)
    m_help = iValue;

  auto requirementNode = node->FirstChildElement(SETTING_XML_ELM_REQUIREMENT);
  if (!requirementNode)
    return true;

  return m_requirementCondition.Deserialize(requirementNode);
}

bool ISetting::DeserializeIdentification(const tinyxml2::XMLNode* node, std::string& identification)
{
  return DeserializeIdentificationFromAttribute(node, SETTING_XML_ATTR_ID, identification);
}

bool ISetting::DeserializeIdentificationFromAttribute(const tinyxml2::XMLNode* node,
                                                      const std::string& attribute,
                                                      std::string& identification)
{
  if (node == nullptr)
    return false;

  auto element = node->ToElement();
  if (element == nullptr)
    return false;

  auto idAttribute = element->Attribute(attribute.c_str());
  if (idAttribute == nullptr || *idAttribute == 0)
    return false;

  identification = idAttribute;
  return true;
}

void ISetting::CheckRequirements()
{
  m_meetsRequirements = m_requirementCondition.Check();
}

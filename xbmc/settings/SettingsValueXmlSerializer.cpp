/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingsValueXmlSerializer.h"

#include "settings/lib/SettingDefinitions.h"
#include "settings/lib/SettingSection.h"
#include "settings/lib/SettingsManager.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

static constexpr char SETTINGS_XML_ROOT[] = "settings";

std::string CSettingsValueXmlSerializer::SerializeValues(
  const CSettingsManager* settingsManager) const
{
  if (settingsManager == nullptr)
    return "";

  CXBMCTinyXML2 xmlDoc;
  auto* rootElement = xmlDoc.NewElement(SETTINGS_XML_ROOT);
  rootElement->SetAttribute(SETTING_XML_ROOT_VERSION, settingsManager->GetVersion());
  auto* xmlRoot = xmlDoc.InsertEndChild(rootElement);
  if (xmlRoot == nullptr)
    return "";

  const auto sections = settingsManager->GetSections();
  for (const auto& section : sections)
    SerializeSection(xmlRoot, section);

  tinyxml2::XMLPrinter printer;
  xmlDoc.Print(&printer);

  std::string stream = printer.CStr();

  return stream;
}

void CSettingsValueXmlSerializer::SerializeSection(
    tinyxml2::XMLNode* parent, const std::shared_ptr<CSettingSection>& section) const
{
  if (section == nullptr)
    return;

  const auto categories = section->GetCategories();
  for (const auto& category : categories)
    SerializeCategory(parent, category);
}

void CSettingsValueXmlSerializer::SerializeCategory(
    tinyxml2::XMLNode* parent, const std::shared_ptr<CSettingCategory>& category) const
{
  if (category == nullptr)
    return;

  const auto groups = category->GetGroups();
  for (const auto& group : groups)
    SerializeGroup(parent, group);
}

void CSettingsValueXmlSerializer::SerializeGroup(tinyxml2::XMLNode* parent,
                                                 const std::shared_ptr<CSettingGroup>& group) const
{
  if (group == nullptr)
    return;

  const auto settings = group->GetSettings();
  for (const auto& setting : settings)
    SerializeSetting(parent, setting);
}

void CSettingsValueXmlSerializer::SerializeSetting(tinyxml2::XMLNode* parent,
                                                   const std::shared_ptr<CSetting>& setting) const
{
  if (setting == nullptr)
    return;

  // ignore references and action settings (which don't have a value)
  if (setting->IsReference() || setting->GetType() == SettingType::Action)
    return;

  auto* settingElement = parent->GetDocument()->NewElement(SETTING_XML_ELM_SETTING);
  settingElement->SetAttribute(SETTING_XML_ATTR_ID, setting->GetId().c_str());

  // add the default attribute
  if (setting->IsDefault())
    settingElement->SetAttribute(SETTING_XML_ELM_DEFAULT, "true");

  // add the value
  auto* value = parent->GetDocument()->NewText(setting->ToString().c_str());
  settingElement->InsertEndChild(value);

  if (parent->InsertEndChild(settingElement) == nullptr)
    CLog::Log(LOGWARNING,
      "CSettingsValueXmlSerializer: unable to write <" SETTING_XML_ELM_SETTING " id=\"{}\"> tag",
      setting->GetId());
}

/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingsValueFlatJsonSerializer.h"

#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/lib/SettingSection.h"
#include "settings/lib/SettingType.h"
#include "settings/lib/SettingsManager.h"
#include "utils/JSONVariantWriter.h"
#include "utils/log.h"

CSettingsValueFlatJsonSerializer::CSettingsValueFlatJsonSerializer(bool compact /* = true */)
  : m_compact(compact)
{ }

std::string CSettingsValueFlatJsonSerializer::SerializeValues(
  const CSettingsManager* settingsManager) const
{
  if (settingsManager == nullptr)
    return "";

  CVariant root(CVariant::VariantTypeObject);

  const auto sections = settingsManager->GetSections();
  for (const auto& section : sections)
    SerializeSection(root, section);

  std::string result;
  if (!CJSONVariantWriter::Write(root, result, m_compact))
  {
    CLog::Log(LOGWARNING,
      "CSettingsValueFlatJsonSerializer: failed to serialize settings into JSON");
    return "";
  }

  return result;
}

void CSettingsValueFlatJsonSerializer::SerializeSection(
  CVariant& parent, std::shared_ptr<CSettingSection> section) const
{
  if (section == nullptr)
    return;

  const auto categories = section->GetCategories();
  for (const auto& category : categories)
    SerializeCategory(parent, category);
}

void CSettingsValueFlatJsonSerializer::SerializeCategory(
  CVariant& parent, std::shared_ptr<CSettingCategory> category) const
{
  if (category == nullptr)
    return;

  const auto groups = category->GetGroups();
  for (const auto& group : groups)
    SerializeGroup(parent, group);
}

void CSettingsValueFlatJsonSerializer::SerializeGroup(
  CVariant& parent, std::shared_ptr<CSettingGroup> group) const
{
  if (group == nullptr)
    return;

  const auto settings = group->GetSettings();
  for (const auto& setting : settings)
    SerializeSetting(parent, setting);
}

void CSettingsValueFlatJsonSerializer::SerializeSetting(
  CVariant& parent, std::shared_ptr<CSetting> setting) const
{
  if (setting == nullptr)
    return;

  // ignore references and action settings (which don't have a value)
  if (setting->GetType() == SettingType::Reference || setting->GetType() == SettingType::Action)
    return;

  const auto valueObj = SerializeSettingValue(setting);
  if (valueObj.isNull())
    return;

  parent[setting->GetId()] = valueObj;
}

CVariant CSettingsValueFlatJsonSerializer::SerializeSettingValue(
  std::shared_ptr<CSetting> setting) const
{
  switch (setting->GetType())
  {
    case SettingType::Action:
      return CVariant::ConstNullVariant;

    case SettingType::Boolean:
      return CVariant(std::static_pointer_cast<CSettingBool>(setting)->GetValue());

    case SettingType::Integer:
      return CVariant(std::static_pointer_cast<CSettingInt>(setting)->GetValue());

    case SettingType::Number:
      return CVariant(std::static_pointer_cast<CSettingNumber>(setting)->GetValue());

    case SettingType::String:
      return CVariant(std::static_pointer_cast<CSettingString>(setting)->GetValue());

    case SettingType::List:
    {
      const auto settingList = std::static_pointer_cast<CSettingList>(setting);

      CVariant settingListValuesObj(CVariant::VariantTypeArray);
      const auto settingListValues = settingList->GetValue();
      for (const auto& settingListValue : settingListValues)
      {
        const auto valueObj = SerializeSettingValue(settingListValue);
        if (!valueObj.isNull())
          settingListValuesObj.push_back(valueObj);
      }

      return settingListValuesObj;
    }

    case SettingType::Unknown:
    default:
      CLog::Log(LOGWARNING,
        "CSettingsValueFlatJsonSerializer: failed to serialize setting \"{}\" with value \"{}\" " \
        "of unknown type", setting->GetId(), setting->ToString());
      return CVariant::ConstNullVariant;
  }
}

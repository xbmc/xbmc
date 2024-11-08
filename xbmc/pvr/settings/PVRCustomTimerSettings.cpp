/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRCustomTimerSettings.h"

#include "pvr/settings/IPVRSettingsContainer.h"
#include "pvr/settings/PVRTimerSettingDefinition.h"
#include "pvr/timers/PVRTimerType.h"
#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace PVR
{
static constexpr const char* SETTING_TMR_CUSTOM_INT{"customsetting.int"};
static constexpr const char* SETTING_TMR_CUSTOM_STRING{"customsetting.string"};

CPVRCustomTimerSettings::CPVRCustomTimerSettings(
    const CPVRTimerType& timerType,
    const CPVRTimerInfoTag::CustomPropsMap& customProps,
    const std::map<int, std::shared_ptr<CPVRTimerType>>& typeEntries)
  : m_customProps(customProps)
{
  unsigned int idx{0};
  for (const auto& type : typeEntries)
  {
    const std::vector<std::shared_ptr<const CPVRTimerSettingDefinition>>& settingDefs{
        type.second->GetCustomSettingDefinitions()};
    for (const auto& settingDef : settingDefs)
    {
      std::string settingIdPrefix;
      if (settingDef->IsIntDefinition())
      {
        settingIdPrefix = SETTING_TMR_CUSTOM_INT;
      }
      else if (settingDef->IsStringDefinition())
      {
        settingIdPrefix = SETTING_TMR_CUSTOM_STRING;
      }
      else
      {
        CLog::LogF(LOGERROR, "Unknown custom setting definition ignored!");
        continue;
      }

      const std::string settingId{StringUtils::Format("{}-{}", settingIdPrefix, idx)};
      m_customSettingDefs.emplace_back(std::make_pair(settingId, settingDef));
      ++idx;
    }
  }

  SetTimerType(timerType);
}

void CPVRCustomTimerSettings::SetTimerType(const CPVRTimerType& timerType)
{
  CPVRTimerInfoTag::CustomPropsMap newCustomProps;
  for (const auto& entry : m_customSettingDefs)
  {
    // Complete custom props for given type.
    const CPVRTimerSettingDefinition& def{*entry.second};
    if (def.GetTimerTypeId() == timerType.GetTypeId() &&
        def.GetClientId() == timerType.GetClientId())
    {
      const auto it{m_customProps.find(def.GetId())};
      if (it == m_customProps.cend())
        newCustomProps.insert({def.GetId(), {def.GetType(), def.GetDefaultValue()}});
      else
        newCustomProps.insert({def.GetId(), {(*it).second.type, (*it).second.value}});
    }
  }
  m_customProps = newCustomProps;
}

void CPVRCustomTimerSettings::AddSettings(IPVRSettingsContainer& settingsContainer,
                                          const std::shared_ptr<CSettingGroup>& group)
{
  for (const auto& settingDef : m_customSettingDefs)
  {
    const CPVRTimerSettingDefinition& def{*settingDef.second};
    const auto it{m_customProps.find(def.GetId())};
    if (it == m_customProps.cend())
      continue;

    const std::string settingName{settingDef.first};
    if (IsCustomIntSetting(settingName))
    {
      const int intValue{(*it).second.value.asInteger32()};
      const CPVRIntSettingDefinition& intDef{def.GetIntDefinition()};
      if (intDef.GetValues().empty())
        settingsContainer.AddSingleIntSetting(group, settingName, intValue, intDef.GetMinValue(),
                                              intDef.GetStepValue(), intDef.GetMaxValue());
      else
        settingsContainer.AddMultiIntSetting(group, settingName, intValue);
    }
    else if (IsCustomStringSetting(settingName))
    {
      const std::string stringValue{(*it).second.value.asString()};
      const CPVRStringSettingDefinition& stringDef{def.GetStringDefinition()};
      if (stringDef.GetValues().empty())
        settingsContainer.AddSingleStringSetting(group, settingName, stringValue,
                                                 stringDef.IsAllowEmptyValue());
      else
        settingsContainer.AddMultiStringSetting(group, settingName, stringValue);
    }
  }
}

bool CPVRCustomTimerSettings::IsCustomSetting(const std::string& settingId) const
{
  return IsCustomIntSetting(settingId) || IsCustomStringSetting(settingId);
}

bool CPVRCustomTimerSettings::IsCustomIntSetting(const std::string& settingId) const
{
  return settingId.starts_with(SETTING_TMR_CUSTOM_INT);
}

bool CPVRCustomTimerSettings::IsCustomStringSetting(const std::string& settingId) const
{
  return settingId.starts_with(SETTING_TMR_CUSTOM_STRING);
}

bool CPVRCustomTimerSettings::UpdateIntProperty(const std::shared_ptr<const CSetting>& setting)
{
  const auto def{GetSettingDefintion(setting->GetId())};
  if (!def)
  {
    CLog::LogF(LOGERROR, "Custom int setting definition not found");
    return false;
  }

  CVariant& prop{m_customProps[def->GetId()].value};
  prop = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
  return true;
}

bool CPVRCustomTimerSettings::UpdateStringProperty(const std::shared_ptr<const CSetting>& setting)
{
  const auto def{GetSettingDefintion(setting->GetId())};
  if (!def)
  {
    CLog::LogF(LOGERROR, "Custom string setting definition not found");
    return false;
  }

  CVariant& prop{m_customProps[def->GetId()].value};
  prop = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
  return true;
}

std::string CPVRCustomTimerSettings::GetSettingsLabel(const std::string& settingId) const
{
  const auto def{GetSettingDefintion(settingId)};
  if (!def)
    return {};

  return def->GetName();
}

bool CPVRCustomTimerSettings::IntSettingDefinitionsFiller(const std::string& settingId,
                                                          std::vector<IntegerSettingOption>& list,
                                                          int& current)
{
  const auto def{GetSettingDefintion(settingId)};
  if (!def)
  {
    CLog::LogF(LOGERROR, "Custom setting definition not found");
    return false;
  }

  const std::vector<SettingIntValue>& values{def->GetIntDefinition().GetValues()};
  std::transform(values.cbegin(), values.cend(), std::back_inserter(list),
                 [](const auto& value) { return IntegerSettingOption(value.first, value.second); });

  const auto it2{m_customProps.find(def->GetId())};
  if (it2 != m_customProps.cend())
    current = (*it2).second.value.asInteger32();
  else
    current = def->GetIntDefinition().GetDefaultValue();

  return true;
}

bool CPVRCustomTimerSettings::StringSettingDefinitionsFiller(const std::string& settingId,
                                                             std::vector<StringSettingOption>& list,
                                                             std::string& current)
{
  const auto def{GetSettingDefintion(settingId)};
  if (!def)
  {
    CLog::LogF(LOGERROR, "Custom setting definition not found");
    return false;
  }

  const std::vector<SettingStringValue>& values{def->GetStringDefinition().GetValues()};
  std::transform(values.cbegin(), values.cend(), std::back_inserter(list),
                 [](const auto& value) { return StringSettingOption(value.first, value.second); });

  const auto it2{m_customProps.find(def->GetId())};
  if (it2 != m_customProps.cend())
    current = (*it2).second.value.asString();
  else
    current = def->GetStringDefinition().GetDefaultValue();

  return true;
}

bool CPVRCustomTimerSettings::IsSettingReadonlyForTimerState(const std::string& settingId,
                                                             PVR_TIMER_STATE timerState) const
{
  const auto def{GetSettingDefintion(settingId)};
  if (!def)
  {
    CLog::LogF(LOGERROR, "Custom setting definition not found");
    return false;
  }

  return def->IsReadonlyForTimerState(timerState);
}

bool CPVRCustomTimerSettings::IsSettingSupportedForTimerType(const std::string& settingId,
                                                             const CPVRTimerType& timerType) const
{
  const auto def{GetSettingDefintion(settingId)};
  if (!def)
  {
    CLog::LogF(LOGERROR, "Custom setting definition not found");
    return false;
  }

  return timerType.GetClientId() == def->GetClientId() &&
         timerType.GetTypeId() == def->GetTimerTypeId();
}

std::shared_ptr<const CPVRTimerSettingDefinition> CPVRCustomTimerSettings::GetSettingDefintion(
    const std::string& settingId) const
{
  const auto it{std::find_if(m_customSettingDefs.cbegin(), m_customSettingDefs.cend(),
                             [&settingId](const auto& entry) { return entry.first == settingId; })};
  if (it == m_customSettingDefs.cend())
    return {};

  return (*it).second;
}
} // namespace PVR

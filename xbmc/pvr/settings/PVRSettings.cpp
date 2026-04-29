/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRSettings.h"

#include "ServiceBroker.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/guilib/PVRGUIActionsParentalControl.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace PVR;

unsigned int CPVRSettings::m_iInstances = 0;

CPVRSettings::CPVRSettings(const SettingsContainer& settingNames)
{
  Init(settingNames);

  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  settings->GetSettingsManager()->RegisterSettingsHandler(this);
  settings->RegisterCallback(this, settingNames);

  if (m_iInstances == 0)
  {
    // statics must only be registered once, not per instance
    settings->GetSettingsManager()->RegisterSettingOptionsFiller("pvrrecordmargins",
                                                                 MarginTimeFiller);
    settings->GetSettingsManager()->AddDynamicCondition("pvrsettingvisible", IsSettingVisible);
    settings->GetSettingsManager()->AddDynamicCondition("checkpvrparentalpin", CheckParentalPin);
  }
  m_iInstances++;
}

CPVRSettings::~CPVRSettings()
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  m_iInstances--;
  if (m_iInstances == 0)
  {
    settings->GetSettingsManager()->RemoveDynamicCondition("checkpvrparentalpin");
    settings->GetSettingsManager()->RemoveDynamicCondition("pvrsettingvisible");
    settings->GetSettingsManager()->UnregisterSettingOptionsFiller("pvrrecordmargins");
  }

  settings->UnregisterCallback(this);
  settings->GetSettingsManager()->UnregisterSettingsHandler(this);
}

void CPVRSettings::RegisterCallback(ISettingCallback* callback)
{
  std::unique_lock lock(m_critSection);
  m_callbacks.insert(callback);
}

void CPVRSettings::UnregisterCallback(ISettingCallback* callback)
{
  std::unique_lock lock(m_critSection);
  m_callbacks.erase(callback);
}

void CPVRSettings::Init(const SettingsContainer& settingNames)
{
  for (const auto& settingName : settingNames)
  {
    SettingPtr setting =
        CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(settingName);
    if (!setting)
    {
      CLog::LogF(LOGERROR, "Unknown PVR setting '{}'", settingName);
      continue;
    }

    std::unique_lock lock(m_critSection);
    m_settings.try_emplace(settingName, setting->Clone(settingName));
  }
}

void CPVRSettings::OnSettingsLoaded()
{
  SettingsContainer settingNames;

  {
    std::unique_lock lock(m_critSection);
    for (const auto& [settingName, _] : m_settings)
      settingNames.insert(settingName);

    m_settings.clear();
  }

  Init(settingNames);
}

bool CPVRSettings::OnSettingChanging(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
  {
    CLog::LogF(LOGERROR, "No setting");
    return false;
  }

  const std::string_view& settingId = setting->GetId();

  if (settingId == CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME)
  {
    auto dlgError = []()
    {
      // Time must be in HH:MM:SS format, including any leading zeroes.
      KODI::MESSAGING::HELPERS::ShowOKDialogText(257, 19688);
      return false;
    };

    const std::string& value = std::static_pointer_cast<const CSettingString>(setting)->GetValue();

    if (value.length() < 5 || value.length() > 8)
      return dlgError();

    std::vector<std::string_view> parts;
    size_t start{0};
    size_t end = value.find(':');

    while (end != std::string_view::npos)
    {
      parts.push_back(value.substr(start, end - start));
      start = end + 1;
      end = value.find(':', start);
    }
    parts.push_back(value.substr(start));

    if (parts.size() < 2 || parts.size() > 3)
      return dlgError();

    for (std::string_view sv : parts)
    {
      if (sv.length() != 2 || !std::all_of(sv.begin(), sv.end(), ::isdigit))
        return dlgError();
    }

    auto stoi = [](std::string_view sv)
    {
      int val{0};
      for (char c : sv)
        val = val * 10 + (c - '0');
      return val;
    };

    int h = stoi(parts.at(0));
    int m = stoi(parts.at(1));
    int s = (parts.size() == 3) ? stoi(parts.at(2)) : 0;

    if (h > 23 || m > 59 || s > 59)
      return dlgError();

    return true;
  }

  return true;
}

void CPVRSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
    return;

  std::unique_lock lock(m_critSection);
  m_settings[setting->GetId()] = setting->Clone(setting->GetId());
  const auto callbacks(m_callbacks);
  lock.unlock();

  for (const auto& callback : callbacks)
    callback->OnSettingChanged(setting);
}

bool CPVRSettings::GetBoolValue(const std::string& settingName) const
{
  std::unique_lock lock(m_critSection);
  auto settingIt = m_settings.find(settingName);
  if (settingIt != m_settings.end() && (*settingIt).second->GetType() == SettingType::Boolean)
  {
    std::shared_ptr<const CSettingBool> setting =
        std::dynamic_pointer_cast<const CSettingBool>((*settingIt).second);
    if (setting)
      return setting->GetValue();
  }

  CLog::LogF(LOGERROR, "PVR setting '{}' not found or wrong type given", settingName);
  return false;
}

int CPVRSettings::GetIntValue(const std::string& settingName) const
{
  std::unique_lock lock(m_critSection);
  auto settingIt = m_settings.find(settingName);
  if (settingIt != m_settings.end() && (*settingIt).second->GetType() == SettingType::Integer)
  {
    std::shared_ptr<const CSettingInt> setting =
        std::dynamic_pointer_cast<const CSettingInt>((*settingIt).second);
    if (setting)
      return setting->GetValue();
  }

  CLog::LogF(LOGERROR, "PVR setting '{}' not found or wrong type given", settingName);
  return -1;
}

std::string CPVRSettings::GetStringValue(const std::string& settingName) const
{
  std::unique_lock lock(m_critSection);
  auto settingIt = m_settings.find(settingName);
  if (settingIt != m_settings.end() && (*settingIt).second->GetType() == SettingType::String)
  {
    std::shared_ptr<const CSettingString> setting =
        std::dynamic_pointer_cast<const CSettingString>((*settingIt).second);
    if (setting)
      return setting->GetValue();
  }

  CLog::LogF(LOGERROR, "PVR setting '{}' not found or wrong type given", settingName);
  return "";
}

void CPVRSettings::MarginTimeFiller(const SettingConstPtr& /*setting*/,
                                    std::vector<IntegerSettingOption>& list,
                                    int& current)
{
  list.clear();

  static constexpr std::array<int, 13> marginTimeValues = {
      0, 1, 2, 3, 5, 10, 15, 20, 30, 60, 90, 120, 180 // minutes
  };

  for (int iValue : marginTimeValues)
  {
    list.emplace_back(
        StringUtils::Format(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(14044),
                            iValue) /* {} min */,
        iValue);
  }
}

bool CPVRSettings::IsSettingVisible(const std::string& condition,
                                    const std::string& value,
                                    const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
    return false;

  const std::string& settingId = setting->GetId();

  if (settingId == CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS)
  {
    // Setting is only visible if exactly one PVR client is enabled or
    // the expert setting to always use backend numbers is enabled
    const auto& settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    const size_t enabledClientAmount =
        CServiceBroker::GetPVRManager().Clients()->EnabledClientAmount();

    return enabledClientAmount == 1 ||
           (settings->GetBool(CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERSALWAYS) &&
            enabledClientAmount > 1);
  }
  else if (settingId == CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERSALWAYS)
  {
    // Setting is only visible if more than one PVR client is enabled.
    return CServiceBroker::GetPVRManager().Clients()->EnabledClientAmount() > 1;
  }
  else if (settingId == CSettings::SETTING_PVRMANAGER_CLIENTPRIORITIES)
  {
    // Setting is only visible if more than one PVR client is enabled.
    return CServiceBroker::GetPVRManager().Clients()->EnabledClientAmount() > 1;
  }
  else
  {
    // Show all other settings unconditionally.
    return true;
  }
}

bool CPVRSettings::CheckParentalPin(const std::string& /*condition*/,
                                    const std::string& /*value*/,
                                    const std::shared_ptr<const CSetting>& /*setting*/)
{
  return CServiceBroker::GetPVRManager().Get<PVR::GUI::Parental>().CheckParentalPIN() ==
         ParentalCheckResult::SUCCESS;
}

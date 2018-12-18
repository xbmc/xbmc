/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRSettings.h"

#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/lib/SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"

using namespace PVR;

unsigned int CPVRSettings::m_iInstances = 0;

CPVRSettings::CPVRSettings(const std::set<std::string> &settingNames)
{
  Init(settingNames);

  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  settings->GetSettingsManager()->RegisterSettingsHandler(this);
  settings->RegisterCallback(this, settingNames);

  if (m_iInstances == 0)
  {
    // statics must only be registered once, not per instance
    settings->GetSettingsManager()->RegisterSettingOptionsFiller("pvrrecordmargins", MarginTimeFiller);
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

void CPVRSettings::Init(const std::set<std::string> &settingNames)
{
  for (auto settingName : settingNames)
  {
    SettingPtr setting = CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(settingName);
    if (!setting)
    {
      CLog::LogF(LOGERROR, "Unknown PVR setting '%s'", settingName.c_str());
      continue;
    }

    CSingleLock lock(m_critSection);
    m_settings.insert(std::make_pair(settingName, setting->Clone(settingName)));
  }
}

void CPVRSettings::OnSettingsLoaded()
{
  std::set<std::string> settingNames;

  {
    CSingleLock lock(m_critSection);
    for (const auto& settingName : m_settings)
      settingNames.insert(settingName.first);

    m_settings.clear();
  }

  Init(settingNames);
}

void CPVRSettings::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting == nullptr)
    return;

  CSingleLock lock(m_critSection);
  m_settings[setting->GetId()] = setting->Clone(setting->GetId());
}

bool CPVRSettings::GetBoolValue(const std::string &settingName) const
{
  CSingleLock lock(m_critSection);
  auto settingIt = m_settings.find(settingName);
  if (settingIt != m_settings.end() && (*settingIt).second->GetType() == SettingType::Boolean)
  {
    std::shared_ptr<const CSettingBool> setting = std::dynamic_pointer_cast<const CSettingBool>((*settingIt).second);
    if (setting)
      return setting->GetValue();
  }

  CLog::LogF(LOGERROR, "PVR setting '%s' not found or wrong type given", settingName.c_str());
  return false;
}

int CPVRSettings::GetIntValue(const std::string &settingName) const
{
  CSingleLock lock(m_critSection);
  auto settingIt = m_settings.find(settingName);
  if (settingIt != m_settings.end() && (*settingIt).second->GetType() == SettingType::Integer)
  {
    std::shared_ptr<const CSettingInt> setting = std::dynamic_pointer_cast<const CSettingInt>((*settingIt).second);
    if (setting)
      return setting->GetValue();
  }

  CLog::LogF(LOGERROR, "PVR setting '%s' not found or wrong type given", settingName.c_str());
  return -1;
}

std::string CPVRSettings::GetStringValue(const std::string &settingName) const
{
  CSingleLock lock(m_critSection);
  auto settingIt = m_settings.find(settingName);
  if (settingIt != m_settings.end() && (*settingIt).second->GetType() == SettingType::String)
  {
    std::shared_ptr<const CSettingString> setting = std::dynamic_pointer_cast<const CSettingString>((*settingIt).second);
    if (setting)
      return setting->GetValue();
  }

  CLog::LogF(LOGERROR, "PVR setting '%s' not found or wrong type given", settingName.c_str());
  return "";
}

void CPVRSettings::MarginTimeFiller(
  SettingConstPtr  /*setting*/, std::vector<IntegerSettingOption> &list, int &current, void * /*data*/)
{
  list.clear();

  static const int marginTimeValues[] =
  {
    0, 1, 3, 5, 10, 15, 20, 30, 60, 90, 120, 180 // minutes
  };

  for (int iValue : marginTimeValues)
  {
    list.push_back(
      IntegerSettingOption(StringUtils::Format(g_localizeStrings.Get(14044).c_str(), iValue) /* %i min */, iValue));
  }
}

bool CPVRSettings::IsSettingVisible(const std::string &condition, const std::string &value, std::shared_ptr<const CSetting> setting, void *data)
{
  if (setting == nullptr)
    return false;

  const std::string &settingId = setting->GetId();

  if (settingId == CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS)
  {
    // Setting is only visible if exactly one PVR client is enabeld.
    return CServiceBroker::GetPVRManager().Clients()->EnabledClientAmount() == 1;
  }
  else if (settingId == CSettings::SETTING_PVRMANAGER_CLIENTPRIORITIES)
  {
    // Setting is only visible if more than one PVR client is enabeld.
    return CServiceBroker::GetPVRManager().Clients()->EnabledClientAmount() > 1;
  }
  else
  {
    // Show all other settings unconditionally.
    return true;
  }
}

bool CPVRSettings::CheckParentalPin(const std::string &condition, const std::string &value, std::shared_ptr<const CSetting> setting, void *data)
{
  return CServiceBroker::GetPVRManager().GUIActions()->CheckParentalPIN() == ParentalCheckResult::SUCCESS;
}

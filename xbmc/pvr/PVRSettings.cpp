/*
 *      Copyright (C) 2015 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PVRSettings.h"

#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "settings/lib/SettingsManager.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace PVR;

CPVRSettings::CPVRSettings(const std::set<std::string> &settingNames)
{
  Init(settingNames);
  CServiceBroker::GetSettings().RegisterCallback(this, settingNames);
}

CPVRSettings::~CPVRSettings()
{
  CServiceBroker::GetSettings().UnregisterCallback(this);
}

void CPVRSettings::Init(const std::set<std::string> &settingNames)
{
  for (auto settingName : settingNames)
  {
    SettingPtr setting = CServiceBroker::GetSettings().GetSetting(settingName);
    if (!setting)
    {
      CLog::Log(LOGERROR, "CPVRSettings - %s - Unknown setting '%s'", __FUNCTION__, settingName.c_str());
      continue;
    }

    CSingleLock lock(m_critSection);
    m_settings.insert(std::make_pair(settingName, setting->Clone(settingName)));
  }
}

void CPVRSettings::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting == nullptr)
    return;

  CSingleLock lock(m_critSection);
  m_settings[setting->GetId()] = SettingPtr(setting->Clone(setting->GetId()));
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

  CLog::Log(LOGERROR, "CPVRSettings - %s - setting '%s' not found or wrong type given", __FUNCTION__, settingName.c_str());
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

  CLog::Log(LOGERROR, "CPVRSettings - %s - setting '%s' not found or wrong type given", __FUNCTION__, settingName.c_str());
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

  CLog::Log(LOGERROR, "CPVRSettings - %s - setting '%s' not found or wrong type given", __FUNCTION__, settingName.c_str());
  return "";
}

void CPVRSettings::MarginTimeFiller(
  SettingConstPtr  /*setting*/, std::vector< std::pair<std::string, int> > &list, int &current, void * /*data*/)
{
  list.clear();

  static const int marginTimeValues[] =
  {
    0, 1, 3, 5, 10, 15, 20, 30, 60, 90, 120, 180 // minutes
  };
  static const size_t marginTimeValuesCount = sizeof(marginTimeValues) / sizeof(int);

  for (size_t i = 0; i < marginTimeValuesCount; ++i)
  {
    int iValue = marginTimeValues[i];
    list.push_back(
      std::make_pair(StringUtils::Format(g_localizeStrings.Get(14044).c_str(), iValue) /* %i min */, iValue));
  }
}

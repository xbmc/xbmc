/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WeatherManager.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "WeatherJob.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/gui/GUIDialogAddonSettings.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "weather/WeatherProperties.h"

using namespace ADDON;
using namespace KODI::WEATHER;

CWeatherManager::CWeatherManager() : CInfoLoader(WEATHER_REFRESH_INTERVAL_MS)
{
  CServiceBroker::GetSettingsComponent()->GetSettings()->GetSettingsManager()->RegisterCallback(
      this, {CSettings::SETTING_WEATHER_ADDON, CSettings::SETTING_WEATHER_ADDONSETTINGS});
}

CWeatherManager::~CWeatherManager()
{
  const auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return;

  const std::shared_ptr<CSettings> settings = settingsComponent->GetSettings();
  if (!settings)
    return;

  settings->GetSettingsManager()->UnregisterCallback(this);
}

std::string CWeatherManager::GetProperty(const std::string& property) const
{
  // Trigger refresh of data if outdated
  const_cast<CWeatherManager*>(this)->RefreshIfNeeded();

  std::lock_guard lock(m_critSection);

  // Check whether this is a known property and in case return respective value.
  const auto it{m_infoV2.find(property)};
  if (it != m_infoV2.cend())
    return (*it).second;

  if (!IsUpdating()) // window properties are in undefined state while updating.
  {
    CGUIComponent* gui{CServiceBroker::GetGUI()};
    if (gui != nullptr)
    {
      // Fetch the value from respective weather window property and store.
      const CGUIWindow* window{gui->GetWindowManager().GetWindow(WINDOW_WEATHER)};
      if (window != nullptr)
      {
        const std::string val{window->GetProperty(property).asString()};
        m_infoV2.try_emplace(property, val);
        return val;
      }
    }
  }
  return {};
}

std::string CWeatherManager::GetDayProperty(unsigned int index, const std::string& property) const
{
  // Note: No '.' after 'Day'. This is different from 'Daily' and 'Hourly' syntax.
  return GetProperty(StringUtils::Format(DAY_WITH_INDEX_AND_NAME, index, property));
}

std::string CWeatherManager::BusyInfo(int info) const
{
  if (info == WEATHER_IMAGE_CURRENT_ICON)
    return URIUtils::AddFileToFolder(ICON_ADDON_PATH, "na.png");

  return CInfoLoader::BusyInfo(info);
}

std::string CWeatherManager::TranslateInfo(int info) const
{
  std::lock_guard lock(m_critSection);
  switch (info)
  {
    case WEATHER_LABEL_CURRENT_COND:
      return m_info.currentConditions;
    case WEATHER_IMAGE_CURRENT_ICON:
      return m_info.currentIcon;
    case WEATHER_LABEL_CURRENT_TEMP:
      return m_info.currentTemperature;
    case WEATHER_LABEL_CURRENT_FEEL:
      return m_info.currentFeelsLike;
    case WEATHER_LABEL_CURRENT_UVID:
      return m_info.currentUVIndex;
    case WEATHER_LABEL_CURRENT_WIND:
      return m_info.currentWind;
    case WEATHER_LABEL_CURRENT_DEWP:
      return m_info.currentDewPoint;
    case WEATHER_LABEL_CURRENT_HUMI:
      return m_info.currentHumidity;
    case WEATHER_LABEL_LOCATION:
      return m_info.location;
    default:
      return "";
  }
}

std::vector<std::string> CWeatherManager::GetLocations() const
{
  const int locations{std::atoi(GetProperty(GENERAL_LOCATIONS).c_str())};
  std::vector<std::string> ret;
  ret.reserve(locations);
  for (int i = 1; i <= locations; ++i)
    ret.emplace_back(GetLocation(i));

  return ret;
}

std::string CWeatherManager::GetLocation(int iLocation) const
{
  return GetProperty(StringUtils::Format(GENERAL_LOCATION_WITH_INDEX, iLocation));
}

void CWeatherManager::Reset()
{
  std::lock_guard lock(m_critSection);
  m_info = {};
  m_infoV2 = {};
}

bool CWeatherManager::IsFetched()
{
  // Make sure that we actually start up
  RefreshIfNeeded();

  std::lock_guard lock(m_critSection);
  return !m_info.lastUpdateTime.empty();
}

ForecastDay CWeatherManager::GetForecast(int day) const
{
  std::lock_guard lock(m_critSection);
  return m_info.forecast[day];
}

std::string CWeatherManager::GetLastUpdateTime() const
{
  std::lock_guard lock(m_critSection);
  return m_info.lastUpdateTime;
}

void CWeatherManager::SetArea(int iLocation)
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  settings->SetInt(CSettings::SETTING_WEATHER_CURRENTLOCATION, iLocation);
  settings->Save();
}

int CWeatherManager::GetArea()
{
  return CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_WEATHER_CURRENTLOCATION);
}

CJob* CWeatherManager::GetJob() const
{
  return new CWeatherJob(GetArea());
}

void CWeatherManager::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  std::lock_guard lock(m_critSection);
  m_info = static_cast<CWeatherJob*>(job)->GetInfo();
  m_infoV2 = static_cast<CWeatherJob*>(job)->GetInfoV2();

  CInfoLoader::OnJobComplete(jobID, success, job);

  CGUIComponent* gui{CServiceBroker::GetGUI()};
  if (gui)
  {
    // Send a message that we're done.
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WEATHER_FETCHED);
    gui->GetWindowManager().SendThreadMessage(msg);
  }
}

void CWeatherManager::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (!setting)
    return;

  const std::string settingId = setting->GetId();
  if (settingId == CSettings::SETTING_WEATHER_ADDON)
  {
    // clear "WeatherProviderLogo" property that some weather addons set
    CGUIWindow* window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_WEATHER);
    if (window != nullptr)
      window->SetProperty("WeatherProviderLogo", "");
    Refresh();
  }
}

void CWeatherManager::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  if (!setting)
    return;

  const std::string settingId = setting->GetId();
  if (settingId == CSettings::SETTING_WEATHER_ADDONSETTINGS)
  {
    AddonPtr addon;
    if (CServiceBroker::GetAddonMgr().GetAddon(
            CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
                CSettings::SETTING_WEATHER_ADDON),
            addon, AddonType::SCRIPT_WEATHER, OnlyEnabled::CHOICE_YES) &&
        addon)
    {
      //! @todo maybe have ShowAndGetInput return a bool if settings changed, then only reset weather if true.
      CGUIDialogAddonSettings::ShowForAddon(addon);
      Refresh();
    }
  }
}

bool CWeatherManager::CaseInsensitiveCompare::operator()(std::string_view s1,
                                                         std::string_view s2) const
{
  return StringUtils::CompareNoCase(s1, s2) < 0;
}

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
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/gui/GUIDialogAddonSettings.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "weather/WeatherProperties.h"

using namespace ADDON;
using namespace KODI::WEATHER;

CWeatherManager::CWeatherManager(ADDON::CAddonMgr& addonManager)
  : CInfoLoader(WEATHER_REFRESH_INTERVAL_MS),
    m_addonManager(addonManager)
{
  const auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (settingsComponent)
  {
    std::shared_ptr<CSettings> settings = settingsComponent->GetSettings();
    if (settings)
    {
      m_location = settings->GetInt(CSettings::SETTING_WEATHER_CURRENTLOCATION);

      // Register settings callback
      settings->GetSettingsManager()->RegisterCallback(
          this, {CSettings::SETTING_WEATHER_ADDON, CSettings::SETTING_WEATHER_ADDONSETTINGS});

      // At startup, if current weather add-on isn't available, reset the setting
      const std::string addonId = settings->GetString(CSettings::SETTING_WEATHER_ADDON);
      if (!addonId.empty() &&
          (addonManager.IsAddonDisabled(addonId) || !addonManager.IsAddonInstalled(addonId)))
      {
        settings->SetString(CSettings::SETTING_WEATHER_ADDON, "");
        settings->Save();
      }

      // Handle add-on becoming unavailable
      m_addonManager.Events().Subscribe(
          this,
          [s = std::move(settings)](const AddonEvent& event)
          {
            if (typeid(event) == typeid(AddonEvents::Disabled) || // not called on uninstall
                typeid(event) == typeid(AddonEvents::UnInstalled))
            {
              // If add-on was the current weather add-on, reset the setting
              if (event.addonId == s->GetString(CSettings::SETTING_WEATHER_ADDON))
              {
                s->SetString(CSettings::SETTING_WEATHER_ADDON, "");
                s->Save();
              }
            }
          });
    }
  }
}

CWeatherManager::~CWeatherManager()
{
  m_addonManager.Events().Unsubscribe(this);

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

int CWeatherManager::GetLocation() const
{
  std::lock_guard lock(m_critSection);
  return m_location;
}

void CWeatherManager::SetLocation(int location)
{
  if (location == INVALID_LOCATION)
  {
    CLog::Log(LOGERROR, "Invalid location {}.", location);
    return;
  }

  std::lock_guard lock(m_critSection);

  if (m_newLocation != INVALID_LOCATION)
  {
    // Prevent concurrent updates. First request wins, subsequent requests ignored until completion.
    CLog::LogF(LOGWARNING,
               "Ignoring request for location {}. Refresh for location {} already in progress.",
               location, m_newLocation);
  }
  else
  {
    // Remember new requested location, trigger refresh, set m_location once refresh is done.
    CLog::LogF(LOGDEBUG, "Initiating refresh for location {}", location);
    m_newLocation = location;
    Refresh();
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
  m_location = 1;
  m_newLocation = INVALID_LOCATION;
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

CJob* CWeatherManager::GetJob() const
{
  std::lock_guard lock(m_critSection);
  return new CWeatherJob(m_newLocation != INVALID_LOCATION ? m_newLocation : m_location);
}

void CWeatherManager::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  {
    std::lock_guard lock(m_critSection);

    const auto* wJob = static_cast<const CWeatherJob*>(job);
    m_info = wJob->GetInfo();
    m_infoV2 = wJob->GetInfoV2();
    m_location = wJob->GetLocation();
    m_newLocation = INVALID_LOCATION;

    const std::shared_ptr<CSettings> settings{
        CServiceBroker::GetSettingsComponent()->GetSettings()};
    settings->SetInt(CSettings::SETTING_WEATHER_CURRENTLOCATION, m_location);
    settings->Save();
  }

  CInfoLoader::OnJobComplete(jobID, success, job);

  // Send a message that we're done.
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WEATHER_FETCHED);
  CServiceBroker::GetAppMessenger()->SendGUIMessage(msg, WINDOW_INVALID, true);
}

void CWeatherManager::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (!setting)
    return;

  const std::string settingId = setting->GetId();
  if (settingId == CSettings::SETTING_WEATHER_ADDON)
  {
    // Weather add-on to be used has changed. Clear all weather data and refresh.
    Reset();
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
    if (m_addonManager.GetAddon(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
                                    CSettings::SETTING_WEATHER_ADDON),
                                addon, AddonType::SCRIPT_WEATHER, OnlyEnabled::CHOICE_YES) &&
        addon)
    {
      if (CGUIDialogAddonSettings::ShowForAddon(addon))
      {
        // Weather add-on settings have changed. Clear all weather data and refresh.
        Reset();
        Refresh();
      }
    }
  }
}

bool CWeatherManager::CaseInsensitiveCompare::operator()(std::string_view s1,
                                                         std::string_view s2) const
{
  return StringUtils::CompareNoCase(s1, s2) < 0;
}

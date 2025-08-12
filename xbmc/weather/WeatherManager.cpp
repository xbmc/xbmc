/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WeatherManager.h"

#include "LangInfo.h"
#include "ServiceBroker.h"
#include "WeatherJob.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/gui/GUIDialogAddonSettings.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"

using namespace ADDON;

CWeatherManager::CWeatherManager() : CInfoLoader(30 * 60 * 1000) // 30 minutes
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

std::string CWeatherManager::GetProperty(const std::string& property)
{
  const std::string prop{StringUtils::ToLower(property)};
  if (prop == "conditions")
  {
    return GetInfo(WEATHER_LABEL_CURRENT_COND);
  }
  else if (prop == "temperature")
  {
    return StringUtils::Format("{}{}", GetInfo(WEATHER_LABEL_CURRENT_TEMP),
                               g_langInfo.GetTemperatureUnitString());
  }
  else if (prop == "location")
  {
    return GetInfo(WEATHER_LABEL_LOCATION);
  }
  else if (prop == "fanartcode")
  {
    std::string value{URIUtils::GetFileName(GetInfo(WEATHER_IMAGE_CURRENT_ICON))};
    URIUtils::RemoveExtension(value);
    return value;
  }
  else if (prop == "conditionsicon")
  {
    return GetInfo(WEATHER_IMAGE_CURRENT_ICON);
  }

  CGUIComponent* gui{CServiceBroker::GetGUI()};
  if (gui)
  {
    const CGUIWindow* window{gui->GetWindowManager().GetWindow(WINDOW_WEATHER)};
    if (window)
      return window->GetProperty(property).asString();
  }
  return {};
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

std::string CWeatherManager::GetLocation(int iLocation) const
{
  const CGUIWindow* window{CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_WEATHER)};
  if (window)
  {
    const std::string setting{StringUtils::Format("Location{}", iLocation)};
    return window->GetProperty(setting).asString();
  }
  return "";
}

void CWeatherManager::Reset()
{
  std::lock_guard lock(m_critSection);
  m_info = {};
}

bool CWeatherManager::IsFetched()
{
  // call GetInfo() to make sure that we actually start up
  GetInfo(0);

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
  CInfoLoader::OnJobComplete(jobID, success, job);
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

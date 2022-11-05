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

CWeatherManager::CWeatherManager(void) : CInfoLoader(30 * 60 * 1000) // 30 minutes
{
  CServiceBroker::GetSettingsComponent()->GetSettings()->GetSettingsManager()->RegisterCallback(this, {
    CSettings::SETTING_WEATHER_ADDON,
    CSettings::SETTING_WEATHER_ADDONSETTINGS
  });

  Reset();
}

CWeatherManager::~CWeatherManager(void)
{
  const auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return;

  const std::shared_ptr<CSettings> settings = settingsComponent->GetSettings();
  if (!settings)
    return;

  settings->GetSettingsManager()->UnregisterCallback(this);
}

std::string CWeatherManager::BusyInfo(int info) const
{
  if (info == WEATHER_IMAGE_CURRENT_ICON)
    return URIUtils::AddFileToFolder(ICON_ADDON_PATH, "na.png");

  return CInfoLoader::BusyInfo(info);
}

std::string CWeatherManager::TranslateInfo(int info) const
{
  switch (info) {
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

/*!
 \brief Retrieve the city name for the specified location from the settings
 \param iLocation the location index (can be in the range [1..MAXLOCATION])
 \return the city name (without the accompanying region area code)
 */
std::string CWeatherManager::GetLocation(int iLocation)
{
  CGUIWindow* window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_WEATHER);
  if (window)
  {
    std::string setting = StringUtils::Format("Location{}", iLocation);
    return window->GetProperty(setting).asString();
  }
  return "";
}

void CWeatherManager::Reset()
{
  m_info.Reset();
}

bool CWeatherManager::IsFetched()
{
  // call GetInfo() to make sure that we actually start up
  GetInfo(0);
  return !m_info.lastUpdateTime.empty();
}

const ForecastDay &CWeatherManager::GetForecast(int day) const
{
  return m_info.forecast[day];
}

/*!
 \brief Saves the specified location index to the settings. Call Refresh()
        afterwards to update weather info for the new location.
 \param iLocation the new location index (can be in the range [1..MAXLOCATION])
 */
void CWeatherManager::SetArea(int iLocation)
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  settings->SetInt(CSettings::SETTING_WEATHER_CURRENTLOCATION, iLocation);
  settings->Save();
}

/*!
 \brief Retrieves the current location index from the settings
 \return the active location index (will be in the range [1..MAXLOCATION])
 */
int CWeatherManager::GetArea() const
{
  return CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_WEATHER_CURRENTLOCATION);
}

CJob *CWeatherManager::GetJob() const
{
  return new CWeatherJob(GetArea());
}

void CWeatherManager::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  m_info = static_cast<CWeatherJob*>(job)->GetInfo();
  CInfoLoader::OnJobComplete(jobID, success, job);
}

void CWeatherManager::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
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
  if (setting == NULL)
    return;

  const std::string settingId = setting->GetId();
  if (settingId == CSettings::SETTING_WEATHER_ADDONSETTINGS)
  {
    AddonPtr addon;
    if (CServiceBroker::GetAddonMgr().GetAddon(
            CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
                CSettings::SETTING_WEATHER_ADDON),
            addon, AddonType::SCRIPT_WEATHER, OnlyEnabled::CHOICE_YES) &&
        addon != NULL)
    { //! @todo maybe have ShowAndGetInput return a bool if settings changed, then only reset weather if true.
      CGUIDialogAddonSettings::ShowForAddon(addon);
      Refresh();
    }
  }
}


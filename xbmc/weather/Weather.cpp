/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Weather.h"

#include "addons/AddonManager.h"
#include "addons/settings/GUIDialogAddonSettings.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include "utils/Variant.h"
#include "WeatherJob.h"

using namespace ADDON;

CWeather::CWeather(void) : CInfoLoader(30 * 60 * 1000) // 30 minutes
{
  Reset();
}

CWeather::~CWeather(void) = default;

std::string CWeather::BusyInfo(int info) const
{
  if (info == WEATHER_IMAGE_CURRENT_ICON)
    return URIUtils::AddFileToFolder(ICON_ADDON_PATH, "na.png");

  return CInfoLoader::BusyInfo(info);
}

std::string CWeather::TranslateInfo(int info) const
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
std::string CWeather::GetLocation(int iLocation)
{
  CGUIWindow* window = g_windowManager.GetWindow(WINDOW_WEATHER);
  if (window)
  {
    std::string setting = StringUtils::Format("Location%i", iLocation);
    return window->GetProperty(setting).asString();
  }
  return "";
}

void CWeather::Reset()
{
  m_info.Reset();
}

bool CWeather::IsFetched()
{
  // call GetInfo() to make sure that we actually start up
  GetInfo(0);
  return !m_info.lastUpdateTime.empty();
}

const ForecastDay &CWeather::GetForecast(int day) const
{
  return m_info.forecast[day];
}

/*!
 \brief Saves the specified location index to the settings. Call Refresh()
        afterwards to update weather info for the new location.
 \param iLocation the new location index (can be in the range [1..MAXLOCATION])
 */
void CWeather::SetArea(int iLocation)
{
  CServiceBroker::GetSettings().SetInt(CSettings::SETTING_WEATHER_CURRENTLOCATION, iLocation);
  CServiceBroker::GetSettings().Save();
}

/*!
 \brief Retrieves the current location index from the settings
 \return the active location index (will be in the range [1..MAXLOCATION])
 */
int CWeather::GetArea() const
{
  return CServiceBroker::GetSettings().GetInt(CSettings::SETTING_WEATHER_CURRENTLOCATION);
}

CJob *CWeather::GetJob() const
{
  return new CWeatherJob(GetArea());
}

void CWeather::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  m_info = static_cast<CWeatherJob*>(job)->GetInfo();
  CInfoLoader::OnJobComplete(jobID, success, job);
}

void CWeather::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting == NULL)
    return;

  const std::string settingId = setting->GetId();
  if (settingId == CSettings::SETTING_WEATHER_ADDON)
  {
    // clear "WeatherProviderLogo" property that some weather addons set
    CGUIWindow* window = g_windowManager.GetWindow(WINDOW_WEATHER);
    if (window != nullptr)
      window->SetProperty("WeatherProviderLogo", "");
    Refresh();
  }
}

void CWeather::OnSettingAction(std::shared_ptr<const CSetting> setting)
{
  if (setting == NULL)
    return;

  const std::string settingId = setting->GetId();
  if (settingId == CSettings::SETTING_WEATHER_ADDONSETTINGS)
  {
    AddonPtr addon;
    if (CServiceBroker::GetAddonMgr().GetAddon(CServiceBroker::GetSettings().GetString(CSettings::SETTING_WEATHER_ADDON), addon, ADDON_SCRIPT_WEATHER) && addon != NULL)
    { //! @todo maybe have ShowAndGetInput return a bool if settings changed, then only reset weather if true.
      CGUIDialogAddonSettings::ShowForAddon(addon);
      Refresh();
    }
  }
}


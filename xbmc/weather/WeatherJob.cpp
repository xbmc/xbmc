/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WeatherJob.h"

#include "GUIUserMessages.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "XBDateTime.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonType.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "network/Network.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

using namespace ADDON;
using namespace std::chrono_literals;

CWeatherJob::CWeatherJob(int location) : m_location(location)
{
}

bool CWeatherJob::DoWork()
{
  // wait for the network
  if (!CServiceBroker::GetNetwork().IsAvailable())
    return false;

  AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(
          CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
              CSettings::SETTING_WEATHER_ADDON),
          addon, AddonType::SCRIPT_WEATHER, OnlyEnabled::CHOICE_YES))
    return false;

  // initialize our sys.argv variables
  std::vector<std::string> argv;
  argv.emplace_back(addon->LibPath());

  const std::string strSetting{std::to_string(m_location)};
  argv.emplace_back(strSetting);

  // Download our weather
  CLog::Log(LOGINFO, "WEATHER: Downloading weather");

  // call our script, passing the areacode
  int scriptId = -1;
  if ((scriptId = CScriptInvocationManager::GetInstance().ExecuteAsync(argv[0], addon, argv)) >= 0)
  {
    while (true)
    {
      if (!CScriptInvocationManager::GetInstance().IsRunning(scriptId))
        break;

      KODI::TIME::Sleep(100ms);
    }

    CLog::Log(LOGINFO, "WEATHER: Successfully downloaded weather");

    SetFromProperties();

    // and send a message that we're done
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WEATHER_FETCHED);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }
  else
    CLog::Log(LOGERROR, "WEATHER: Weather download failed!");

  return true;
}

const WeatherInfo& CWeatherJob::GetInfo() const
{
  return m_info;
}

namespace
{
std::string ConstructPath(const std::string& in)
{
  if (in.find('/') != std::string::npos || in.find('\\') != std::string::npos)
    return in;

  return URIUtils::AddFileToFolder(ICON_ADDON_PATH, (in.empty() || in == "N/A") ? "na.png" : in);
}

void FormatTemperature(std::string& text, double temp)
{
  const CTemperature temperature{CTemperature::CreateFromCelsius(temp)};
  text = StringUtils::Format("{:.0f}", temperature.To(g_langInfo.GetTemperatureUnit()));
}
} // unnamed namespace

void CWeatherJob::SetFromProperties()
{
  CGUIWindow* window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_WEATHER);
  if (window)
  {
    const CDateTime time{CDateTime::GetCurrentDateTime()};
    m_info.lastUpdateTime = time.GetAsLocalizedDateTime(false, false);
    m_info.currentConditions =
        m_localizer.LocalizeOverview(window->GetProperty("Current.Condition").asString());
    m_info.currentIcon = ConstructPath(window->GetProperty("Current.OutlookIcon").asString());
    FormatTemperature(
        m_info.currentTemperature,
        std::strtod(window->GetProperty("Current.Temperature").asString().c_str(), nullptr));
    FormatTemperature(
        m_info.currentFeelsLike,
        std::strtod(window->GetProperty("Current.FeelsLike").asString().c_str(), nullptr));
    m_info.currentUVIndex =
        m_localizer.LocalizeOverview(window->GetProperty("Current.UVIndex").asString());
    const CSpeed speed{CSpeed::CreateFromKilometresPerHour(
        std::strtod(window->GetProperty("Current.Wind").asString().c_str(), nullptr))};
    std::string direction = window->GetProperty("Current.WindDirection").asString();
    if (direction == "CALM")
      m_info.currentWind = g_localizeStrings.Get(1410);
    else
    {
      direction = m_localizer.LocalizeOverviewToken(direction);
      m_info.currentWind = StringUtils::Format(
          g_localizeStrings.Get(434), direction,
          static_cast<int>(speed.To(g_langInfo.GetSpeedUnit())), g_langInfo.GetSpeedUnitString());
    }
    const std::string windspeed{
        StringUtils::Format("{} {}", static_cast<int>(speed.To(g_langInfo.GetSpeedUnit())),
                            g_langInfo.GetSpeedUnitString())};
    window->SetProperty("Current.WindSpeed", windspeed);
    FormatTemperature(
        m_info.currentDewPoint,
        std::strtod(window->GetProperty("Current.DewPoint").asString().c_str(), nullptr));
    if (window->GetProperty("Current.Humidity").asString().empty())
      m_info.currentHumidity.clear();
    else
      m_info.currentHumidity =
          StringUtils::Format("{}%", window->GetProperty("Current.Humidity").asString());
    m_info.location = window->GetProperty("Current.Location").asString();
    for (unsigned int i = 0; i < WeatherInfo::NUM_DAYS; ++i)
    {
      std::string strDay = StringUtils::Format("Day{}.Title", i);
      m_info.forecast[i].m_day =
          m_localizer.LocalizeOverviewToken(window->GetProperty(strDay).asString());
      strDay = StringUtils::Format("Day{}.HighTemp", i);
      FormatTemperature(m_info.forecast[i].m_high,
                        std::strtod(window->GetProperty(strDay).asString().c_str(), nullptr));
      strDay = StringUtils::Format("Day{}.LowTemp", i);
      FormatTemperature(m_info.forecast[i].m_low,
                        std::strtod(window->GetProperty(strDay).asString().c_str(), nullptr));
      strDay = StringUtils::Format("Day{}.OutlookIcon", i);
      m_info.forecast[i].m_icon = ConstructPath(window->GetProperty(strDay).asString());
      strDay = StringUtils::Format("Day{}.Outlook", i);
      m_info.forecast[i].m_overview =
          m_localizer.LocalizeOverview(window->GetProperty(strDay).asString());
    }
  }
}

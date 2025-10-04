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
#include "messaging/ApplicationMessenger.h"
#include "network/Network.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"
#include "weather/WeatherProperties.h"
#include "weather/WeatherPropertyHelper.h"

using namespace ADDON;
using namespace KODI::WEATHER;
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
  argv.emplace_back(std::to_string(m_location));

  // Clear all add-on supplied window properties.
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WEATHER_RESET);
  CServiceBroker::GetAppMessenger()->SendGUIMessage(msg, WINDOW_INVALID, true);

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
    SetFromPropertiesV2();

    const CDateTime time{CDateTime::GetCurrentDateTime()};
    m_info.lastUpdateTime = time.GetAsLocalizedDateTime(false, false);
  }
  else
    CLog::Log(LOGERROR, "WEATHER: Weather download failed!");

  return true;
}

int CWeatherJob::GetLocation() const
{
  return m_location;
}

const WeatherInfo& CWeatherJob::GetInfo() const
{
  return m_info;
}

const CWeatherManager::WeatherInfoV2& CWeatherJob::GetInfoV2() const
{
  return m_infoV2;
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

class CWeatherPropertyWriter
{
public:
  CWeatherPropertyWriter(const CGUIWindow& window,
                         CWeatherTokenLocalizer& localizer,
                         CWeatherManager::WeatherInfoV2& info)
    : m_propReader(window, localizer),
      m_propSink(info)
  {
  }

  CWeatherPropertyHelper::Property Write(const CWeatherPropertyHelper::PropertyName& propName,
                                         const CWeatherPropertyHelper::PropertyValue& propValue)
  {
    m_propSink.try_emplace(propName, propValue);
    return {propName, propValue};
  }

  CWeatherPropertyHelper::Property Write(const CWeatherPropertyHelper::PropertyName& propName)
  {
    const auto& [name, value] = m_propReader.GetProperty(propName);
    return Write(name, value);
  }

  CWeatherPropertyHelper::Property WriteDay(int index,
                                            const CWeatherPropertyHelper::PropertyName& propName)
  {
    const auto& [name, value] = m_propReader.GetDayProperty(index, propName);
    return Write(name, value);
  }

  CWeatherPropertyHelper::Property WriteDaily(int index,
                                              const CWeatherPropertyHelper::PropertyName& propName)
  {
    const auto& [name, value] = m_propReader.GetDailyProperty(index, propName);
    return Write(name, value);
  }

  CWeatherPropertyHelper::Property WriteHourly(int index,
                                               const CWeatherPropertyHelper::PropertyName& propName)
  {
    const auto& [name, value] = m_propReader.GetHourlyProperty(index, propName);
    return Write(name, value);
  }

  const CWeatherPropertyHelper& GetPropertyReader() const { return m_propReader; }

private:
  CWeatherPropertyHelper m_propReader;
  CWeatherManager::WeatherInfoV2& m_propSink;
};

} // unnamed namespace

void CWeatherJob::SetFromProperties()
{
  CGUIWindow* window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_WEATHER);
  if (window)
  {
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
    const CVariant wind{window->GetProperty("Current.Wind")};
    if (!wind.isNull())
    {
      // Special casing: Combine window props Current.Wind and Current.WindDirection values.
      const CSpeed speed{
          CSpeed::CreateFromKilometresPerHour(std::strtod(wind.asString().c_str(), nullptr))};
      std::string direction{window->GetProperty("Current.WindDirection").asString()};
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
    }
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

void CWeatherJob::SetFromPropertiesV2()
{
  CGUIComponent* gui{CServiceBroker::GetGUI()};
  if (gui == nullptr)
    return;

  CGUIWindow* window{gui->GetWindowManager().GetWindow(WINDOW_WEATHER)};
  if (window == nullptr)
    return;

  CWeatherPropertyWriter props{*window, m_localizer, m_infoV2};

  // Data for current conditions

  props.Write(CURRENT_LOCATION);
  props.Write(CURRENT_CONDITION);
  props.Write(CURRENT_TEMPERATURE);
  props.Write(CURRENT_FEELS_LIKE);
  props.Write(CURRENT_DEW_POINT);
  props.Write(CURRENT_HUMIDITY);
  props.Write(CURRENT_PRECIPITATION);
  props.Write(CURRENT_CLOUDINESS);
  props.Write(CURRENT_UV_INDEX);

  {
    const auto& [iconProp, iconValue] = props.Write(CURRENT_OUTLOOK_ICON);
    props.Write(CURRENT_CONDITION_ICON, iconValue); // See spec.

    // If not provided by the add-on, create fanart code from outlook icon file name.
    const auto& [fanartCodeProp, fanartCodeValue] =
        props.GetPropertyReader().GetProperty(CURRENT_FANART_CODE);
    props.Write(fanartCodeProp,
                CWeatherPropertyHelper::FormatFanartCode(fanartCodeValue, iconValue));
  }

  {
    const auto& [directionProp, directionValue] = props.Write(CURRENT_WIND_DIRECTION);

    if (props.GetPropertyReader().HasProperty(CURRENT_WIND))
    {
      // Special casing: Combine window props Current.Wind and Current.WindDirection values, store
      // the result in Current.Wind, store original value of Current.Wind in Current.WindSpeed.
      // See spec.
      const auto& [windProp, speedValue] = props.GetPropertyReader().GetProperty(CURRENT_WIND);
      const CSpeed speed{
          CSpeed::CreateFromKilometresPerHour(std::strtod(speedValue.c_str(), nullptr))};
      props.Write(windProp, CWeatherPropertyHelper::FormatWind(directionValue, speed));
      props.Write(CURRENT_WIND_SPEED, CWeatherPropertyHelper::FormatWind("", speed));
    }
  }

  // Data for a week

  for (unsigned int i = 0; i < WeatherInfo::NUM_DAYS; ++i)
  {
    props.WriteDay(i, DAY_TITLE);
    props.WriteDay(i, DAY_OUTLOOK);
    props.WriteDay(i, DAY_HIGH_TEMP);
    props.WriteDay(i, DAY_LOW_TEMP);

    {
      const auto& [iconProp, iconValue] = props.WriteDay(i, DAY_OUTLOOK_ICON);

      // If not provided by the add-on, create fanart code from outlook icon file name.
      const auto& [fanartCodeProp, fanartCodeValue] =
          props.GetPropertyReader().GetDayProperty(i, DAY_FANART_CODE);
      props.Write(fanartCodeProp,
                  CWeatherPropertyHelper::FormatFanartCode(fanartCodeValue, iconValue));
    }
  }

  // Hourly data

  int idx{1};
  while (idx <= MAX_HOURS_TO_FETCH)
  {
    if (!props.GetPropertyReader().HasHourlyProperties(idx))
      break; // done, no more data

    props.WriteHourly(idx, HOURLY_TIME);
    props.WriteHourly(idx, HOURLY_LONG_DATE);
    props.WriteHourly(idx, HOURLY_SHORT_DATE);
    props.WriteHourly(idx, HOURLY_OUTLOOK);
    props.WriteHourly(idx, HOURLY_TEMPERATURE);
    props.WriteHourly(idx, HOURLY_DEW_POINT);
    props.WriteHourly(idx, HOURLY_FEELS_LIKE);
    props.WriteHourly(idx, HOURLY_HUMIDITY);
    props.WriteHourly(idx, HOURLY_PRECIPITATION);
    props.WriteHourly(idx, HOURLY_PRESSURE);
    props.WriteHourly(idx, HOURLY_WIND_SPEED);
    props.WriteHourly(idx, HOURLY_WIND_DIRECTION);

    {
      const auto& [iconProp, iconValue] = props.WriteHourly(idx, HOURLY_OUTLOOK_ICON);

      // If not provided by the add-on, create fanart code from outlook icon file name.
      const auto& [fanartCodeProp, fanartCodeValue] =
          props.GetPropertyReader().GetHourlyProperty(idx, HOURLY_FANART_CODE);
      props.Write(fanartCodeProp,
                  CWeatherPropertyHelper::FormatFanartCode(fanartCodeValue, iconValue));
    }

    idx++;
  }

  // Daily data

  idx = 1;
  while (idx <= MAX_DAYS_TO_FETCH)
  {
    if (!props.GetPropertyReader().HasDailyProperties(idx))
      break; // done, no more data

    props.WriteDaily(idx, DAILY_SHORT_DATE);
    props.WriteDaily(idx, DAILY_SHORT_DAY);
    props.WriteDaily(idx, DAILY_OUTLOOK);
    props.WriteDaily(idx, DAILY_HIGH_TEMPERATURE);
    props.WriteDaily(idx, DAILY_LOW_TEMPERATURE);
    props.WriteDaily(idx, DAILY_PRECIPITATION);
    props.WriteDaily(idx, DAILY_WIND_SPEED);
    props.WriteDaily(idx, DAILY_WIND_DIRECTION);

    {
      const auto& [iconProp, iconValue] = props.WriteDaily(idx, DAILY_OUTLOOK_ICON);

      // If not provided by the add-on, create fanart code from outlook icon file name.
      const auto& [fanartCodeProp, fanartCodeValue] =
          props.GetPropertyReader().GetDailyProperty(idx, DAILY_FANART_CODE);
      props.Write(fanartCodeProp,
                  CWeatherPropertyHelper::FormatFanartCode(fanartCodeValue, iconValue));
    }

    idx++;
  }

  // Locations

  const auto& [prop, value] = props.Write(GENERAL_LOCATIONS);

  const int locations{std::atoi(value.c_str())};
  for (int i = 1; i <= locations; ++i)
  {
    props.Write(StringUtils::Format(GENERAL_LOCATION_WITH_INDEX, i));
  }
}

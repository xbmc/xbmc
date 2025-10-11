/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowWeather.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "guilib/WindowIDs.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "weather/WeatherManager.h"
#include "weather/WeatherProperties.h"

#include <utility>

using namespace KODI::WEATHER;

namespace
{
constexpr unsigned int CONTROL_BTNREFRESH = 2;
constexpr unsigned int CONTROL_SELECTLOCATION = 3;
constexpr unsigned int CONTROL_LABELUPDATED = 11;

constexpr unsigned int CONTROL_STATICTEMP = 223;
constexpr unsigned int CONTROL_STATICFEEL = 224;
constexpr unsigned int CONTROL_STATICUVID = 225;
constexpr unsigned int CONTROL_STATICWIND = 226;
constexpr unsigned int CONTROL_STATICDEWP = 227;
constexpr unsigned int CONTROL_STATICHUMI = 228;

constexpr unsigned int CONTROL_LABELD0DAY = 31;
constexpr unsigned int CONTROL_LABELD0HI = 32;
constexpr unsigned int CONTROL_LABELD0LOW = 33;
constexpr unsigned int CONTROL_LABELD0GEN = 34;
constexpr unsigned int CONTROL_IMAGED0IMG = 35;

} // unnamed namespace

CGUIWindowWeather::CGUIWindowWeather()
  : CGUIWindow(WINDOW_WEATHER, "MyWeather.xml"),
    m_windowProps(GetPropertyNames())
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIWindowWeather::~CGUIWindowWeather() = default;

bool CGUIWindowWeather::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      const int iControl{message.GetSenderId()};
      if (iControl == CONTROL_BTNREFRESH)
      {
        CServiceBroker::GetWeatherManager().Refresh(); // Refresh clicked so do a complete update
      }
      else if (iControl == CONTROL_SELECTLOCATION)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_SELECTLOCATION);
        OnMessage(msg);

        SetLocation(msg.GetParam1());
      }
      break;
    }
    case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_WINDOW_RESET)
      {
        CServiceBroker::GetWeatherManager().Refresh(); // Do a complete update
        return true;
      }
      else if (message.GetParam1() == GUI_MSG_WEATHER_RESET)
      {
        ClearProps();
        return true;
      }
      else if (message.GetParam1() == GUI_MSG_WEATHER_FETCHED)
      {
        UpdateLocations();
        SetProps();
        return true;
      }
      break;
    }
    case GUI_MSG_ITEM_SELECT:
    {
      if (message.GetSenderId() == 0) //handle only message from builtin
      {
        SetLocation(message.GetParam1());
        return true;
      }
      break;
    }
    case GUI_MSG_MOVE_OFFSET:
    {
      if (message.GetSenderId() == 0 && m_maxLocation > 0) //handle only message from builtin
      {
        // Clamp location between 1 and m_maxLocation
        const CWeatherManager& wmgr{CServiceBroker::GetWeatherManager()};
        int v = (wmgr.GetLocation() + message.GetParam1() - 1) % m_maxLocation + 1;
        if (v < 1)
          v += m_maxLocation;
        SetLocation(v);
        return true;
      }
      break;
    }
    default:
      break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowWeather::OnInitWindow()
{
  // call UpdateButtons() so that we start with our initial stuff already present
  UpdateButtons();
  UpdateLocations();
  CGUIWindow::OnInitWindow();
}

void CGUIWindowWeather::UpdateLocations()
{
  if (!IsActive())
    return;

  CWeatherManager& wmgr{CServiceBroker::GetWeatherManager()};

  const std::vector<std::string> locations{wmgr.GetLocations()};
  if (locations.empty())
    return;

  m_maxLocation = static_cast<unsigned int>(locations.size());

  unsigned int iCurWeather{static_cast<unsigned int>(wmgr.GetLocation())};
  if (iCurWeather > m_maxLocation)
  {
    iCurWeather = m_maxLocation;
    wmgr.SetLocation(iCurWeather);
    return;
  }

  std::vector<std::pair<std::string, int>> labels;
  labels.reserve(m_maxLocation);
  for (unsigned int i = 1; i <= m_maxLocation; ++i)
  {
    std::string strLabel{locations.at(i - 1)};
    if (strLabel.size() > 1) //got the location string yet?
    {
      const size_t iPos{strLabel.rfind(", ")};
      if (iPos != std::string::npos)
      {
        std::string strLabel2(strLabel);
        strLabel = strLabel2.substr(0, iPos);
      }
    }
    else
    {
      strLabel = StringUtils::Format("AreaCode {}", i);
    }
    labels.emplace_back(strLabel, i);

    // in case it's a button, set the label
    if (i == iCurWeather)
      SET_CONTROL_LABEL(CONTROL_SELECTLOCATION, strLabel);
  }

  SET_CONTROL_LABELS(CONTROL_SELECTLOCATION, iCurWeather, &labels);
}

void CGUIWindowWeather::UpdateButtons()
{
  CONTROL_ENABLE(CONTROL_BTNREFRESH);

  SET_CONTROL_LABEL(CONTROL_BTNREFRESH, 184); //Refresh

  const CWeatherManager& wmgr{CServiceBroker::GetWeatherManager()};

  SET_CONTROL_LABEL(WEATHER_LABEL_LOCATION, wmgr.GetLocation(wmgr.GetLocation()));
  SET_CONTROL_LABEL(CONTROL_LABELUPDATED, wmgr.GetLastUpdateTime());

  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_COND, wmgr.GetProperty(CURRENT_CONDITION));
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_TEMP, wmgr.GetProperty(CURRENT_TEMPERATURE));
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_FEEL, wmgr.GetProperty(CURRENT_FEELS_LIKE));
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_UVID, wmgr.GetProperty(CURRENT_UV_INDEX));
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_WIND, wmgr.GetProperty(CURRENT_WIND));
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_DEWP, wmgr.GetProperty(CURRENT_DEW_POINT));
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_HUMI, wmgr.GetProperty(CURRENT_HUMIDITY));
  SET_CONTROL_FILENAME(WEATHER_IMAGE_CURRENT_ICON, wmgr.GetProperty(CURRENT_OUTLOOK_ICON));

  //static labels
  SET_CONTROL_LABEL(CONTROL_STATICTEMP, 401); //Temperature
  SET_CONTROL_LABEL(CONTROL_STATICFEEL, 402); //Feels Like
  SET_CONTROL_LABEL(CONTROL_STATICUVID, 403); //UV Index
  SET_CONTROL_LABEL(CONTROL_STATICWIND, 404); //Wind
  SET_CONTROL_LABEL(CONTROL_STATICDEWP, 405); //Dew Point
  SET_CONTROL_LABEL(CONTROL_STATICHUMI, 406); //Humidity

  for (unsigned int i = 0; i < WeatherInfo::NUM_DAYS; ++i)
  {
    SET_CONTROL_LABEL(CONTROL_LABELD0DAY + (i * 10), wmgr.GetDayProperty(i, DAY_TITLE));
    SET_CONTROL_LABEL(CONTROL_LABELD0HI + (i * 10), wmgr.GetDayProperty(i, DAY_HIGH_TEMP));
    SET_CONTROL_LABEL(CONTROL_LABELD0LOW + (i * 10), wmgr.GetDayProperty(i, DAY_LOW_TEMP));
    SET_CONTROL_LABEL(CONTROL_LABELD0GEN + (i * 10), wmgr.GetDayProperty(i, DAY_OUTLOOK));
    SET_CONTROL_FILENAME(CONTROL_IMAGED0IMG + (i * 10), wmgr.GetDayProperty(i, DAY_OUTLOOK_ICON));
  }
}

void CGUIWindowWeather::FrameMove()
{
  // update our controls
  UpdateButtons();

  CGUIWindow::FrameMove();
}

/*!
 \brief Sets the location to the specified index and refreshes the weather
 \param loc the location index (can be any value except WeatherManager::INVALID_LOCATION)
 */
void CGUIWindowWeather::SetLocation(int loc)
{
  if (loc < 1 || loc > static_cast<int>(m_maxLocation))
    return;

  CServiceBroker::GetWeatherManager().SetLocation(loc);
}

void CGUIWindowWeather::SetProps()
{
  CWeatherManager& wmgr{CServiceBroker::GetWeatherManager()};

  // Current weather
  const int iCurWeather{wmgr.GetLocation()};
  SetProperty("Location", wmgr.GetLocation(iCurWeather));
  SetProperty("LocationIndex", iCurWeather);
  SetProperty("Updated", wmgr.GetLastUpdateTime());
  SetProperty("Current.ConditionIcon", wmgr.GetInfo(WEATHER_IMAGE_CURRENT_ICON));
  SetProperty("Current.Condition", wmgr.GetInfo(WEATHER_LABEL_CURRENT_COND));
  SetProperty("Current.Temperature", wmgr.GetInfo(WEATHER_LABEL_CURRENT_TEMP));
  SetProperty("Current.FeelsLike", wmgr.GetInfo(WEATHER_LABEL_CURRENT_FEEL));
  SetProperty("Current.UVIndex", wmgr.GetInfo(WEATHER_LABEL_CURRENT_UVID));
  SetProperty("Current.Wind", wmgr.GetInfo(WEATHER_LABEL_CURRENT_WIND));
  SetProperty("Current.DewPoint", wmgr.GetInfo(WEATHER_LABEL_CURRENT_DEWP));
  SetProperty("Current.Humidity", wmgr.GetInfo(WEATHER_LABEL_CURRENT_HUMI));
  // we use the icons code number for fanart as it's the safest way
  std::string fanartcode{URIUtils::GetFileName(wmgr.GetInfo(WEATHER_IMAGE_CURRENT_ICON))};
  URIUtils::RemoveExtension(fanartcode);
  SetProperty("Current.FanartCode", fanartcode);

  // Future weather
  std::string day;
  for (unsigned int i = 0; i < WeatherInfo::NUM_DAYS; ++i)
  {
    day = StringUtils::Format("Day{}.", i);
    SetProperty(day + "Title", wmgr.GetForecast(i).m_day);
    SetProperty(day + "HighTemp", wmgr.GetForecast(i).m_high);
    SetProperty(day + "LowTemp", wmgr.GetForecast(i).m_low);
    SetProperty(day + "Outlook", wmgr.GetForecast(i).m_overview);
    SetProperty(day + "OutlookIcon", wmgr.GetForecast(i).m_icon);
    fanartcode = URIUtils::GetFileName(wmgr.GetForecast(i).m_icon);
    URIUtils::RemoveExtension(fanartcode);
    SetProperty(day + "FanartCode", fanartcode);
  }
}

void CGUIWindowWeather::ClearProps()
{
  // Erase all add-on supplied window properties, keep all others.

  std::vector<std::pair<std::string, CVariant>> winProps;
  winProps.reserve(m_windowProps.size());

  for (const std::string& prop : m_windowProps)
  {
    winProps.emplace_back(prop, GetProperty(prop));
  }

  ClearProperties();

  for (const auto& [name, value] : winProps)
  {
    if (!value.isNull())
      SetProperty(name, value);
  }
}

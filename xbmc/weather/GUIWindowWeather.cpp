/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowWeather.h"

#include "GUIUserMessages.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "guilib/WindowIDs.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "weather/WeatherManager.h"

#include <utility>

using namespace ADDON;

#define CONTROL_BTNREFRESH             2
#define CONTROL_SELECTLOCATION         3
#define CONTROL_LABELUPDATED          11

#define CONTROL_STATICTEMP           223
#define CONTROL_STATICFEEL           224
#define CONTROL_STATICUVID           225
#define CONTROL_STATICWIND           226
#define CONTROL_STATICDEWP           227
#define CONTROL_STATICHUMI           228

#define CONTROL_LABELD0DAY            31
#define CONTROL_LABELD0HI             32
#define CONTROL_LABELD0LOW            33
#define CONTROL_LABELD0GEN            34
#define CONTROL_IMAGED0IMG            35

#define LOCALIZED_TOKEN_FIRSTID      370
#define LOCALIZED_TOKEN_LASTID       395

/*
FIXME'S
>strings are not centered
*/

CGUIWindowWeather::CGUIWindowWeather(void)
    : CGUIWindow(WINDOW_WEATHER, "MyWeather.xml")
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIWindowWeather::~CGUIWindowWeather(void) = default;

bool CGUIWindowWeather::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNREFRESH)
      {
        CServiceBroker::GetWeatherManager().Refresh(); // Refresh clicked so do a complete update
      }
      else if (iControl == CONTROL_SELECTLOCATION)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_SELECTLOCATION);
        OnMessage(msg);

        SetLocation(msg.GetParam1());
      }
    }
    break;
  case GUI_MSG_NOTIFY_ALL:
    if (message.GetParam1() == GUI_MSG_WINDOW_RESET)
    {
      CServiceBroker::GetWeatherManager().Reset();
      return true;
    }
    else if (message.GetParam1() == GUI_MSG_WEATHER_FETCHED)
    {
      UpdateLocations();
      SetProperties();
    }
    break;
  case GUI_MSG_ITEM_SELECT:
    {
      if (message.GetSenderId() == 0) //handle only message from builtin
      {
        SetLocation(message.GetParam1());
        return true;
      }
    }
    break;
  case GUI_MSG_MOVE_OFFSET:
    {
      if (message.GetSenderId() == 0 && m_maxLocation > 0) //handle only message from builtin
      {
        // Clamp location between 1 and m_maxLocation
        int v = (CServiceBroker::GetWeatherManager().GetArea() + message.GetParam1() - 1) % m_maxLocation + 1;
        if (v < 1) v += m_maxLocation;
        SetLocation(v);
        return true;
      }
    }
    break;
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
  if (!IsActive()) return;
  m_maxLocation = strtol(GetProperty("Locations").asString().c_str(),0,10);
  if (m_maxLocation < 1) return;

  std::vector< std::pair<std::string, int> > labels;

  unsigned int iCurWeather = CServiceBroker::GetWeatherManager().GetArea();

  if (iCurWeather > m_maxLocation)
  {
    CServiceBroker::GetWeatherManager().SetArea(m_maxLocation);
    iCurWeather = m_maxLocation;
    ClearProperties();
    CServiceBroker::GetWeatherManager().Refresh();
  }

  for (unsigned int i = 1; i <= m_maxLocation; i++)
  {
    std::string strLabel = CServiceBroker::GetWeatherManager().GetLocation(i);
    if (strLabel.size() > 1) //got the location string yet?
    {
      size_t iPos = strLabel.rfind(", ");
      if (iPos != std::string::npos)
      {
        std::string strLabel2(strLabel);
        strLabel = strLabel2.substr(0,iPos);
      }
      labels.emplace_back(strLabel, i);
    }
    else
    {
      strLabel = StringUtils::Format("AreaCode {}", i);
      labels.emplace_back(strLabel, i);
    }
    // in case it's a button, set the label
    if (i == iCurWeather)
      SET_CONTROL_LABEL(CONTROL_SELECTLOCATION,strLabel);
  }

  SET_CONTROL_LABELS(CONTROL_SELECTLOCATION, iCurWeather, &labels);
}

void CGUIWindowWeather::UpdateButtons()
{
  CONTROL_ENABLE(CONTROL_BTNREFRESH);

  SET_CONTROL_LABEL(CONTROL_BTNREFRESH, 184);   //Refresh

  SET_CONTROL_LABEL(WEATHER_LABEL_LOCATION, CServiceBroker::GetWeatherManager().GetLocation(CServiceBroker::GetWeatherManager().GetArea()));
  SET_CONTROL_LABEL(CONTROL_LABELUPDATED, CServiceBroker::GetWeatherManager().GetLastUpdateTime());

  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_COND, CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_COND));
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_TEMP, CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_TEMP) + g_langInfo.GetTemperatureUnitString());
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_FEEL, CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_FEEL) + g_langInfo.GetTemperatureUnitString());
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_UVID, CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_UVID));
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_WIND, CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_WIND));
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_DEWP, CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_DEWP) + g_langInfo.GetTemperatureUnitString());
  SET_CONTROL_LABEL(WEATHER_LABEL_CURRENT_HUMI, CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_HUMI));
  SET_CONTROL_FILENAME(WEATHER_IMAGE_CURRENT_ICON, CServiceBroker::GetWeatherManager().GetInfo(WEATHER_IMAGE_CURRENT_ICON));

  //static labels
  SET_CONTROL_LABEL(CONTROL_STATICTEMP, 401);  //Temperature
  SET_CONTROL_LABEL(CONTROL_STATICFEEL, 402);  //Feels Like
  SET_CONTROL_LABEL(CONTROL_STATICUVID, 403);  //UV Index
  SET_CONTROL_LABEL(CONTROL_STATICWIND, 404);  //Wind
  SET_CONTROL_LABEL(CONTROL_STATICDEWP, 405);  //Dew Point
  SET_CONTROL_LABEL(CONTROL_STATICHUMI, 406);  //Humidity

  for (int i = 0; i < NUM_DAYS; i++)
  {
    SET_CONTROL_LABEL(CONTROL_LABELD0DAY + (i*10), CServiceBroker::GetWeatherManager().GetForecast(i).m_day);
    SET_CONTROL_LABEL(CONTROL_LABELD0HI + (i*10), CServiceBroker::GetWeatherManager().GetForecast(i).m_high + g_langInfo.GetTemperatureUnitString());
    SET_CONTROL_LABEL(CONTROL_LABELD0LOW + (i*10), CServiceBroker::GetWeatherManager().GetForecast(i).m_low + g_langInfo.GetTemperatureUnitString());
    SET_CONTROL_LABEL(CONTROL_LABELD0GEN + (i*10), CServiceBroker::GetWeatherManager().GetForecast(i).m_overview);
    SET_CONTROL_FILENAME(CONTROL_IMAGED0IMG + (i*10), CServiceBroker::GetWeatherManager().GetForecast(i).m_icon);
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
 \param loc the location index (in the range [1..MAXLOCATION])
 */
void CGUIWindowWeather::SetLocation(int loc)
{
  if (loc < 1 || loc > (int)m_maxLocation)
    return;
  // Avoid a settings write if old location == new location
  if (CServiceBroker::GetWeatherManager().GetArea() != loc)
  {
    ClearProperties();
    CServiceBroker::GetWeatherManager().SetArea(loc);
    std::string strLabel = CServiceBroker::GetWeatherManager().GetLocation(loc);
    size_t iPos = strLabel.rfind(", ");
    if (iPos != std::string::npos)
      strLabel.resize(iPos);
    SET_CONTROL_LABEL(CONTROL_SELECTLOCATION, strLabel);
  }
  CServiceBroker::GetWeatherManager().Refresh();
}

void CGUIWindowWeather::SetProperties()
{
  // Current weather
  int iCurWeather = CServiceBroker::GetWeatherManager().GetArea();
  SetProperty("Location", CServiceBroker::GetWeatherManager().GetLocation(iCurWeather));
  SetProperty("LocationIndex", iCurWeather);
  SetProperty("Updated", CServiceBroker::GetWeatherManager().GetLastUpdateTime());
  SetProperty("Current.ConditionIcon", CServiceBroker::GetWeatherManager().GetInfo(WEATHER_IMAGE_CURRENT_ICON));
  SetProperty("Current.Condition", CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_COND));
  SetProperty("Current.Temperature", CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_TEMP));
  SetProperty("Current.FeelsLike", CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_FEEL));
  SetProperty("Current.UVIndex", CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_UVID));
  SetProperty("Current.Wind", CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_WIND));
  SetProperty("Current.DewPoint", CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_DEWP));
  SetProperty("Current.Humidity", CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_HUMI));
  // we use the icons code number for fanart as it's the safest way
  std::string fanartcode = URIUtils::GetFileName(CServiceBroker::GetWeatherManager().GetInfo(WEATHER_IMAGE_CURRENT_ICON));
  URIUtils::RemoveExtension(fanartcode);
  SetProperty("Current.FanartCode", fanartcode);

  // Future weather
  std::string day;
  for (int i = 0; i < NUM_DAYS; i++)
  {
    day = StringUtils::Format("Day{}.", i);
    SetProperty(day + "Title", CServiceBroker::GetWeatherManager().GetForecast(i).m_day);
    SetProperty(day + "HighTemp", CServiceBroker::GetWeatherManager().GetForecast(i).m_high);
    SetProperty(day + "LowTemp", CServiceBroker::GetWeatherManager().GetForecast(i).m_low);
    SetProperty(day + "Outlook", CServiceBroker::GetWeatherManager().GetForecast(i).m_overview);
    SetProperty(day + "OutlookIcon", CServiceBroker::GetWeatherManager().GetForecast(i).m_icon);
    fanartcode = URIUtils::GetFileName(CServiceBroker::GetWeatherManager().GetForecast(i).m_icon);
    URIUtils::RemoveExtension(fanartcode);
    SetProperty(day + "FanartCode", fanartcode);
  }
}

void CGUIWindowWeather::ClearProperties()
{
  // Current weather
  SetProperty("Location", "");
  SetProperty("LocationIndex", "");
  SetProperty("Updated", "");
  SetProperty("Current.ConditionIcon", "");
  SetProperty("Current.Condition", "");
  SetProperty("Current.Temperature", "");
  SetProperty("Current.FeelsLike", "");
  SetProperty("Current.UVIndex", "");
  SetProperty("Current.Wind", "");
  SetProperty("Current.DewPoint", "");
  SetProperty("Current.Humidity", "");
  SetProperty("Current.FanartCode", "");

  // Future weather
  std::string day;
  for (int i = 0; i < NUM_DAYS; i++)
  {
    day = StringUtils::Format("Day{}.", i);
    SetProperty(day + "Title", "");
    SetProperty(day + "HighTemp", "");
    SetProperty(day + "LowTemp", "");
    SetProperty(day + "Outlook", "");
    SetProperty(day + "OutlookIcon", "");
    SetProperty(day + "FanartCode", "");
  }
}

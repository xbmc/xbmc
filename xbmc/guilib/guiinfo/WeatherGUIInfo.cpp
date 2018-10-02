/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/WeatherGUIInfo.h"

#include "FileItem.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "weather/WeatherManager.h"

#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"

using namespace KODI::GUILIB::GUIINFO;

bool CWeatherGUIInfo::InitCurrentItem(CFileItem *item)
{
  return false;
}

bool CWeatherGUIInfo::GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // WEATHER_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case WEATHER_CONDITIONS_TEXT:
      value = CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_COND);
      StringUtils::Trim(value);
      return true;
    case WEATHER_CONDITIONS_ICON:
      value = CServiceBroker::GetWeatherManager().GetInfo(WEATHER_IMAGE_CURRENT_ICON);
      return true;
    case WEATHER_TEMPERATURE:
      value = StringUtils::Format("%s%s",
                                  CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_CURRENT_TEMP).c_str(),
                                  g_langInfo.GetTemperatureUnitString().c_str());
      return true;
    case WEATHER_LOCATION:
      value = CServiceBroker::GetWeatherManager().GetInfo(WEATHER_LABEL_LOCATION);
      return true;
    case WEATHER_FANART_CODE:
      value = URIUtils::GetFileName(CServiceBroker::GetWeatherManager().GetInfo(WEATHER_IMAGE_CURRENT_ICON));
      URIUtils::RemoveExtension(value);
      return true;
    case WEATHER_PLUGIN:
      value = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_WEATHER_ADDON);
      return true;
  }

  return false;
}

bool CWeatherGUIInfo::GetInt(int& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  return false;
}

bool CWeatherGUIInfo::GetBool(bool& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // WEATHER_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case WEATHER_IS_FETCHED:
      value = CServiceBroker::GetWeatherManager().IsFetched();
      return true;;
  }

  return false;
}

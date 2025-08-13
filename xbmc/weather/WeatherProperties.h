/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI::WEATHER
{
// Specification: https://kodi.wiki/view/Weather_addons

// Current weather
constexpr const char* CURRENT_CLOUDINESS = "Current.Cloudiness";
constexpr const char* CURRENT_CONDITION = "Current.Condition";
constexpr const char* CURRENT_CONDITION_ICON = "Current.ConditionIcon";
constexpr const char* CURRENT_DEW_POINT = "Current.DewPoint";
constexpr const char* CURRENT_FANART_CODE = "Current.FanartCode";
constexpr const char* CURRENT_FEELS_LIKE = "Current.FeelsLike";
constexpr const char* CURRENT_HUMIDITY = "Current.Humidity";
constexpr const char* CURRENT_LOCATION = "Current.Location";
constexpr const char* CURRENT_OUTLOOK_ICON = "Current.OutlookIcon";
constexpr const char* CURRENT_PRECIPITATION = "Current.Precipitation";
constexpr const char* CURRENT_TEMPERATURE = "Current.Temperature";
constexpr const char* CURRENT_UV_INDEX = "Current.UVIndex";
constexpr const char* CURRENT_WIND = "Current.Wind";
constexpr const char* CURRENT_WIND_DIRECTION = "Current.WindDirection";
constexpr const char* CURRENT_WIND_SPEED = "Current.WindSpeed";

// Daily weather, format #1, weather for up to 7 days
// Format: Day{0..6}.{property-name}
constexpr const char* DAY_WITH_INDEX_AND_NAME = "Day{}.{}";

constexpr const char* DAY_FANART_CODE = "FanartCode";
constexpr const char* DAY_HIGH_TEMP = "HighTemp";
constexpr const char* DAY_LOW_TEMP = "LowTemp";
constexpr const char* DAY_OUTLOOK = "Outlook";
constexpr const char* DAY_OUTLOOK_ICON = "OutlookIcon";
constexpr const char* DAY_TITLE = "Title";

// Daily weather, format #2
// Format: Daily.{1-based-index}.{property-name}
constexpr const char* DAILY_WITH_INDEX_AND_NAME = "Daily.{}.{}";

constexpr const char* DAILY_FANART_CODE = "FanartCode";
constexpr const char* DAILY_HIGH_TEMPERATURE = "HighTemperature";
constexpr const char* DAILY_LOW_TEMPERATURE = "LowTemperature";
constexpr const char* DAILY_OUTLOOK = "Outlook";
constexpr const char* DAILY_OUTLOOK_ICON = "OutlookIcon";
constexpr const char* DAILY_PRECIPITATION = "Precipitation";
constexpr const char* DAILY_SHORT_DATE = "ShortDate";
constexpr const char* DAILY_SHORT_DAY = "ShortDay";
constexpr const char* DAILY_WIND_DIRECTION = "WindDirection";
constexpr const char* DAILY_WIND_SPEED = "WindSpeed";

// Hourly weather
// Format: Hourly.{1-based-index}.{property-name}
constexpr const char* HOURLY_WITH_INDEX_AND_NAME = "Hourly.{}.{}";

constexpr const char* HOURLY_DEW_POINT = "DewPoint";
constexpr const char* HOURLY_FANART_CODE = "FanartCode";
constexpr const char* HOURLY_FEELS_LIKE = "FeelsLike";
constexpr const char* HOURLY_HUMIDITY = "Humidity";
constexpr const char* HOURLY_LONG_DATE = "LongDate";
constexpr const char* HOURLY_OUTLOOK = "Outlook";
constexpr const char* HOURLY_OUTLOOK_ICON = "OutlookIcon";
constexpr const char* HOURLY_PRECIPITATION = "Precipitation";
constexpr const char* HOURLY_PRESSURE = "Pressure";
constexpr const char* HOURLY_SHORT_DATE = "ShortDate";
constexpr const char* HOURLY_TEMPERATURE = "Temperature";
constexpr const char* HOURLY_TIME = "Time";
constexpr const char* HOURLY_WIND_DIRECTION = "WindDirection";
constexpr const char* HOURLY_WIND_SPEED = "WindSpeed";

// General properties
constexpr const char* GENERAL_LOCATIONS = "Locations";
constexpr const char* GENERAL_LOCATION_WITH_INDEX = "Location{}"; // Format: Location{1-based-index}

// Weather condition codes
constexpr const char* CONDITION_CODE_NOT_AVAILABLE = "na";

} // namespace KODI::WEATHER

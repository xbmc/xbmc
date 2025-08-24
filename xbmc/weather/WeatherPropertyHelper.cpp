/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WeatherPropertyHelper.h"

#include "LangInfo.h"
#include "guilib/GUIWindow.h"
#include "guilib/LocalizeStrings.h"
#include "utils/Map.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "weather/WeatherProperties.h"
#include "weather/WeatherTokenLocalizer.h"

#include <map>
#include <string_view>

using namespace KODI::WEATHER;

namespace
{
std::string FormatTemperature(const std::string& temperature)
{
  if (temperature.empty())
    return temperature;

  // Convert from celsius to system temperature unit and append resp. unit of measurement.
  const CTemperature t{CTemperature::CreateFromCelsius(std::strtod(temperature.c_str(), nullptr))};
  if (!t.IsValid())
    return temperature;

  return StringUtils::Format("{:.0f}{}", t.To(g_langInfo.GetTemperatureUnit()),
                             g_langInfo.GetTemperatureUnitString());
}

std::string FormatWindSpeed(const std::string& speed)
{
  if (speed.empty())
    return speed;

  // Convert from kilometres per hour to system speed unit and append resp. unit of measurement.
  const CSpeed s{CSpeed::CreateFromKilometresPerHour(std::strtod(speed.c_str(), nullptr))};
  if (!s.IsValid())
    return speed;

  return StringUtils::Format("{} {}", static_cast<int>(s.To(g_langInfo.GetSpeedUnit())),
                             g_langInfo.GetSpeedUnitString());
}

std::string FormatPercentage(const std::string& percentage)
{
  if (percentage.empty())
    return {};

  if (StringUtils::EndsWith(percentage, "%")) // Some add-ons return value including unit
    return percentage;

  return StringUtils::Format("{}%", percentage);
}

enum class LocalizationType
{
  NONE,
  OVERVIEW,
  OVERVIEW_TOKEN,
};

struct PropertyDetails
{
  using FormatterPtr = std::add_pointer_t<std::string(const std::string&)>;
  FormatterPtr formatter;
  LocalizationType l10n{LocalizationType::NONE};
};

// clang-format off
constexpr auto propertyDetails = make_map<std::string_view, PropertyDetails>({
    {"Current.DewPoint",    {FormatTemperature, LocalizationType::NONE}},
    {"Current.FeelsLike",   {FormatTemperature, LocalizationType::NONE}},
    {"Current.Temperature", {FormatTemperature, LocalizationType::NONE}},
    {"HighTemp",            {FormatTemperature, LocalizationType::NONE}},
    {"LowTemp",             {FormatTemperature, LocalizationType::NONE}},
    {"Current.Cloudiness",  {FormatPercentage,  LocalizationType::NONE}},
    {"Humidity",            {FormatPercentage,  LocalizationType::NONE}},
    {"Precipitation",       {FormatPercentage,  LocalizationType::NONE}},
    {"Current.WindSpeed",   {FormatWindSpeed,   LocalizationType::NONE}},
    {"Current.UVIndex",     {{},                LocalizationType::OVERVIEW}},
    {"Condition",           {{},                LocalizationType::OVERVIEW}},
    {"Outlook",             {{},                LocalizationType::OVERVIEW}},
    {"WindDirection",       {{},                LocalizationType::OVERVIEW_TOKEN}},
    {"Title",               {{},                LocalizationType::OVERVIEW_TOKEN}},
});
// clang-format on
} // unnamed namespace

bool CWeatherPropertyHelper::HasProperty(const std::string& prop) const
{
  return !m_window.GetProperty(prop).isNull();
}

CWeatherPropertyHelper::Property CWeatherPropertyHelper::GetProperty(const std::string& prop) const
{
  if (prop.empty())
    return {};

  std::string val{m_window.GetProperty(prop).asString()};

  // direct match?
  auto it{propertyDetails.find(prop)};
  if (it == propertyDetails.cend())
  {
    // match via last token?
    const size_t pos{prop.find_last_of('.')};
    if (pos != std::string::npos && pos < (prop.size() - 1))
      it = propertyDetails.find(std::string_view(prop).substr(pos + 1));
  }

  if (it != propertyDetails.cend())
  {
    const auto& [formatter, l10n] = (*it).second;

    // Need to format the prop?
    if (formatter)
      val = formatter(val);

    // Need to localize the prop?
    if (l10n == LocalizationType::OVERVIEW)
      val = m_localizer.LocalizeOverview(val);
    else if (l10n == LocalizationType::OVERVIEW_TOKEN)
      val = m_localizer.LocalizeOverviewToken(val);
  }

  return {prop, val};
}

CWeatherPropertyHelper::Property CWeatherPropertyHelper::GetDayProperty(
    int index, const std::string& prop) const
{
  // Note: No '.' after 'Day'. This is different from 'Daily' and 'Hourly'.
  return GetProperty(StringUtils::Format(DAY_WITH_INDEX_AND_NAME, index, prop));
}

CWeatherPropertyHelper::Property CWeatherPropertyHelper::GetDailyProperty(
    int index, const std::string& prop) const
{
  return GetProperty(StringUtils::Format(DAILY_WITH_INDEX_AND_NAME, index, prop));
}

CWeatherPropertyHelper::Property CWeatherPropertyHelper::GetHourlyProperty(
    int index, const std::string& prop) const
{
  return GetProperty(StringUtils::Format(HOURLY_WITH_INDEX_AND_NAME, index, prop));
}

bool CWeatherPropertyHelper::HasDailyProperties(int index) const
{
  // No date value, assume no data for this index.
  return !m_window
              .GetProperty(StringUtils::Format(DAILY_WITH_INDEX_AND_NAME, index, DAILY_SHORT_DATE))
              .empty();
}

bool CWeatherPropertyHelper::HasHourlyProperties(int index) const
{
  // No time value, assume no data for this index.
  return !m_window.GetProperty(StringUtils::Format(HOURLY_WITH_INDEX_AND_NAME, index, HOURLY_TIME))
              .empty();
}

std::string CWeatherPropertyHelper::FormatWind(const std::string& direction, const CSpeed& speed)
{
  if (direction == "CALM")
  {
    return g_localizeStrings.Get(1410); // Calm
  }
  else
  {
    if (!speed.IsValid())
      return {};

    if (direction.empty())
    {
      return StringUtils::Format("{} {}", speed.To(g_langInfo.GetSpeedUnit()),
                                 g_langInfo.GetSpeedUnitString());
    }
    else
    {
      return StringUtils::Format(g_localizeStrings.Get(434), // From {direction} at {speed} {unit}
                                 direction, static_cast<int>(speed.To(g_langInfo.GetSpeedUnit())),
                                 g_langInfo.GetSpeedUnitString());
    }
  }
}

std::string CWeatherPropertyHelper::FormatFanartCode(const std::string& fanartCode,
                                                     const std::string& outlookIcon)
{
  if (!fanartCode.empty())
    return fanartCode; // take what we have

  if (!outlookIcon.empty())
  {
    // As fallback, extract code from outlook icon file name
    std::string code{URIUtils::GetFileName(outlookIcon)};
    URIUtils::RemoveExtension(code);
    return code;
  }

  return CONDITION_CODE_NOT_AVAILABLE;
}

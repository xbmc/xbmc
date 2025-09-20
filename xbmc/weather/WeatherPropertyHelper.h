/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <tuple>

class CGUIWindow;
class CSpeed;
class CWeatherTokenLocalizer;

namespace KODI::WEATHER
{
class CWeatherPropertyHelper
{
public:
  CWeatherPropertyHelper(const CGUIWindow& window, CWeatherTokenLocalizer& localizer)
    : m_window(window),
      m_localizer(localizer)
  {
  }

  bool HasProperty(const std::string& prop) const;

  using PropertyName = std::string;
  using PropertyValue = std::string;
  using Property = std::tuple<PropertyName, PropertyValue>;

  Property GetProperty(const std::string& prop) const;
  Property GetDayProperty(int index, const std::string& prop) const;
  Property GetDailyProperty(int index, const std::string& prop) const;
  Property GetHourlyProperty(int index, const std::string& prop) const;

  bool HasDailyProperties(int index) const;
  bool HasHourlyProperties(int index) const;

  static std::string FormatWind(const std::string& direction, const CSpeed& speed);
  static std::string FormatFanartCode(const std::string& fanartCode,
                                      const std::string& outlookIcon);

private:
  const CGUIWindow& m_window;
  CWeatherTokenLocalizer& m_localizer;
};
} // namespace KODI::WEATHER

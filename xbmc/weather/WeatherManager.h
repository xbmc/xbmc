/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingCallback.h"
#include "threads/CriticalSection.h"
#include "utils/InfoLoader.h"

#include <array>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ADDON
{
class CAddonMgr;
} // namespace ADDON

constexpr unsigned int WEATHER_LABEL_LOCATION = 10;
constexpr unsigned int WEATHER_IMAGE_CURRENT_ICON = 21;
constexpr unsigned int WEATHER_LABEL_CURRENT_COND = 22;
constexpr unsigned int WEATHER_LABEL_CURRENT_TEMP = 23;
constexpr unsigned int WEATHER_LABEL_CURRENT_FEEL = 24;
constexpr unsigned int WEATHER_LABEL_CURRENT_UVID = 25;
constexpr unsigned int WEATHER_LABEL_CURRENT_WIND = 26;
constexpr unsigned int WEATHER_LABEL_CURRENT_DEWP = 27;
constexpr unsigned int WEATHER_LABEL_CURRENT_HUMI = 28;

constexpr const char* ICON_ADDON_PATH = "resource://resource.images.weathericons.default";

struct ForecastDay
{
  std::string m_icon;
  std::string m_overview;
  std::string m_day;
  std::string m_high;
  std::string m_low;
};

struct WeatherInfo
{
  static constexpr unsigned int NUM_DAYS = 7;
  std::array<ForecastDay, NUM_DAYS> forecast;

  std::string lastUpdateTime;
  std::string location;
  std::string currentIcon;
  std::string currentConditions;
  std::string currentTemperature;
  std::string currentFeelsLike;
  std::string currentUVIndex;
  std::string currentWind;
  std::string currentDewPoint;
  std::string currentHumidity;
};

class CWeatherManager : public CInfoLoader, public ISettingCallback
{
public:
  explicit CWeatherManager(ADDON::CAddonMgr& addonManager);
  ~CWeatherManager() override;

  /*!
   \brief The intervall for refreshing weather data.
   */
  static constexpr int WEATHER_REFRESH_INTERVAL_MS = 30 * 60 * 1000; // 30 minutes

  /*!
   \brief Retrieve the value for the given weather property
   \param property the full name of the property (e.g. Current.Temperature, Hourly.1.Temperature)
   \return the property value
   */
  std::string GetProperty(const std::string& property) const;

  /*!
   \brief Retrieve the value for the given "day" weather property
   \param index the index for the day, (can be in the range [0..6])
   \param property the name of the property (e.g. HighTemp)
   \return the property value
   */
  std::string GetDayProperty(unsigned int index, const std::string& property) const;

  /*!
   \brief Get the index for currently active location
   \return the id of the location
   */
  int GetLocation() const;

  /*!
   \brief Sets the location to the specified index and refreshes the weather data.
   No concurrent updates. First request wins, subsequent requests ignored until completion.
   \param location the location index (can be any value except INVALID_LOCATION)
   */
  void SetLocation(int location);

  /*!
   \brief Get the city names of all available locations, sorted by location index.
   \return the city names
   \sa GetLocation
   */
  std::vector<std::string> GetLocations() const;

  /*!
   \brief Retrieve the city name for the specified location from the settings
   \param iLocation the location index (can be any value except INVALID_LOCATION)
   \return the city name (without the accompanying region area code)
   */
  std::string GetLocation(int iLocation) const;

  std::string GetLastUpdateTime() const;
  ForecastDay GetForecast(int day) const;
  bool IsFetched();

  struct CaseInsensitiveCompare
  {
    using is_transparent = void; // Enables heterogeneous operations.
    bool operator()(std::string_view lhs, std::string_view rhs) const;
  };

  using WeatherInfoV2 = std::map<std::string, std::string, CaseInsensitiveCompare>;

protected:
  CJob* GetJob() const override;
  std::string TranslateInfo(int info) const override;
  std::string BusyInfo(int info) const override;
  void OnJobComplete(unsigned int jobID, bool success, CJob* job) override;

  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;
  void OnSettingAction(const std::shared_ptr<const CSetting>& setting) override;

private:
  CWeatherManager() = delete;
  void Reset();

  // Construction parameters
  ADDON::CAddonMgr& m_addonManager;

  // Synchronization parameters
  mutable CCriticalSection m_critSection;

  // State parameters
  WeatherInfo m_info{};
  mutable WeatherInfoV2 m_infoV2{};

  static constexpr int INVALID_LOCATION{0};
  int m_location{1}; // Current active location
  int m_newLocation{INVALID_LOCATION}; // Pending location update, or INVALID_LOCATION if none
};

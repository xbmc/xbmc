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
#include <memory>
#include <string>

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
  CWeatherManager();
  ~CWeatherManager() override;

  /*!
   \brief Retrieve the city name for the specified location from the settings
   \param iLocation the location index (can be in the range [1..MAXLOCATION])
   \return the city name (without the accompanying region area code)
   */
  std::string GetLocation(int iLocation) const;

  std::string GetLastUpdateTime() const;
  ForecastDay GetForecast(int day) const;
  bool IsFetched();
  void Reset();

  /*!
   \brief Saves the specified location index to the settings. Call Refresh()
          afterwards to update weather info for the new location.
   \param iLocation the new location index (can be in the range [1..MAXLOCATION])
   */
  static void SetArea(int iLocation);

  /*!
   \brief Retrieves the current location index from the settings
   \return the active location index (will be in the range [1..MAXLOCATION])
   */
  static int GetArea();

protected:
  CJob* GetJob() const override;
  std::string TranslateInfo(int info) const override;
  std::string BusyInfo(int info) const override;
  void OnJobComplete(unsigned int jobID, bool success, CJob* job) override;

  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;
  void OnSettingAction(const std::shared_ptr<const CSetting>& setting) override;

private:
  mutable CCriticalSection m_critSection;
  WeatherInfo m_info{};
};

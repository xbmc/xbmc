/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/InfoLoader.h"
#include "settings/lib/ISettingCallback.h"

#include <string>

#define WEATHER_LABEL_LOCATION   10
#define WEATHER_IMAGE_CURRENT_ICON 21
#define WEATHER_LABEL_CURRENT_COND 22
#define WEATHER_LABEL_CURRENT_TEMP 23
#define WEATHER_LABEL_CURRENT_FEEL 24
#define WEATHER_LABEL_CURRENT_UVID 25
#define WEATHER_LABEL_CURRENT_WIND 26
#define WEATHER_LABEL_CURRENT_DEWP 27
#define WEATHER_LABEL_CURRENT_HUMI 28

class CSettings;

namespace ADDON
{
  class CAddonMgr;
}

static const std::string ICON_ADDON_PATH = "resource://resource.images.weathericons.default";

struct ForecastDay
{
  std::string m_icon;
  std::string m_overview;
  std::string m_day;
  std::string m_high;
  std::string m_low;
};

#define NUM_DAYS 7

class CWeatherInfo
{
public:
  ForecastDay forecast[NUM_DAYS];

  void Reset()
  {
    lastUpdateTime.clear();
    currentIcon.clear();
    currentConditions.clear();
    currentTemperature.clear();
    currentFeelsLike.clear();
    currentWind.clear();
    currentHumidity.clear();
    currentUVIndex.clear();
    currentDewPoint.clear();

    for (int i = 0; i < NUM_DAYS; i++)
    {
      forecast[i].m_icon.clear();
      forecast[i].m_overview.clear();
      forecast[i].m_day.clear();
      forecast[i].m_high.clear();
      forecast[i].m_low.clear();
    }
  };

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
  std::string busyString;
  std::string naIcon;
};

class CWeatherManager
: public CInfoLoader, public ISettingCallback
{
public:
  CWeatherManager(CSettings &settings,
                  ADDON::CAddonMgr &addonManager);
  ~CWeatherManager(void) override;
  static bool GetSearchResults(const std::string &strSearch, std::string &strResult);

  std::string GetLocation(int iLocation);
  const std::string &GetLastUpdateTime() const { return m_info.lastUpdateTime; };
  const ForecastDay &GetForecast(int day) const;
  bool IsFetched();
  void Reset();

  void SetArea(int iLocation);
  int GetArea() const;
protected:
  CJob *GetJob() const override;
  std::string TranslateInfo(int info) const override;
  std::string BusyInfo(int info) const override;
  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override;

  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;
  void OnSettingAction(std::shared_ptr<const CSetting> setting) override;

private:
  // Construction parameters
  CSettings &m_settings;
  ADDON::CAddonMgr &m_addonManager;

  CWeatherInfo m_info;
};

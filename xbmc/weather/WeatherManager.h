/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
  CWeatherManager(void);
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

  CWeatherInfo m_info;
};

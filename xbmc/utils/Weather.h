#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "InfoLoader.h"
#include "StdString.h"
#include "utils/GlobalsHandling.h"

#include <map>

class TiXmlElement;

#define WEATHER_LABEL_LOCATION   10
#define WEATHER_IMAGE_CURRENT_ICON 21
#define WEATHER_LABEL_CURRENT_COND 22
#define WEATHER_LABEL_CURRENT_TEMP 23
#define WEATHER_LABEL_CURRENT_FEEL 24
#define WEATHER_LABEL_CURRENT_UVID 25
#define WEATHER_LABEL_CURRENT_WIND 26
#define WEATHER_LABEL_CURRENT_DEWP 27
#define WEATHER_LABEL_CURRENT_HUMI 28

struct day_forecast
{
  CStdString m_icon;
  CStdString m_overview;
  CStdString m_day;
  CStdString m_high;
  CStdString m_low;
};

#define NUM_DAYS 4

class CWeatherInfo
{
public:
  day_forecast forecast[NUM_DAYS];

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

  CStdString lastUpdateTime;
  CStdString location;
  CStdString currentIcon;
  CStdString currentConditions;
  CStdString currentTemperature;
  CStdString currentFeelsLike;
  CStdString currentUVIndex;
  CStdString currentWind;
  CStdString currentDewPoint;
  CStdString currentHumidity;
  CStdString busyString;
  CStdString naIcon;
};

class CWeatherJob : public CJob
{
public:
  CWeatherJob(int location);

  virtual bool DoWork();

  const CWeatherInfo &GetInfo() const;
private:
  void LocalizeOverview(CStdString &str);
  void LocalizeOverviewToken(CStdString &str);
  void LoadLocalizedToken();
  int ConvertSpeed(int speed);

  void SetFromProperties();

  /*! \brief Formats a celcius temperature into a string based on the users locale
   \param text the string to format
   \param temp the temperature (in degrees celcius).
   */
  void FormatTemperature(CStdString &text, int temp);

  std::map<CStdString, int> m_localizedTokens;
  typedef std::map<CStdString, int>::const_iterator ilocalizedTokens;

  CWeatherInfo m_info;
  int m_location;

  static bool m_imagesOkay;
};

class CWeather : public CInfoLoader
{
public:
  CWeather(void);
  virtual ~CWeather(void);
  static bool GetSearchResults(const CStdString &strSearch, CStdString &strResult);

  CStdString GetLocation(int iLocation);
  const CStdString &GetLastUpdateTime() const { return m_info.lastUpdateTime; };
  const day_forecast &GetForecast(int day) const;
  bool IsFetched();
  void Reset();

  void SetArea(int iLocation);
  int GetArea() const;
protected:
  virtual CJob *GetJob() const;
  virtual CStdString TranslateInfo(int info) const;
  virtual CStdString BusyInfo(int info) const;
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

private:

  CWeatherInfo m_info;
};

XBMC_GLOBAL_REF(CWeather, g_weatherManager);
#define g_weatherManager XBMC_GLOBAL_USE(CWeather)

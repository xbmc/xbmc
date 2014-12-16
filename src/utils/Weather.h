#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "InfoLoader.h"
#include "settings/lib/ISettingCallback.h"
#include "utils/GlobalsHandling.h"

#include <map>
#include <string>

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

class CWeatherJob : public CJob
{
public:
  CWeatherJob(int location);

  virtual bool DoWork();

  const CWeatherInfo &GetInfo() const;
private:
  void LocalizeOverview(std::string &str);
  void LocalizeOverviewToken(std::string &str);
  void LoadLocalizedToken();
  static int ConvertSpeed(int speed);

  void SetFromProperties();

  /*! \brief Formats a celcius temperature into a string based on the users locale
   \param text the string to format
   \param temp the temperature (in degrees celcius).
   */
  static void FormatTemperature(std::string &text, int temp);

  struct ci_less : std::binary_function<std::string, std::string, bool>
  {
    // case-independent (ci) compare_less binary function
    struct nocase_compare : public std::binary_function<unsigned char,unsigned char,bool>
    {
      bool operator() (const unsigned char& c1, const unsigned char& c2) const {
          return tolower (c1) < tolower (c2);
      }
    };
    bool operator() (const std::string & s1, const std::string & s2) const {
      return std::lexicographical_compare
        (s1.begin (), s1.end (),
        s2.begin (), s2.end (),
        nocase_compare ());
    }
  };

  std::map<std::string, int, ci_less> m_localizedTokens;
  typedef std::map<std::string, int, ci_less>::const_iterator ilocalizedTokens;

  CWeatherInfo m_info;
  int m_location;

  static bool m_imagesOkay;
};

class CWeather : public CInfoLoader,
                 public ISettingCallback
{
public:
  CWeather(void);
  virtual ~CWeather(void);
  static bool GetSearchResults(const std::string &strSearch, std::string &strResult);

  std::string GetLocation(int iLocation);
  const std::string &GetLastUpdateTime() const { return m_info.lastUpdateTime; };
  const day_forecast &GetForecast(int day) const;
  bool IsFetched();
  void Reset();

  void SetArea(int iLocation);
  int GetArea() const;
protected:
  virtual CJob *GetJob() const;
  virtual std::string TranslateInfo(int info) const;
  virtual std::string BusyInfo(int info) const;
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction(const CSetting *setting);

private:

  CWeatherInfo m_info;
};

XBMC_GLOBAL_REF(CWeather, g_weatherManager);
#define g_weatherManager XBMC_GLOBAL_USE(CWeather)

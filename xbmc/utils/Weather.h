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

struct day_forcast
{
  CStdString m_icon;
  CStdString m_overview;
  CStdString m_day;
  CStdString m_high;
  CStdString m_low;
};

#define NUM_DAYS 4

class CWeather : public CInfoLoader
{
public:
  CWeather(void);
  virtual ~CWeather(void);
  static bool GetSearchResults(const CStdString &strSearch, CStdString &strResult);

  CStdString GetLocation(int iLocation);
  const CStdString &GetLastUpdateTime() const { return m_lastUpdateTime; };
  bool IsFetched();
  void Reset();

  void SetArea(int iArea) { m_iCurWeather = iArea; };
  int GetArea() const { return m_iCurWeather; };
  CStdString GetAreaCode(const CStdString &codeAndCity) const;
  CStdString GetAreaCity(const CStdString &codeAndCity) const;

  day_forcast m_dfForcast[NUM_DAYS];
protected:
  virtual void DoWork();
  virtual CStdString TranslateInfo(int info) const;
  virtual CStdString BusyInfo(int info) const;

private:
  bool LoadWeather(const CStdString& strWeatherFile); //parse strWeatherFile
  void GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, CStdString &value, const CStdString& strDefaultValue);
  void GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName, int& iValue);
  void LocalizeOverview(CStdString &str);
  void LocalizeOverviewToken(CStdString &str);
  void LocalizeDay(CStdString &day);
  void LoadLocalizedToken();
  int ConvertSpeed(int speed);
  std::map<CStdString, int> m_localizedTokens;
  typedef std::map<CStdString, int>::const_iterator ilocalizedTokens;

  CStdString m_location[3];

  // Last updated
  CStdString m_lastUpdateTime;
  // Now weather
  CStdString m_currentIcon;
  CStdString m_currentConditions;
  CStdString m_currentTemperature;
  CStdString m_currentFeelsLike;
  CStdString m_currentUVIndex;
  CStdString m_currentWind;
  CStdString m_currentDewPoint;
  CStdString m_currentHumidity;
  CStdString m_busyString;
  CStdString m_naIcon;

  unsigned int m_iCurWeather;
  bool m_bImagesOkay;
};

extern CWeather g_weatherManager;

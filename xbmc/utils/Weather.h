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
  char m_szIcon[256];
  char m_szOverview[256];
  char m_szDay[20];
  char m_szHigh[15];
  char m_szLow[15];
};

#define NUM_DAYS 4

class CBackgroundWeatherLoader : public CBackgroundLoader
{
public:
  CBackgroundWeatherLoader(CInfoLoader *pCallback) : CBackgroundLoader(pCallback) {};

protected:
  virtual void GetInformation();
};

class CWeather : public CInfoLoader
{
public:
  CWeather(void);
  virtual ~CWeather(void);
  static bool GetSearchResults(const CStdString &strSearch, CStdString &strResult);
  bool LoadWeather(const CStdString& strWeatherFile); //parse strWeatherFile

  char *GetLocation(int iLocation);
  char *GetLastUpdateTime() { return m_szLastUpdateTime; };
  bool IsFetched();
  void Reset();

  void SetArea(int iArea) { m_iCurWeather = iArea; };
  int GetArea() const { return m_iCurWeather; };
  CStdString GetAreaCode(const CStdString &codeAndCity) const;
  CStdString GetAreaCity(const CStdString &codeAndCity) const;

  day_forcast m_dfForcast[NUM_DAYS];
  bool m_bImagesOkay;
protected:
  virtual const char *TranslateInfo(DWORD dwInfo);
  virtual const char *BusyInfo(DWORD dwInfo);
  virtual DWORD TimeToNextRefreshInMs();

  void GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, char* szValue, const CStdString& strDefaultValue);
  void GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName, int& iValue);
  void LocalizeOverview(char *szStr);
  void LocalizeOverviewToken(char *szStr, bool bAppendSpace = true);
  void LocalizeDay(char *szDay);
  void LoadLocalizedToken();
  int ConvertSpeed(int speed);
  std::map<CStdString, DWORD> m_localizedTokens;
  typedef std::map<CStdString, DWORD>::const_iterator ilocalizedTokens;

  char m_szLocation[3][100];

  // Last updated
  char m_szLastUpdateTime[256];
  // Now weather
  char m_szCurrentIcon[256];
  char m_szCurrentConditions[256];
  char m_szCurrentTemperature[10];
  char m_szCurrentFeelsLike[10];
  char m_szCurrentUVIndex[10];
  char m_szCurrentWind[256];
  char m_szCurrentDewPoint[10];
  char m_szCurrentHumidity[10];
  char m_szBusyString[256];
  char m_szNAIcon[256];

  unsigned int m_iCurWeather;
};

extern CWeather g_weatherManager;

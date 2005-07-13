#pragma once
#include "HTTP.h"
#include "thread.h"

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

class CWeather;

class CBackgroundWeatherLoader : public CThread
{
public:
  CBackgroundWeatherLoader(CWeather *pCallback, int iArea);
  ~CBackgroundWeatherLoader();

private:
  void Process();
  int m_iArea;
  CWeather *m_pCallback;
  bool m_bImagesOkay;
};

class CWeather
{
public:
  CWeather(void);
  virtual ~CWeather(void);
  static bool GetSearchResults(const CStdString &strSearch, CStdString &strResult);
  bool LoadWeather(const CStdString& strWeatherFile); //parse strWeatherFile
  void Refresh(int iArea);
  void LoaderFinished();

  char *GetLocation(int iLocation) { return m_szLocation[iLocation]; };
  const char *GetLabel(DWORD dwLabel);
  char *GetLastUpdateTime() { return m_szLastUpdateTime; };
  char *GetCurrentIcon();
  char *GetCurrentConditions() { return m_szCurrentConditions; };
  char *GetCurrentTemperature() { return m_szCurrentTemperature; };
  char *GetCurrentFeelsLike() { return m_szCurrentFeelsLike; };
  char *GetCurrentUVIndex() { return m_szCurrentUVIndex; };
  char *GetCurrentWind() { return m_szCurrentWind; };
  char *GetCurrentDewPoint() { return m_szCurrentDewPoint; };
  char *GetCurrentHumidity() { return m_szCurrentHumidity; };

  day_forcast m_dfForcast[NUM_DAYS];

protected:
  bool Download(const CStdString& strWeatherFile);  //download to strWeatherFile
  void GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, char* szValue, const CStdString& strDefaultValue);
  void GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName, int& iValue);
  void LocalizeOverview(char *szStr);
  void LocalizeOverviewToken(char *szStr, bool bAppendSpace = true);
  void LocalizeDay(char *szDay);
  void LoadLocalizedToken();
  void SplitLongString(char *szString, int splitStart, int splitEnd);
  int ConvertSpeed(int speed);
  map<wstring, DWORD> m_localizedTokens;
  typedef map<wstring, DWORD>::const_iterator ilocalizedTokens;

  char m_szLocation[3][100];

  DWORD m_lRefreshTime;  //for autorefresh
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
  bool m_bBusy;
  CBackgroundWeatherLoader *m_pBackgroundLoader;
};

extern CWeather g_weatherManager;

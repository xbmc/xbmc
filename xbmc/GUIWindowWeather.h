#pragma once
#include "guiImage.h"
#include "guiwindow.h"
#include "stdstring.h"
#include "utils\HTTP.h"
#include "tinyxml/tinyxml.h"
#include "guiDialogProgress.h"

using namespace std;

struct day_forcast
{
	char		m_szIcon[256];
	char		m_szOverview[256];
	char		m_szDay[20];
	char		m_szHigh[15];
	char		m_szLow[15];
	CGUIImage	*pImage;
};

#define NUM_DAYS	4

class CGUIWindowWeather : 	public CGUIWindow
{
public:
	CGUIWindowWeather(void);
	virtual ~CGUIWindowWeather(void);
	virtual bool			OnMessage(CGUIMessage& message);
	virtual void			OnAction(const CAction &action);
	virtual void			Render();

protected:
	void					UpdateButtons();
	bool					Download(const CStdString& strWeatherFile);		//download to strWeatherFile
	bool					LoadWeather(const CStdString& strWeatherFile);	//parse strWeatherFile
	void					GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, char* szValue, const CStdString& strDefaultValue);
	void					GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName, int& iValue);
	void					RefreshMe(bool autoUpdate);
	void					SplitLongString(char *szString, int splitStart, int splitEnd);
	void					LocalizeDay(char *szDay);
	void					LocalizeOverview(char *szStr);
	void					LocalizeOverviewToken(char *szStr);
	void					LoadLocalizedToken();



	CHTTP					m_httpGrabber;
	char					m_szLocation[3][100];
	char					m_szUpdated[256];
	char					m_szNowIcon[256];
	char					m_szNowCond[256];
	char					m_szNowTemp[10];
	char					m_szNowFeel[10];
	char					m_szNowUVId[10];
	char					m_szNowWind[256];
	char					m_szNowDewp[10];
	char					m_szNowHumd[10];

	day_forcast				m_dfForcast[NUM_DAYS];

	CGUIImage				*pNowImage;
	DWORD					m_lRefreshTime;		//for autorefresh

	unsigned int			m_iCurWeather;

	map<wstring,DWORD> m_localizedTokens;
	typedef map<wstring,DWORD>::const_iterator ilocalizedTokens;

};

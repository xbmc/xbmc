#include "GUIWindowWeather.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "guiDialogProgress.h"
#include "GUIDialogOK.h"
#include "localizestrings.h"
#include "util.h"
#include <algorithm>

#define CONTROL_BTNREFRESH		2
#define CONTROL_LABELLOCATION	10
#define CONTROL_LABELUPDATED	11
#define CONTROL_IMAGELOGO		101

#define CONTROL_IMAGENOWICON	21
#define CONTROL_LABELNOWCOND	22
#define	CONTROL_LABELNOWTEMP	23
#define CONTORL_LABELNOWFEEL	24
#define CONTROL_LABELNOWUVID	25
#define CONTROL_LABELNOWWIND	26
#define	CONTROL_LABELNOWDEWP	27
#define CONTORL_LABELNOWHUMI	28

#define CONTROL_STATICTEMP		223
#define CONTROL_STATICFEEL		224
#define CONTROL_STATICUVID		225
#define CONTROL_STATICWIND		226
#define CONTROL_STATICDEWP		227
#define CONTROL_STATICHUMI		228

#define CONTROL_LABELD0DAY		31
#define CONTROL_LABELD0HI		32
#define CONTROL_LABELD0LOW		33
#define CONTROL_LABELD0GEN		34
#define CONTROL_IMAGED0IMG		35

#define DEGREE_CHARACTER		(char)176	//the degree 'o' character

#define PARTNER_ID				"1004124588"			//weather.com partner id
#define PARTNER_KEY				"079f24145f208494"		//weather.com partner key

/*
FIXME'S
>strings are not centered
>weather.com dev account is mine not a general xbmc one
>do something when entering the weather screen for the first time
	download if current weather is out of date or something?
>weather.com doesn't return xml for some area codes? UKXX1000, UKXX1201 (not my fault?)

LONGTERM:
>Allow mixing of units (eg oC and mph for us crazy brits)
>Add settings screen with text input search for location code
	+ select metric/imperial or (F/C and MPH / KMH seperatly)
*/

int FtoC(int Far)
{
	return (int)((Far-32)*(5.0/9.0) + 0.5);  //0.5 for rounding (seems a bit hacky)
}
CGUIWindowWeather::CGUIWindowWeather(void)
:CGUIWindow(0)
{
	pNowImage = NULL;
	strcpy(m_szLocation, "");
	strcpy(m_szUpdated, "");
	strcpy(m_szNowIcon, "Q:\\weather\\128x128\\na.png");
	strcpy(m_szNowCond, "");
	strcpy(m_szNowTemp, "");
	strcpy(m_szNowFeel, "");

	strcpy(m_szNowWind, "");
	strcpy(m_szNowHumd, "");
	strcpy(m_szNowUVId, "");
	strcpy(m_szNowDewp, "");

	//loop here as well
	for(int i=0; i<NUM_DAYS; i++)
	{
		strcpy(m_dfForcast[i].m_szIcon, "Q:\\weather\\64x64\\na.png");
		strcpy(m_dfForcast[i].m_szOverview, "");
		strcpy(m_dfForcast[i].m_szDay, "");
		strcpy(m_dfForcast[i].m_szHigh, "");
		strcpy(m_dfForcast[i].m_szLow, "");
	}
}

CGUIWindowWeather::~CGUIWindowWeather(void)
{
}

void CGUIWindowWeather::OnAction(const CAction &action)
{
	if (action.wID == ACTION_PREVIOUS_MENU)
	{
		m_gWindowManager.PreviousWindow();
		return;
	}
	CGUIWindow::OnAction(action);
}

bool CGUIWindowWeather::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
	{
		case GUI_MSG_WINDOW_DEINIT:
			//Clear();
		break;

		case GUI_MSG_WINDOW_INIT:
				CGUIWindow::OnMessage(message);

				pNowImage = new CGUIImage(GetID(), CONTROL_IMAGENOWICON, 220, 170, 128, 128, m_szNowIcon, 0);
				Add(pNowImage);

				for(int i=0; i<NUM_DAYS; i++) 
				{
					m_dfForcast[i].pImage = new CGUIImage(GetID(), CONTROL_IMAGED0IMG+(i*10), 220+(i*125), 455, 64, 64, m_dfForcast[i].m_szIcon, 0);
					Add(m_dfForcast[i].pImage);
				}

				UpdateButtons();
		break;

		case GUI_MSG_CLICKED:
		{
			int iControl=message.GetSenderId();
			if (iControl == CONTROL_BTNREFRESH)
				RefreshMe();	//refresh clicked so do a complete update
		}
		break;
	}
	return CGUIWindow::OnMessage(message);
}


void CGUIWindowWeather::UpdateButtons()
{
	SET_CONTROL_LABEL(GetID(), CONTROL_BTNREFRESH, 184);			//Refresh
	SET_CONTROL_LABEL(GetID(), CONTROL_LABELLOCATION, m_szLocation);
	SET_CONTROL_LABEL(GetID(), CONTROL_LABELUPDATED, m_szUpdated);

	//urgh, remove, create then add image each refresh to update nicely
	Remove(pNowImage->GetID());
	pNowImage = new CGUIImage(GetID(), CONTROL_IMAGENOWICON, 220, 170, 128, 128, m_szNowIcon, 0);
	Add(pNowImage);

	SET_CONTROL_LABEL(GetID(), CONTROL_LABELNOWCOND, m_szNowCond);
	SET_CONTROL_LABEL(GetID(), CONTROL_LABELNOWTEMP, m_szNowTemp);
	SET_CONTROL_LABEL(GetID(), CONTORL_LABELNOWFEEL, m_szNowFeel);
	SET_CONTROL_LABEL(GetID(), CONTROL_LABELNOWUVID, m_szNowUVId);
	SET_CONTROL_LABEL(GetID(), CONTROL_LABELNOWWIND, m_szNowWind);
	SET_CONTROL_LABEL(GetID(), CONTROL_LABELNOWDEWP, m_szNowDewp);
	SET_CONTROL_LABEL(GetID(), CONTORL_LABELNOWHUMI, m_szNowHumd);

	//static labels
	SET_CONTROL_LABEL(GetID(), CONTROL_STATICTEMP, 401);		//Temperature
	SET_CONTROL_LABEL(GetID(), CONTROL_STATICFEEL, 402);		//Feels Like
	SET_CONTROL_LABEL(GetID(), CONTROL_STATICUVID, 403);		//UV Index
	SET_CONTROL_LABEL(GetID(), CONTROL_STATICWIND, 404);		//Wind
	SET_CONTROL_LABEL(GetID(), CONTROL_STATICDEWP, 405);		//Dew Point
	SET_CONTROL_LABEL(GetID(), CONTROL_STATICHUMI, 406);		//Humidity
	


	for(int i=0; i<NUM_DAYS; i++)
	{
		SET_CONTROL_LABEL(GetID(), CONTROL_LABELD0DAY+(i*10), m_dfForcast[i].m_szDay);
		SET_CONTROL_LABEL(GetID(), CONTROL_LABELD0HI+(i*10), m_dfForcast[i].m_szHigh);
		SET_CONTROL_LABEL(GetID(), CONTROL_LABELD0LOW+(i*10), m_dfForcast[i].m_szLow);
		SET_CONTROL_LABEL(GetID(), CONTROL_LABELD0GEN+(i*10), m_dfForcast[i].m_szOverview);
		
		//Seems a bit messy, but works. Remove, Create and then Add the image to update nicely
		Remove(m_dfForcast[i].pImage->GetID());
		m_dfForcast[i].pImage = new CGUIImage(GetID(), CONTROL_IMAGED0IMG+(i*10), 220+(i*125), 455, 64, 64, m_dfForcast[i].m_szIcon, 0);
		Add(m_dfForcast[i].pImage);
	}
}


void CGUIWindowWeather::Render()
{
	CGUIWindow::Render();
}

bool CGUIWindowWeather::Download(const CStdString& strWeatherFile)
{
	CGUIDialogProgress* 	pDlgProgress	 = (CGUIDialogProgress*)m_gWindowManager.GetWindow(101);
	CGUIDialogOK* 		 	pDlgOK			 = (CGUIDialogOK*)m_gWindowManager.GetWindow(2002);
	CStdString				strPath, strURL;
	
	//progress dialog for download
	pDlgProgress->SetHeading(410);							//"Accessing Weather.com"
	pDlgProgress->SetLine(0, 411);							//"Getting Weather For:"
	pDlgProgress->SetLine(1, g_stSettings.m_szWeatherArea);	//Area code
	if(strlen(m_szLocation) > 1)							//got the location string yet?
		pDlgProgress->SetLine(2, m_szLocation);
	else
		pDlgProgress->SetLine(2, "");
	pDlgProgress->StartModal(GetID());
	pDlgProgress->Progress();

	char c_units = g_stSettings.m_szWeatherFormat[0];
	if(c_units == 'i')	//weather.com uses 's'tandard instead of imperial :)
		c_units = 's';

	strURL.Format("http://xoap.weather.com/weather/local/%s?cc=*&unit=%c&dayf=4&prod=xoap&par=%s&key=%s",
				g_stSettings.m_szWeatherArea, c_units, PARTNER_ID, PARTNER_KEY);

	m_httpGrabber.SetHTTPVer(0);	//set to HTTP/1.0 to download nicely
	bool res = m_httpGrabber.Download(strURL, strWeatherFile);
	pDlgProgress->Close();
	
	if(!res)
	{
		// show failed dialog...
		pDlgOK->SetHeading(412);	//"Unable to get weather data"
		pDlgOK->SetLine(0, "");
		pDlgOK->SetLine(1, L"");
		pDlgOK->SetLine(2, L"");
		pDlgOK->DoModal(GetID());
	}

	return res;
}


bool CGUIWindowWeather::LoadWeather(const CStdString& strWeatherFile)
{
	int			iTmpInt;
	char		iTmpStr[256];
	char		szUnitTemp[2];
	char		szUnitSpeed[5];
	SYSTEMTIME	time;
	
	GetLocalTime(&time);	//used when deciding what weather to grab for today

	// load the xml file
	TiXmlDocument xmlDoc;
	if (!xmlDoc.LoadFile(strWeatherFile))
	{
		OutputDebugString("Unable to load:");
		OutputDebugString(strWeatherFile.c_str());
		OutputDebugString("\n");
		return false;
	}

	TiXmlElement *pRootElement = xmlDoc.RootElement();

	//if root element is 'error' display the error message
	if(strcmp(pRootElement->Value(), "error") == 0)
	{
		char szCheckError[256];
		CGUIDialogOK* pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(2002);

		GetString(pRootElement, "err", szCheckError, "Unknown Error");	//grab the error string

		// show error dialog...
		pDlgOK->SetHeading(412);	//"Unable to get weather data"
		pDlgOK->SetLine(0, szCheckError);
		pDlgOK->SetLine(1, L"");
		pDlgOK->SetLine(2, L"");
		pDlgOK->DoModal(GetID());
		return false;
	}

	// units (C or F and mph or km/h)
	TiXmlElement *pElement = pRootElement->FirstChildElement("head");
	if(pElement)
	{
		GetString(pElement, "ut", szUnitTemp, "C");
		GetString(pElement, "us", szUnitSpeed, "km/h");
	}

	// location
	pElement = pRootElement->FirstChildElement("loc");
	if(pElement)
	{
		GetString(pElement, "dnam", m_szLocation, "");
	}

	//current weather
	pElement = pRootElement->FirstChildElement("cc");
	if(pElement)
	{
		GetString(pElement, "lsup", m_szUpdated, "");

		GetInteger(pElement, "icon", iTmpInt);
		sprintf(m_szNowIcon, "Q:\\weather\\128x128\\%i.png", iTmpInt);

		GetString(pElement, "t", m_szNowCond, "");			//current condition
		SplitLongString(m_szNowCond, 8, 15);				//split to 2 lines if needed

		GetInteger(pElement, "tmp", iTmpInt);				//current temp
		sprintf(m_szNowTemp, "%i%c%s", iTmpInt, DEGREE_CHARACTER, szUnitTemp);	
		GetInteger(pElement, "flik", iTmpInt);				//current 'Feels Like'
		sprintf(m_szNowFeel, "%i%c%s", iTmpInt, DEGREE_CHARACTER, szUnitTemp);
		
		TiXmlElement *pNestElement = pElement->FirstChildElement("wind");	//current wind
		if(pNestElement)
		{
			GetInteger(pNestElement, "s", iTmpInt);			//current wind strength
			GetString(pNestElement, "t", iTmpStr, "N");		//current wind direction

			//From <dir eg NW> at <speed> km/h		 g_localizeStrings.Get(407)
			//This is a bit untidy, but i'm fed up with localization and string formats :)
			CStdString szWindFrom = g_localizeStrings.Get(407);
			CStdString szWindAt = g_localizeStrings.Get(408);

			sprintf(m_szNowWind, "%s %s %s %i %s", 
				szWindFrom.GetBuffer(szWindFrom.GetLength()), iTmpStr, 
				szWindAt.GetBuffer(szWindAt.GetLength()), iTmpInt, szUnitSpeed);
		}

		GetInteger(pElement, "hmid", iTmpInt);				//current humidity
		sprintf(m_szNowHumd, "%i%%", iTmpInt);

		pNestElement = pElement->FirstChildElement("uv");	//current UV index
		if(pNestElement)
		{
			GetInteger(pNestElement, "i", iTmpInt);	
			GetString(pNestElement, "t", iTmpStr, "");
			sprintf(m_szNowUVId, "%i %s", iTmpInt, iTmpStr);
		}

		GetInteger(pElement, "dewp", iTmpInt);				//current dew point
		sprintf(m_szNowDewp, "%i%c%s", iTmpInt, DEGREE_CHARACTER, szUnitTemp);

	}
	//future forcast
	pElement = pRootElement->FirstChildElement("dayf");
	if(pElement)
	{
		TiXmlElement *pOneDayElement = pElement->FirstChildElement("day");;
		for(int i=0; i<NUM_DAYS; i++)
		{
			if(pOneDayElement)
			{
				strcpy(m_dfForcast[i].m_szDay, pOneDayElement->Attribute("t"));
				LocalizeDay(m_dfForcast[i].m_szDay);

				GetString(pOneDayElement, "hi", iTmpStr, "");	//string cause i've seen it return N/A
				if(strcmp(iTmpStr, "N/A") == 0)
					strcpy(m_dfForcast[i].m_szHigh, "");
				else
					sprintf(m_dfForcast[i].m_szHigh, "%s%c%s", iTmpStr, DEGREE_CHARACTER, szUnitTemp);	

				GetString(pOneDayElement, "low", iTmpStr, "");
				if(strcmp(iTmpStr, "N/A") == 0)
					strcpy(m_dfForcast[i].m_szHigh, "");
				else
					sprintf(m_dfForcast[i].m_szLow, "%s%c%s", iTmpStr, DEGREE_CHARACTER, szUnitTemp);

				TiXmlElement *pDayTimeElement = pOneDayElement->FirstChildElement("part");	//grab the first day/night part (should be day)
				if(i == 0 && (time.wHour < 7 || time.wHour >= 19))	//weather.com works on a 7am to 7pm basis so grab night if its late in the day
					pDayTimeElement = pDayTimeElement->NextSiblingElement("part");

				if(pDayTimeElement)
				{
					GetInteger(pDayTimeElement, "icon", iTmpInt);
					sprintf(m_dfForcast[i].m_szIcon, "Q:\\weather\\64x64\\%i.png", iTmpInt);
					GetString(pDayTimeElement, "t", m_dfForcast[i].m_szOverview, "");
					SplitLongString(m_dfForcast[i].m_szOverview, 7, 15);
				}
			}
			pOneDayElement = pOneDayElement->NextSiblingElement("day");
		}
	}
	return true;
}


void CGUIWindowWeather::GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, char* szValue, const CStdString& strDefaultValue)
{
	strcpy(szValue,"");
	const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
	if (pChild)
	{
		CStdString strValue=pChild->FirstChild()->Value();
		if (strValue.size() )
		{
			if (strValue !="-")
				strcpy(szValue,strValue.c_str());
		}
	}
	if (strlen(szValue)==0)
	{
		strcpy(szValue,strDefaultValue.c_str());
	}
}

void CGUIWindowWeather::GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName, int& iValue)
{
	const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
	if (pChild)
	{
		iValue = atoi( pChild->FirstChild()->Value() );
	}
}

//Do a complete download, parse and update
void CGUIWindowWeather::RefreshMe()
{
	//message strings for refresh of images
	CGUIMessage msgDe(GUI_MSG_WINDOW_DEINIT,0,0);
	CGUIMessage msgRe(GUI_MSG_WINDOW_INIT,0,0,WINDOW_INVALID);
	CStdString strWeatherFile = "Q:\\weather\\curWeather.xml";
	
	Download(strWeatherFile);		//download
	LoadWeather(strWeatherFile);	//parse
	UpdateButtons();

	//Send the refresh messages
	OnMessage(msgDe);
	OnMessage(msgRe);
}


//was a nice idea, but the label's width isn't set in code, only manually in the .xml?
void CGUIWindowWeather::CenterLabel(DWORD dwControlID, DWORD dwWidth)
{
	/*CGUIControl* pControl = (CGUIControl*)GetControl(dwControlID);
	DWORD x = (dwWidth - pControl->GetWidth()) / 2;
	x += pControl->GetXPosition();*/

	/*	CGUIDialogOK* 		 	pDlgOK			 = (CGUIDialogOK*)m_gWindowManager.GetWindow(2002);
		char inte[10];
		itoa(dwWidth, inte, 10);
		pDlgOK->SetHeading(inte);	//aaaaand more strings
		itoa(pControl->GetWidth(), inte, 10);
		pDlgOK->SetLine(0, inte);
		itoa(pControl->GetXPosition(), inte, 10);
		pDlgOK->SetLine(1, inte);
		itoa(x, inte, 10);
		pDlgOK->SetLine(2, inte);
		pDlgOK->DoModal(GetID());*/

	//pControl->SetPosition(x, pControl->GetYPosition());
}

//splitStart + End are the chars to search between for a space to replace with a \n
void CGUIWindowWeather::SplitLongString(char *szString, int splitStart, int splitEnd)
{
	//search chars 10 to 15 for a space
	//if we find one, replace it with a newline
	for(int i=splitStart; i<splitEnd && i<(int)strlen(szString); i++)
	{
		if(szString[i] == ' ')
		{
			szString[i] = '\n';
			return;
		}
	}
}


//convert weather.com day strings into localized string id's
void CGUIWindowWeather::LocalizeDay(char *szDay)
{
	CStdString strLocDay;
		
	if(strcmp(szDay, "Monday") == 0)			//monday is localized string 11
		strLocDay = g_localizeStrings.Get(11);
	else if(strcmp(szDay, "Tuesday") == 0)
		strLocDay = g_localizeStrings.Get(12);
	else if(strcmp(szDay, "Wednesday") == 0)
		strLocDay = g_localizeStrings.Get(13);
	else if(strcmp(szDay, "Thursday") == 0)
		strLocDay = g_localizeStrings.Get(14);
	else if(strcmp(szDay, "Friday") == 0)
		strLocDay = g_localizeStrings.Get(15);
	else if(strcmp(szDay, "Saturday") == 0)
		strLocDay = g_localizeStrings.Get(16);
	else if(strcmp(szDay, "Sunday") == 0)
		strLocDay = g_localizeStrings.Get(17);
	else
		strLocDay = "";

	strcpy(szDay, strLocDay.GetBuffer(strLocDay.GetLength()));
}
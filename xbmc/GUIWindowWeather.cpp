
#include "stdafx.h"
#include "GUIWindowWeather.h"
#include "GUISpinControl.h"
#include "GUIDialogOK.h"
#include "guiWindowManager.h"
#include "settings.h"
#include "localizestrings.h"
#include "util.h"
#include <algorithm>
#include "utils/log.h"

#define CONTROL_BTNREFRESH		2
#define CONTROL_SELECTLOCATION	3
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

#define MAX_LOCATION			3
#define LOCALIZED_TOKEN_FIRSTID	370
#define LOCALIZED_TOKEN_LASTID	395
/*
FIXME'S
>strings are not centered
>weather.com dev account is mine not a general xbmc one
*/
int ConvertSpeed(int curSpeed)
{
	//we might not need to convert at all
	if((g_stSettings.m_szWeatherFTemp[0] == 'C' && g_stSettings.m_szWeatherFSpeed[0] == 'K') ||
		(g_stSettings.m_szWeatherFTemp[0] == 'F' && g_stSettings.m_szWeatherFSpeed[0] == 'M'))
		return curSpeed;

	//got through that so if temp is C, speed must be M or S
	if(g_stSettings.m_szWeatherFTemp[0] == 'C')
	{
		if (g_stSettings.m_szWeatherFSpeed[0] == 'S')
			return (int)(curSpeed * (1000.0/3600.0) + 0.5);		//mps
		else
			return (int)(curSpeed / (8.0/5.0));		//mph
	}
	else
	{
		if (g_stSettings.m_szWeatherFSpeed[0] == 'S')
			return (int)(curSpeed * (8.0/5.0) * (1000.0/3600.0) + 0.5);		//mps
		else
			return (int)(curSpeed * (8.0/5.0));		//kph
	}
}

CGUIWindowWeather::CGUIWindowWeather(void)
:CGUIWindow(0)
{
	pNowImage = NULL;
	for(int i=0; i<MAX_LOCATION; i++)
	{
		strcpy(m_szLocation[i],"");
	}
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

	m_iCurWeather = 0;
	srand(timeGetTime());
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

				//do image id to control stuff so we can use them later
				pNowImage = (CGUIImage*)GetControl(CONTROL_IMAGENOWICON);	
				for(int i=0; i<NUM_DAYS; i++) 
					m_dfForcast[i].pImage = (CGUIImage*)GetControl(CONTROL_IMAGED0IMG+(i*10));
				UpdateButtons();

				m_lRefreshTime = timeGetTime() - (g_stSettings.m_iWeatherRefresh*60000) + 2000; //refresh in 2 seconds
			LoadLocalizedToken();
		break;

		case GUI_MSG_CLICKED:
		{
			int iControl=message.GetSenderId();
			if(iControl == CONTROL_BTNREFRESH)
			{
				RefreshMe(false);	//refresh clicked so do a complete update (not an autoUpdate)
			}
			else if(iControl == CONTROL_SELECTLOCATION)
			{
				CGUISpinControl *pTempSpin = (CGUISpinControl*)GetControl(iControl);
				m_iCurWeather = pTempSpin->GetValue();
			}
		}
		break;
	}
	return CGUIWindow::OnMessage(message);
}


void CGUIWindowWeather::UpdateButtons()
{
	SET_CONTROL_LABEL(GetID(), CONTROL_BTNREFRESH, 184);			//Refresh
	SET_CONTROL_LABEL(GetID(), CONTROL_LABELLOCATION, m_szLocation[m_iCurWeather]);
	SET_CONTROL_LABEL(GetID(), CONTROL_LABELUPDATED, m_szUpdated);

	//urgh, remove, create then add image each refresh to update nicely
	Remove(pNowImage->GetID());
	int posX = pNowImage->GetXPosition();
	int posY = pNowImage->GetYPosition();
	pNowImage = new CGUIImage(GetID(), CONTROL_IMAGENOWICON, posX, posY, 128, 128, m_szNowIcon, 0);
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
		int posX = m_dfForcast[i].pImage->GetXPosition();
		int posY = m_dfForcast[i].pImage->GetYPosition();
		m_dfForcast[i].pImage = new CGUIImage(GetID(), CONTROL_IMAGED0IMG+(i*10), posX, posY, 64, 64, m_dfForcast[i].m_szIcon, 0);
		Add(m_dfForcast[i].pImage);
	}

	CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SELECTLOCATION, 0, 0, NULL);
	g_graphicsContext.SendMessage(msg);
	CGUIMessage msg2(GUI_MSG_LABEL_ADD, GetID(), CONTROL_SELECTLOCATION, 0, 0);
	for(int i=0; i<MAX_LOCATION; i++)
	{
		if(strlen(m_szLocation[i]) > 1)							//got the location string yet?
		{
			CStdString strLabel = m_szLocation[i];
			int iPos = strLabel.ReverseFind(", ");
			if (iPos) strLabel=strLabel.Left(iPos);	// strip off the country part
			msg2.SetLabel(strLabel);				// so we just display the town
		}
		else
		{
			msg2.SetLabel(g_stSettings.m_szWeatherArea[i]);
		}
		g_graphicsContext.SendMessage(msg2);
	}

	CGUIMessage msgSet(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_SELECTLOCATION, m_iCurWeather, 0, NULL);
	g_graphicsContext.SendMessage(msgSet);  
}


void CGUIWindowWeather::Render()
{
	DWORD dwTimeElapsed = timeGetTime() - m_lRefreshTime;
	if(dwTimeElapsed >= (DWORD)(g_stSettings.m_iWeatherRefresh * 60000))
		RefreshMe(true);	//do an autoUpdate refresh

	CGUIWindow::Render();
}

bool CGUIWindowWeather::Download(const CStdString& strWeatherFile)
{
	CStdString			strURL;
	
	char c_units = g_stSettings.m_szWeatherFTemp[0];	//convert from temp units to metric/standard
	if(c_units == 'F')	//we'll convert the speed later depending on what thats set to
		c_units = 's';
	else
		c_units = 'm';

	strURL.Format("http://xoap.weather.com/weather/local/%s?cc=*&unit=%c&dayf=4&prod=xoap&par=%s&key=%s",
				g_stSettings.m_szWeatherArea[m_iCurWeather], c_units, PARTNER_ID, PARTNER_KEY);

	return m_httpGrabber.Download(strURL, strWeatherFile);
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
		return false;

	TiXmlElement *pRootElement = xmlDoc.RootElement();
	if (!pRootElement)
		return false;

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
		return true;	//we got a message so do display a second in refreshme()
	}

	// units (C or F and mph or km/h or m/s) 
	strcpy(szUnitTemp, g_stSettings.m_szWeatherFTemp);

	if(g_stSettings.m_szWeatherFSpeed[0] == 'M')
		strcpy(szUnitSpeed, "mph");
	else if(g_stSettings.m_szWeatherFSpeed[0] == 'K')
		strcpy(szUnitSpeed, "km/h");
	else
		strcpy(szUnitSpeed, "m/s");

	// location
	TiXmlElement *pElement = pRootElement->FirstChildElement("loc");
	if(pElement)
	{
		GetString(pElement, "dnam", m_szLocation[m_iCurWeather], "");
	}

	//current weather
	pElement = pRootElement->FirstChildElement("cc");
	if(pElement)
	{
		GetString(pElement, "lsup", m_szUpdated, "");

		GetString(pElement, "icon", iTmpStr, "");	//string cause i've seen it return N/A
		if(strcmp(iTmpStr, "N/A") == 0)
			sprintf(m_szNowIcon, "Q:\\weather\\128x128\\na.png");
		else
			sprintf(m_szNowIcon, "Q:\\weather\\128x128\\%s.png", iTmpStr);

		GetString(pElement, "t", m_szNowCond, "");			//current condition
		LocalizeOverview(m_szNowCond);
		SplitLongString(m_szNowCond, 8, 15);				//split to 2 lines if needed

		GetInteger(pElement, "tmp", iTmpInt);				//current temp
		sprintf(m_szNowTemp, "%i%c%s", iTmpInt, DEGREE_CHARACTER, szUnitTemp);	
		GetInteger(pElement, "flik", iTmpInt);				//current 'Feels Like'
		sprintf(m_szNowFeel, "%i%c%s", iTmpInt, DEGREE_CHARACTER, szUnitTemp);
		
		TiXmlElement *pNestElement = pElement->FirstChildElement("wind");	//current wind
		if(pNestElement)
		{
			GetInteger(pNestElement, "s", iTmpInt);			//current wind strength
			iTmpInt = ConvertSpeed(iTmpInt);				//convert speed if needed
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
					GetString(pDayTimeElement, "icon", iTmpStr, "");	//string cause i've seen it return N/A
					if(strcmp(iTmpStr, "N/A") == 0)
						sprintf(m_dfForcast[i].m_szIcon, "Q:\\weather\\64x64\\na.png");
					else
						sprintf(m_dfForcast[i].m_szIcon, "Q:\\weather\\64x64\\%s.png", iTmpStr);
			
					GetString(pDayTimeElement, "t", m_dfForcast[i].m_szOverview, "");
					LocalizeOverview(m_dfForcast[i].m_szOverview);
					SplitLongString(m_dfForcast[i].m_szOverview, 6, 15);
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
void CGUIWindowWeather::RefreshMe(bool autoUpdate)
{
	//message strings for refresh of images
	CGUIMessage msgDe(GUI_MSG_WINDOW_DEINIT,0,0);
	CGUIMessage msgRe(GUI_MSG_WINDOW_INIT,0,0,WINDOW_INVALID);
  CStdString strWeatherFile = "Z:\\curWeather.xml";

	CGUIDialogProgress*	pDlgProgress	= (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
	CGUIDialogOK* 		pDlgOK			= (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
	bool dlRes = false, ldRes = false;

	//progress dialog for download
	if(pDlgProgress && !autoUpdate) //dont display progress dialog on autoupdate or it crashes! :|
	{
		pDlgProgress->SetHeading(410);							//"Accessing Weather.com"
		pDlgProgress->SetLine(0, 411);							//"Getting Weather For:"
		pDlgProgress->SetLine(1, g_stSettings.m_szWeatherArea[m_iCurWeather]);	//Area code
		if(strlen(m_szLocation[m_iCurWeather]) > 1)							//got the location string yet?
			pDlgProgress->SetLine(2, m_szLocation[m_iCurWeather]);
		else
			pDlgProgress->SetLine(2, "");
		pDlgProgress->StartModal(GetID());
		pDlgProgress->Progress();
	}	

	//Do The Download
	dlRes = Download(strWeatherFile);		
	
	if(pDlgProgress && !autoUpdate)	//close progress dialog
		pDlgProgress->Close();

	if(dlRes)	//dont load if download failed
		ldRes = LoadWeather(strWeatherFile);	//parse

	//if the download or load failed, display an error message
	if((!dlRes || !ldRes) && pDlgOK && !autoUpdate) //this will probably crash on an autoupdate as well, but not tested
	{
		// show failed dialog...
		pDlgOK->SetHeading(412);	//"Unable to get weather data"
		pDlgOK->SetLine(0, "");
		pDlgOK->SetLine(1, L"");
		pDlgOK->SetLine(2, L"");
		pDlgOK->DoModal(GetID());
	} else if(dlRes && ldRes)	//download and load went ok so update
	{
		UpdateButtons();

		//Send the refresh messages
		OnMessage(msgDe);
		OnMessage(msgRe);
	}

	m_lRefreshTime = timeGetTime(); //update the refresh time (even if refresh failed to stop it trying over and over)
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


void CGUIWindowWeather::LoadLocalizedToken()
{
	TiXmlDocument xmlDoc;
	if ( !xmlDoc.LoadFile("Q:\\language\\english\\strings.xml") )
	{
		return;
	}
	TiXmlElement* pRootElement =xmlDoc.RootElement();
	CStdString strValue=pRootElement->Value();
	if (strValue!=CStdString("strings")) return;
	const TiXmlNode *pChild = pRootElement->FirstChild();
	while (pChild)
	{
		CStdString strValue=pChild->Value();
		if (strValue=="string")
		{
			const TiXmlNode *pChildID = pChild->FirstChild("id");
			const TiXmlNode *pChildText = pChild->FirstChild("value");
			DWORD dwID=atoi(pChildID->FirstChild()->Value());
			if (LOCALIZED_TOKEN_FIRSTID <= dwID && dwID <= LOCALIZED_TOKEN_LASTID)
			{
				WCHAR wszText[1024];
				swprintf(wszText,L"%S", pChildText->FirstChild()->Value() );
				if (wcslen(wszText) > 0)
				{
					wstring strText=wszText;
					m_localizedTokens.insert(std::pair<wstring,DWORD>(strText, dwID));
				}

			}
		}
		pChild=pChild->NextSibling();
	}
}
void CGUIWindowWeather::LocalizeOverviewToken(char *szToken)
{
	CStdString strLocStr="";
	WCHAR wszText[1024];
	swprintf(wszText,L"%S", szToken );
	if (wcslen(wszText) > 0)
	{
		wstring strText=wszText;
		ilocalizedTokens i;
		i = m_localizedTokens.find(strText);
		if (i!=m_localizedTokens.end())
		{
			strLocStr = g_localizeStrings.Get(i->second);
		}
	}
	if(strLocStr == "")
		strLocStr = szToken;	//if not found, let fallback
	strLocStr += " "; 
	strcpy(szToken, strLocStr.GetBuffer(strLocStr.GetLength()));
}

void CGUIWindowWeather::LocalizeOverview(char *szStr)
{
	char loc[256];
	char szToken[256];
	int  intOffset = 0;
	char *pnt = NULL;
	memset(loc, '\0', sizeof(loc));

	while((pnt = strstr(szStr+intOffset, " ")) != NULL)
	{
		//get the length of this token (everything before pnt)
		int iTokenLen = (int)(strlen(szStr) - strlen(pnt) - intOffset);
		strncpy(szToken, szStr+intOffset, iTokenLen);	//extract the token
		szToken[iTokenLen] = '\0';						//stick an end on it
		LocalizeOverviewToken(szToken);					//localize
		strcpy(loc+strlen(loc), szToken);				//add it to the end of loc
		intOffset += iTokenLen + 1;						//update offset for next strstr search
	}
	strncpy(szToken, szStr+intOffset, strlen(szStr)-intOffset);	//last word, copy the rest of the string
	szToken[strlen(szStr)-intOffset] = '\0';					//stick an end on it
	LocalizeOverviewToken(szToken);								//localize
	strcpy(loc+strlen(loc), szToken);							//add it to the end of loc
	strcpy(szStr, loc);											//copy loc over the original input string
}

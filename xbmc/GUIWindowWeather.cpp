#include "stdafx.h"
#include "GUIWindowWeather.h"
#include "GUISpinControl.h"
#include "Util.h"
#include "Utils/Weather.h"

#define SPEED_KMH 0
#define SPEED_MPH 1
#define SPEED_MPS 2

#define DEGREES_C 0
#define DEGREES_F 1

#define CONTROL_BTNREFRESH		2
#define CONTROL_SELECTLOCATION	3
#define CONTROL_LABELUPDATED	11
#define CONTROL_IMAGELOGO		101

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

CGUIWindowWeather::CGUIWindowWeather(void)
:CGUIWindow(0)
{
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
		case GUI_MSG_CLICKED:
		{
			int iControl=message.GetSenderId();
			if(iControl == CONTROL_BTNREFRESH)
			{
				Refresh();	// Refresh clicked so do a complete update
			}
			else if(iControl == CONTROL_SELECTLOCATION)
			{
				CGUISpinControl *pTempSpin = (CGUISpinControl*)GetControl(iControl);
				m_iCurWeather = pTempSpin->GetValue();
				Refresh();
			}
		}
		break;
	}
	return CGUIWindow::OnMessage(message);
}


void CGUIWindowWeather::UpdateButtons()
{
	// disable refresh button if internet lookups are disabled
	if (g_guiSettings.GetBool("Network.EnableInternet"))
	{
		CONTROL_ENABLE(CONTROL_BTNREFRESH);
	}
	else
	{
		CONTROL_DISABLE(CONTROL_BTNREFRESH);
	}

	SET_CONTROL_LABEL(CONTROL_BTNREFRESH, 184);			//Refresh

	SET_CONTROL_LABEL(WEATHER_LABEL_LOCATION, g_weatherManager.GetLocation(m_iCurWeather));
	SET_CONTROL_LABEL(CONTROL_LABELUPDATED, g_weatherManager.GetLastUpdateTime());

	for (DWORD dwID=WEATHER_LABEL_CURRENT_COND; dwID <= WEATHER_LABEL_CURRENT_HUMI; dwID++)
	{
		SET_CONTROL_LABEL(dwID, g_weatherManager.GetLabel(dwID));
	}
	CGUIImage *pImage = (CGUIImage *)GetControl(WEATHER_IMAGE_CURRENT_ICON);
	if (pImage) pImage->SetFileName(g_weatherManager.GetCurrentIcon());

	//static labels
	SET_CONTROL_LABEL(CONTROL_STATICTEMP, 401);		//Temperature
	SET_CONTROL_LABEL(CONTROL_STATICFEEL, 402);		//Feels Like
	SET_CONTROL_LABEL(CONTROL_STATICUVID, 403);		//UV Index
	SET_CONTROL_LABEL(CONTROL_STATICWIND, 404);		//Wind
	SET_CONTROL_LABEL(CONTROL_STATICDEWP, 405);		//Dew Point
	SET_CONTROL_LABEL(CONTROL_STATICHUMI, 406);		//Humidity
	
	for(int i=0; i<NUM_DAYS; i++)
	{
		SET_CONTROL_LABEL(CONTROL_LABELD0DAY+(i*10), g_weatherManager.m_dfForcast[i].m_szDay);
		SET_CONTROL_LABEL(CONTROL_LABELD0HI+(i*10), g_weatherManager.m_dfForcast[i].m_szHigh);
		SET_CONTROL_LABEL(CONTROL_LABELD0LOW+(i*10), g_weatherManager.m_dfForcast[i].m_szLow);
		SET_CONTROL_LABEL(CONTROL_LABELD0GEN+(i*10), g_weatherManager.m_dfForcast[i].m_szOverview);
		pImage = (CGUIImage *)GetControl(CONTROL_IMAGED0IMG+(i*10));
		if (pImage) pImage->SetFileName(g_weatherManager.m_dfForcast[i].m_szIcon);
	}

	CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SELECTLOCATION, 0, 0, NULL);
	g_graphicsContext.SendMessage(msg);
	CGUIMessage msg2(GUI_MSG_LABEL_ADD, GetID(), CONTROL_SELECTLOCATION, 0, 0);
	for(int i=0; i<MAX_LOCATION; i++)
	{
		char *szLocation = g_weatherManager.GetLocation(m_iCurWeather);
		if(strlen(szLocation) > 1) //got the location string yet?
		{
			CStdString strLabel = szLocation;
			int iPos = strLabel.ReverseFind(", ");
			if (iPos) strLabel=strLabel.Left(iPos);	// strip off the country part
			msg2.SetLabel(strLabel);				// so we just display the town
		}
		else
		{
			CStdString strSetting;
			strSetting.Format("Weather.AreaCode%i", i+1);
			msg2.SetLabel(g_guiSettings.GetString(strSetting));
		}
		g_graphicsContext.SendMessage(msg2);
	}

	CGUIMessage msgSet(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_SELECTLOCATION, m_iCurWeather, 0, NULL);
	g_graphicsContext.SendMessage(msgSet);
}


void CGUIWindowWeather::Render()
{
	CGUIWindow::Render();
}

//Do a complete download, parse and update
void CGUIWindowWeather::Refresh()
{
	// quietly return if Internet lookups are disabled
	if (!g_guiSettings.GetBool("Network.EnableInternet")) return;

	// update our controls
	UpdateButtons();

	static bool bOnce=false;
	if (!bOnce)
	{
		bOnce=true;
		g_weatherManager.Refresh(m_iCurWeather);
	}
}

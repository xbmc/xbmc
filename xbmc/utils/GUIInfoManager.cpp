#include "stdafx.h"
#include "GUIInfoManager.h"
#include "log.h"
#include "Weather.h"
#include "LocalizeStrings.h"
#include "../GUISettings.h"

CGUIInfoManager g_infoManager;

CGUIInfoManager::CGUIInfoManager(void)
{
}

CGUIInfoManager::~CGUIInfoManager(void)
{
}

/// \brief Obtains the label for the label control from whichever subsystem is needed
wstring CGUIInfoManager::GetLabel(const CStdString &strInfo)
{
	CStdString strLabel;
	CStdString strTest = strInfo;
	strTest = strTest.ToLower();
	if (strTest == "weather.conditions")
		strLabel = g_weatherManager.GetLabel(WEATHER_LABEL_CURRENT_COND);
	else if (strTest == "weather.temperature")
		strLabel = g_weatherManager.GetLabel(WEATHER_LABEL_CURRENT_COND);
	else if (strTest == "system.time")
		return GetTime();
	else if (strTest == "system.date")
		return GetDate();
	// convert our CStdString to a wstring (which the label expects!)
	WCHAR szLabel[256];
	swprintf(szLabel,L"%S", strLabel.c_str() );
	wstring strReturn = szLabel;
	return strReturn;
}

/// \brief Obtains the filename of the image to show from whichever subsystem is needed
CStdString CGUIInfoManager::GetImage(const CStdString &strInfo)
{
	if (strInfo == "Weather.Conditions")
		return g_weatherManager.GetCurrentIcon();
	return "";
}

wstring CGUIInfoManager::GetDate()
{
	WCHAR szText[128];
	SYSTEMTIME time;
	GetLocalTime(&time);

	const WCHAR* day;
	switch (time.wDayOfWeek)
	{
    case 1 :	day = g_localizeStrings.Get(11).c_str();	break;
		case 2 :	day = g_localizeStrings.Get(12).c_str();	break;
		case 3 :	day = g_localizeStrings.Get(13).c_str();	break;
		case 4 :	day = g_localizeStrings.Get(14).c_str();	break;
		case 5 :	day = g_localizeStrings.Get(15).c_str();	break;
		case 6 :	day = g_localizeStrings.Get(16).c_str();	break;
		default:	day = g_localizeStrings.Get(17).c_str();	break;
	}

	const WCHAR* month;
	switch (time.wMonth)
	{
		case 1 :	month= g_localizeStrings.Get(21).c_str();	break;
		case 2 :	month= g_localizeStrings.Get(22).c_str();	break;
		case 3 :	month= g_localizeStrings.Get(23).c_str();	break;
		case 4 :	month= g_localizeStrings.Get(24).c_str();	break;
		case 5 :	month= g_localizeStrings.Get(25).c_str();	break;
		case 6 :	month= g_localizeStrings.Get(26).c_str();	break;
		case 7 :	month= g_localizeStrings.Get(27).c_str();	break;
		case 8 :	month= g_localizeStrings.Get(28).c_str();	break;
		case 9 :	month= g_localizeStrings.Get(29).c_str();	break;
		case 10:	month= g_localizeStrings.Get(30).c_str();	break;
		case 11:	month= g_localizeStrings.Get(31).c_str();	break;
		default:	month= g_localizeStrings.Get(32).c_str();	break;
	}

	if (day && month)
	{
		if (g_guiSettings.GetBool("LookAndFeel.SwapMonthAndDay"))
			swprintf(szText,L"%s, %d %s", day, time.wDay, month);
		else
			swprintf(szText,L"%s, %s %d", day, month, time.wDay);
	}
	else
		swprintf(szText,L"no date");
	wstring strText = szText;
	return strText;
}

wstring CGUIInfoManager::GetTime()
{
	WCHAR szText[128];
	SYSTEMTIME time;
	GetLocalTime(&time);

	INT iHour = time.wHour;

	if (g_guiSettings.GetBool("LookAndFeel.Clock12Hour"))
	{
		if (iHour>11)
		{
			iHour-=(12*(iHour>12));
			swprintf(szText,L"%2d:%02d PM", iHour, time.wMinute);
		}
		else
		{
			iHour+=(12*(iHour<1));
			swprintf(szText,L"%2d:%02d AM", iHour, time.wMinute);
		}
	}
	else
		swprintf(szText,L"%02d:%02d", iHour, time.wMinute);
	wstring strText = szText;
	return strText;
}

#include "stdafx.h"
#include "guiwindowhome.h"
#include "localizestrings.h"
#include "texturemanager.h"
#include "xbox/iosupport.h"
#include "xbox/undocumented.h"
#include "xbox/xkutils.h"
#include "settings.h"
#include "sectionloader.h"
#include "util.h"
#include "application.h"
#include "Credits.h"

#define CONTROL_BTN_SHUTDOWN		10
#define CONTROL_BTN_DASHBOARD		11
#define CONTROL_BTN_REBOOT			12
#define CONTROL_BTN_CREDITS			13
#define CONTROL_BTN_ONLINE			14


CGUIWindowHome::CGUIWindowHome(void) : CGUIWindow(0)
{
	m_iLastControl=-1;
	m_iLastMenuOption=-1;

	ON_CLICK_MESSAGE(CONTROL_BTN_SHUTDOWN,	CGUIWindowHome, OnClickShutdown);	
	ON_CLICK_MESSAGE(CONTROL_BTN_DASHBOARD,	CGUIWindowHome, OnClickDashboard);	
	ON_CLICK_MESSAGE(CONTROL_BTN_REBOOT,	CGUIWindowHome, OnClickReboot);	
	ON_CLICK_MESSAGE(CONTROL_BTN_CREDITS,	CGUIWindowHome, OnClickCredits);	
	ON_CLICK_MESSAGE(CONTROL_BTN_ONLINE,	CGUIWindowHome, OnClickOnlineGaming);	
}

CGUIWindowHome::~CGUIWindowHome(void)
{
}


bool CGUIWindowHome::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
	{
		case GUI_MSG_WINDOW_INIT:
		{
			int iFocusControl=m_iLastControl;
			CGUIWindow::OnMessage(message);
      
			// make controls 101-120 invisible...
			for (int iControl=102; iControl < 120; iControl++)
			{
				SET_CONTROL_HIDDEN(GetID(), iControl);
			}

			if (m_iLastMenuOption>0)
			{
				SET_CONTROL_VISIBLE(GetID(), m_iLastMenuOption+100);
			}

			if (iFocusControl<0)
			{
				iFocusControl=GetFocusedControl();
				m_iLastControl=iFocusControl;
			}

			SET_CONTROL_FOCUS(GetID(), iFocusControl, 0);
			return true;
		}

		case GUI_MSG_SETFOCUS:
		{
			int iControl = message.GetControlId();
			m_iLastControl=iControl;
			if (iControl>=2 && iControl <=9)
			{
				m_iLastMenuOption = m_iLastControl;

				// make controls 101-120 invisible...
				for (int i=102; i < 120; i++)
				{
						SET_CONTROL_HIDDEN(GetID(), i);
				}
		
				SET_CONTROL_VISIBLE(GetID(), iControl+100);
			    break;
			}
		}

		case GUI_MSG_CLICKED:
		{
			m_iLastControl = message.GetSenderId();
			break;
		}
	}
	return CGUIWindow::OnMessage(message);
}

void CGUIWindowHome::OnClickShutdown(CGUIMessage& aMessage)
{
	g_applicationMessenger.Shutdown();
}

void CGUIWindowHome::OnClickDashboard(CGUIMessage& aMessage)
{
	CUtil::RunXBE(g_stSettings.szDashboard);
}

void CGUIWindowHome::OnClickReboot(CGUIMessage& aMessage)
{
  g_applicationMessenger.Reset();
}

void CGUIWindowHome::OnClickCredits(CGUIMessage& aMessage)
{
	RunCredits();
}

void CGUIWindowHome::OnClickOnlineGaming(CGUIMessage& aMessage)
{
	m_gWindowManager.ActivateWindow( WINDOW_BUDDIES );
}

void CGUIWindowHome::OnAction(const CAction &action)
{
	CGUIWindow::OnAction(action);
}

void CGUIWindowHome::Render()
{
  WCHAR szText[128];

  SYSTEMTIME time;
	GetLocalTime(&time);

  GetDate(szText,&time);
	
	SET_CONTROL_LABEL(GetID(), 200,szText);
	SET_CONTROL_LABEL(GetID(), 201,szText);

	GetTime(szText,&time);

	SET_CONTROL_LABEL(GetID(), 201,szText);

  CGUIWindow::Render();
}


VOID CGUIWindowHome::GetDate(WCHAR* wszDate, LPSYSTEMTIME pTime)
{
	if (!pTime) return;
	if (!wszDate) return;
	const WCHAR* day;
	switch (pTime->wDayOfWeek)
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
	switch (pTime->wMonth)
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
		swprintf(wszDate,L"%s, %s %d", day, month, pTime->wDay);
	else
		swprintf(wszDate,L"no date");
}

VOID CGUIWindowHome::GetTime(WCHAR* szTime, LPSYSTEMTIME pTime)
{
	if (!szTime) return;
	if (!pTime) return;
	INT iHour = pTime->wHour;
	swprintf(szTime,L"%02d:%02d", iHour, pTime->wMinute);
}
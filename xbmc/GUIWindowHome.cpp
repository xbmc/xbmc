
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

CGUIWindowHome::CGUIWindowHome(void)
:CGUIWindow(0)
{
  m_iLastControl=-1;
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
	    if (iFocusControl<0)
      {
        iFocusControl=GetFocusedControl();
		    m_iLastControl=iFocusControl;
	    }

	    SET_CONTROL_FOCUS(GetID(), iFocusControl);

			return true;
    }

    case GUI_MSG_SETFOCUS:
    {
      int iControl=message.GetControlId();
      m_iLastControl=iControl;
      if (iControl>=2 && iControl <=8)
      {
        // make controls 101-120 invisible...
        for (int i=102; i < 120; i++)
        {
					SET_CONTROL_HIDDEN(GetID(), i);
        }
				SET_CONTROL_VISIBLE(GetID(), iControl+100);
      }
    }
    break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
      m_iLastControl=iControl;
      switch (iControl)
      {

        case 10: // powerdown
        {
					g_application.Stop();
          XKUtils::XBOXPowerOff();
        }
        break;

        case 11: // execute dashboard
        {
          char* szBackslash = strrchr(g_stSettings.szDashboard,'\\');
		      *szBackslash=0x00;
		      char* szXbe = &szBackslash[1];

		      char* szColon = strrchr(g_stSettings.szDashboard,':');
		      *szColon=0x00;
		      char* szDrive = g_stSettings.szDashboard;
		      char* szDirectory = &szColon[1];
          
          char szDevicePath[1024];
          char szXbePath[1024];
          CIoSupport helper;
		      helper.GetPartition( (LPCSTR) szDrive, szDevicePath);

		      strcat(szDevicePath,szDirectory);
		      wsprintf(szXbePath,"d:\\%s",szXbe);

					g_application.Stop();

          CUtil::LaunchXbe(szDevicePath,szXbePath,NULL);
        }
        break;
      
				case 12:
				{
					g_application.Stop();
					XKUtils::XBOXPowerCycle();
				}
      }
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
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

#include "stdafx.h"
#include "GUIWindowVideoOverlay.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "util.h"
#include "stdstring.h"
#include "application.h"
#include "videodatabase.h"
#include "utils/imdb.h"

#define CONTROL_PLAYTIME		2
#define CONTROL_PLAY_LOGO   3
#define CONTROL_PAUSE_LOGO  4
#define CONTROL_INFO			  5
#define CONTROL_BIG_PLAYTIME 6
#define CONTROL_FF_LOGO  7
#define CONTROL_RW_LOGO  8


CGUIWindowVideoOverlay::CGUIWindowVideoOverlay()
:CGUIWindow(0)
{
}

CGUIWindowVideoOverlay::~CGUIWindowVideoOverlay()
{
}


void CGUIWindowVideoOverlay::OnAction(const CAction &action)
{
  CGUIWindow::OnAction(action);
}

bool CGUIWindowVideoOverlay::OnMessage(CGUIMessage& message)
{
  return CGUIWindow::OnMessage(message);
}


void CGUIWindowVideoOverlay::Render()
{
	if (!g_application.m_pPlayer) return;
	if (!g_application.m_pPlayer->HasVideo()) return;
	
  __int64 lPTS=10*g_application.m_pPlayer->GetTime();
  int hh = (int)(lPTS / 36000) % 100;
  int mm = (int)((lPTS / 600) % 60);
  int ss = (int)((lPTS /  10) % 60);
  int iSpeed=g_application.GetPlaySpeed();
  if (hh==0 && mm==0 && ss<5)
  {
    if (iSpeed < 1)
    {
      iSpeed=1;
      g_application.SetPlaySpeed(iSpeed);
      g_application.m_pPlayer->SeekTime(0);
    }
  }

	
	char szTime[128];
	if (hh>=1)
		sprintf(szTime,"%02.2i:%02.2i:%02.2i",hh,mm,ss);
	else
		sprintf(szTime,"%02.2i:%02.2i",mm,ss);
	{
		CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_BIG_PLAYTIME); 
		msg.SetLabel(szTime); 
		OnMessage(msg); 
	}

  if (iSpeed !=1)
  {
    char szTmp[32];
    sprintf(szTmp,"(%ix)", iSpeed);
    strcat(szTime,szTmp);
  }
  {
		CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_PLAYTIME); 
		msg.SetLabel(szTime); 
		OnMessage(msg); 
	}

	HideControl(CONTROL_PLAY_LOGO); 
  HideControl(CONTROL_PAUSE_LOGO); 
  HideControl( CONTROL_FF_LOGO); 
  HideControl( CONTROL_RW_LOGO); 
	if (g_application.m_pPlayer->IsPaused() )
	{
      ShowControl( CONTROL_PAUSE_LOGO); 
	}
	else
	{
    int iSpeed = g_application.GetPlaySpeed();
    if (iSpeed > 1)
    {
      ShowControl( CONTROL_FF_LOGO); 
    }
    else if (iSpeed < 0)
    {
      ShowControl( CONTROL_RW_LOGO); 
    }
    else
    {
		  ShowControl( CONTROL_PLAY_LOGO); 
    }
	}
	CGUIWindow::Render();
}


	void CGUIWindowVideoOverlay::SetCurrentFile(const CStdString& strFile)
	{
		bool bLoaded=false;
		if ( CUtil::IsVideo(strFile) )
		{
			CGUIMessage msg1(GUI_MSG_LABEL_RESET, GetID(), CONTROL_INFO); 
			OnMessage(msg1);

      CVideoDatabase dbs;
			CIMDBMovie movieDetails;
      bool bMovieInfoFound(false);
      dbs.Open();
      if (dbs.HasMovieInfo(strFile))
      {
        dbs.GetMovieInfo(strFile,movieDetails);
        bMovieInfoFound=true;
      }
      dbs.Close();
			if (bMovieInfoFound)
			{
        g_application.SetCurrentMovie(movieDetails);
				// title
				if ( movieDetails.m_strTitle.size() > 0)
				{
					CStdString strLabel=movieDetails.m_strTitle;
					bLoaded=true;
					CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
					msg1.SetLabel(strLabel );
					OnMessage(msg1);				
				}

				// genre
				if ( movieDetails.m_strGenre.size() > 0)
				{
					const WCHAR* wsGenre=g_localizeStrings.Get(174).c_str();
					WCHAR wsTmp[1024];
					swprintf(wsTmp,L"%s %S", wsGenre, movieDetails.m_strGenre.c_str());
					CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
					msg1.SetLabel(wsTmp );
					OnMessage(msg1);				
				}

				// year
				if ( movieDetails.m_iYear > 1900)
				{
					const WCHAR* wsYear=g_localizeStrings.Get(201).c_str();
					WCHAR wsTmp[1024];
					swprintf(wsTmp,L"%s: %i", wsYear, movieDetails.m_iYear);
					CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
					msg1.SetLabel(wsTmp );
					OnMessage(msg1);				
				}

				// director
				if ( movieDetails.m_strDirector.size() > 0)
				{
					const WCHAR* wsDirector=g_localizeStrings.Get(199).c_str();
					WCHAR wsTmp[1024];
					swprintf(wsTmp,L"%s: %S", wsDirector, movieDetails.m_strDirector.c_str() );
					CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
					msg1.SetLabel(wsTmp );
					OnMessage(msg1);		
				}
			}
			if (!bLoaded)
			{
				CGUIMessage msg1(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INFO); 
				msg1.SetLabel( CUtil::GetFileName(strFile) );
				OnMessage(msg1);
			}
		}
	}

  
void CGUIWindowVideoOverlay::ShowControl(int iControl)
{
  CGUIMessage msg(GUI_MSG_VISIBLE, GetID(), iControl); 
	OnMessage(msg); 
}

void CGUIWindowVideoOverlay::HideControl(int iControl)
{
  CGUIMessage msg(GUI_MSG_HIDDEN, GetID(), iControl); 
	OnMessage(msg); 
}
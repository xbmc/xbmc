
#include "GUIWindowFullScreen.h"
#include "settings.h"
#include "application.h"
#include "util.h"

#define LABEL_ROW1 10
#define LABEL_ROW2 11
#define LABEL_ROW3 12

CGUIWindowFullScreen::CGUIWindowFullScreen(void)
:CGUIWindow(0)
{
	m_bShowInfo=false;
	m_dwLastTime=0;
	m_fFPS=0;
	m_fFrameCounter=0.0f;
	m_dwFPSTime=timeGetTime();
}

CGUIWindowFullScreen::~CGUIWindowFullScreen(void)
{
}


void CGUIWindowFullScreen::OnAction(const CAction &action)
{
	if (action.wID == ACTION_ASPECT_RATIO)
	{
		m_bShowStatus=true;
		m_dwLastTime=timeGetTime();
		// zoom->stretch
		if (g_stSettings.m_bZoom)
		{
			g_stSettings.m_bZoom=false;
			g_stSettings.m_bStretch=true;
			g_settings.Save();
			return;
		}
		// stretch->normal
		if (g_stSettings.m_bStretch)
		{
			g_stSettings.m_bZoom=false;
			g_stSettings.m_bStretch=false;
			g_settings.Save();
			return;
		}
		// normal->zoom
		g_stSettings.m_bZoom=true;
		g_stSettings.m_bStretch=false;
		g_settings.Save();
		return;

	}
	switch (action.wID)
	{
		case ACTION_STEP_BACK:
			g_application.m_pPlayer->Seek(false,false);
		break;

		case ACTION_STEP_FORWARD:
			g_application.m_pPlayer->Seek(true,false);
		break;

		case ACTION_BIG_STEP_BACK:
			g_application.m_pPlayer->Seek(false,true);
		break;

		case ACTION_BIG_STEP_FORWARD:
			g_application.m_pPlayer->Seek(true,true);
		break;

		case ACTION_SHOW_OSD:
			g_application.m_pPlayer->ToggleOSD();
		break;
			
		case ACTION_SHOW_SUBTITLES:
			g_application.m_pPlayer->ToggleSubtitles();
		break;

		case ACTION_SHOW_CODEC:
			m_bShowInfo = !m_bShowInfo;
		break;

		case ACTION_NEXT_SUBTITLE:
			g_application.m_pPlayer->SwitchToNextLanguage();
		break;

		case ACTION_STOP:
			g_application.m_pPlayer->closefile();
		break;

		case ACTION_PAUSE:
			g_application.m_pPlayer->Pause();
		break;
	}
	CGUIWindow::OnAction(action);
}

bool CGUIWindowFullScreen::OnMessage(CGUIMessage& message)
{

	return CGUIWindow::OnMessage(message);
}

void CGUIWindowFullScreen::Render()
{
	m_fFrameCounter+=1.0f;
	FLOAT fTimeSpan=(float)(timeGetTime()-m_dwFPSTime);
	if (fTimeSpan >=1000.0f)
	{
		fTimeSpan/=1000.0f;
		m_fFPS=(m_fFrameCounter/fTimeSpan);
		m_dwFPSTime=timeGetTime();
		m_fFrameCounter=0;
	}
	if (!g_application.m_pPlayer) return;
	
	if (m_bShowStatus)
	{
		if ( (timeGetTime() - m_dwLastTime) >=5000)
		{
			m_bShowStatus=false;
			return;
		}
		CStdString strStatus;
		if (g_stSettings.m_bZoom) strStatus="Zoom";
		else if (g_stSettings.m_bStretch) strStatus="Stretch";
		else strStatus="Normal";

		if (g_stSettings.m_bSoften)
			strStatus += "  |  Soften";
		else
			strStatus += "  |  No Soften";

		RECT SrcRect;
		RECT DestRect;
		g_application.m_pPlayer->GetVideoRect(SrcRect, DestRect);
		CStdString strRects;
		float fAR;
		g_application.m_pPlayer->GetVideoAspectRatio(fAR);
		strRects.Format(" | (%i,%i)-(%i,%i)->(%i,%i)-(%i,%i) AR:%2.2f", 
											SrcRect.left,SrcRect.top,
											SrcRect.right,SrcRect.bottom,
											DestRect.left,DestRect.top,
											DestRect.right,DestRect.bottom, fAR);
		strStatus += strRects;

		CStdString strStatus2;
		int  iResolution=g_graphicsContext.GetVideoResolution();
		strStatus2.Format("%ix%i %s", resInfo[iResolution].iWidth, resInfo[iResolution].iHeight, resInfo[iResolution].strMode);

		{
			CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW1); 
			msg.SetLabel(strStatus); 
			OnMessage(msg);
		}
		{
			CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW2); 
			msg.SetLabel(strStatus2); 
			OnMessage(msg);
		}
		{
			CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW3); 
			msg.SetLabel(""); 
			OnMessage(msg);
		}
		CGUIWindow::Render();
		return;
	}
	//------------------------
	if (!m_bShowInfo) return;
	if (!g_application.m_pPlayer) return;
	// show audio codec info
	CStdString strAudio, strVideo, strGeneral;
	g_application.m_pPlayer->GetAudioInfo(strAudio);
	{	
		CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW1); 
		msg.SetLabel(strAudio); 
		OnMessage(msg);
	}
	// show video codec info
	g_application.m_pPlayer->GetVideoInfo(strVideo);
	{	
		CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW2); 
		msg.SetLabel(strVideo); 
		OnMessage(msg);
	}
	// show general info
	g_application.m_pPlayer->GetGeneralInfo(strGeneral);
	{	
		CStdString strGeneralFPS;
		strGeneralFPS.Format("fps:%02.2f %s", m_fFPS, strGeneral.c_str() );
		CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW3); 
		msg.SetLabel(strGeneralFPS); 
		OnMessage(msg);
	}
	CGUIWindow::Render();
}

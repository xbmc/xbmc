
#include "stdafx.h"
#include "GUIWindowSettingsScreenCalibration.h"
#include "settings.h"
#include "stdstring.h"
#include "GUIImage.h"
#include "application.h"
#include "util.h"

#define CONTROL_LABEL_ROW1		2
#define CONTROL_LABEL_ROW2		3
#define CONTROL_TOP_LEFT		8
#define CONTROL_BOTTOM_RIGHT	9
#define CONTROL_SUBTITLES		10
#define CONTROL_PIXEL_RATIO		11
#define CONTROL_VIDEO			20
#define CONTROL_OSD				12



CGUIWindowSettingsScreenCalibration::CGUIWindowSettingsScreenCalibration(void)
:CGUIWindow(0)
{
}

CGUIWindowSettingsScreenCalibration::~CGUIWindowSettingsScreenCalibration(void)
{
}


void CGUIWindowSettingsScreenCalibration::OnAction(const CAction &action)
{
	if (timeGetTime()-m_dwLastTime > 500)
	{
		m_iSpeed=1;
		m_iCountU=0;
		m_iCountD=0;
		m_iCountL=0;
		m_iCountR=0;
	}
	m_dwLastTime=timeGetTime();

	int x,y;
	if (m_iControl == CONTROL_TOP_LEFT)
	{
		x = g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.left;
		y = g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.top;
	}
	else if (m_iControl == CONTROL_BOTTOM_RIGHT)
	{
		x = g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.width+g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.left;
		y = g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.height+g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.top;
	}
	else if (m_iControl == CONTROL_SUBTITLES)
	{
		x = 0;
		y = g_settings.m_ResInfo[m_Res[m_iCurRes]].iSubtitles;
	}
	else if (m_iControl == CONTROL_OSD)
	{
		x = 0;
		y = (g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight + g_settings.m_ResInfo[m_Res[m_iCurRes]].iOSDYOffset);
    
	}
	else // (m_iControl == CONTROL_PIXEL_RATIO)
	{
		y = 256;
		x = (int)(256.0f/g_settings.m_ResInfo[m_Res[m_iCurRes]].fPixelRatio);
	}

	if (m_iSpeed>10) m_iSpeed=10; // Speed limit for accellerated cursors

	switch (action.wID)
	{
		case ACTION_PREVIOUS_MENU:
		{
			m_gWindowManager.PreviousWindow();
			return;
		}
		break;
		case ACTION_MOVE_LEFT:
		{
			if (m_iCountL==0) m_iSpeed=1;
			x-=m_iSpeed;
			m_iCountL++;
			if (m_iCountL > 5 && m_iSpeed < 10)
			{
				m_iSpeed+=1;
				m_iCountL=1;
			}
			m_iCountU=0;
			m_iCountD=0;
			m_iCountR=0;
		}
		break;

		case ACTION_MOVE_RIGHT:
		{
			if (m_iCountR==0) m_iSpeed=1;
			x+=m_iSpeed;
			m_iCountR++;
			if (m_iCountR > 5 && m_iSpeed < 10)
			{
				m_iSpeed+=1;
				m_iCountR=1;
			}
			m_iCountU=0;
			m_iCountD=0;
			m_iCountL=0;
		}
		break;

		case ACTION_MOVE_UP:
		{
			if (m_iCountU==0) m_iSpeed=1;
			y-=m_iSpeed;
			m_iCountU++;
			if (m_iCountU > 5 && m_iSpeed < 10)
			{
				m_iSpeed+=1;
				m_iCountU=1;
			}
			m_iCountD=0;
			m_iCountL=0;
			m_iCountR=0;
		}
		break;

		case ACTION_MOVE_DOWN:
		{
			if (m_iCountD==0) m_iSpeed=1;
			y+=m_iSpeed;
			m_iCountD++;
			if (m_iCountD > 5 && m_iSpeed < 10)
			{
				m_iSpeed+=1;
				m_iCountD=1;
			}
			m_iCountU=0;
			m_iCountL=0;
			m_iCountR=0;
		}
		break;

		case ACTION_CALIBRATE_SWAP_ARROWS:
			m_iControl++;
			if (m_iControl > CONTROL_OSD)
				m_iControl = CONTROL_TOP_LEFT;
			m_iSpeed=1;
			m_iCountU=0;
			m_iCountD=0;
			m_iCountL=0;
			m_iCountR=0;
			return;
		break;

		case ACTION_CALIBRATE_RESET:
			g_graphicsContext.ResetScreenParameters(m_Res[m_iCurRes]);
			m_iSpeed=1;
			m_iCountU=0;
			m_iCountD=0;
			m_iCountL=0;
			m_iCountR=0;
      g_application.m_guiWindowOSD.ResetAllControls();

			return;
		break;

		case ACTION_CHANGE_RESOLUTION:
			// choose the next resolution in our list
			m_iCurRes++;
			if (m_iCurRes == m_Res.size())
				m_iCurRes = 0;
      Sleep(1000);
			g_graphicsContext.SetGUIResolution(m_Res[m_iCurRes]);
      
      g_application.m_guiWindowOSD.ResetAllControls();
			return;
		break;

		case ACTION_ANALOG_MOVE:
			x += (int)(2*action.fAmount1);
			y -= (int)(2*action.fAmount2);
		break;
	}
	// do the movement
	switch (m_iControl)
	{
		case CONTROL_TOP_LEFT:
			if (x < 0) x=0;
			if (y < 0) y=0;
			if (x > 128) x=128;
			if (y > 128) y=128;
			g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.width += g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.left-x;
			g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.height+= g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.top-y;
			g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.left=x; 
			g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.top=y;
		break;
		case CONTROL_BOTTOM_RIGHT:
			if (x > g_settings.m_ResInfo[m_Res[m_iCurRes]].iWidth) x=g_settings.m_ResInfo[m_Res[m_iCurRes]].iWidth;
			if (y > g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight) y=g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight;
			if (x < g_settings.m_ResInfo[m_Res[m_iCurRes]].iWidth-128) x=g_settings.m_ResInfo[m_Res[m_iCurRes]].iWidth-128;
			if (y < g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight-128) y=g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight-128;
			g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.width=x-g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.left;
			g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.height=y-g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.top;
		break;
		case CONTROL_SUBTITLES:
			if (y > g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight) y=g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight;
			if (y < g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight-128) y=g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight-128;
			g_settings.m_ResInfo[m_Res[m_iCurRes]].iSubtitles=y;
		break;
		case CONTROL_OSD:
			g_settings.m_ResInfo[m_Res[m_iCurRes]].iOSDYOffset = (y - g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight);
      
      g_application.m_guiWindowOSD.ResetAllControls();
		break;
		case CONTROL_PIXEL_RATIO:
			float fPixelRatio = (float)y/x;
			if (fPixelRatio > 2.0f) fPixelRatio = 2.0f;
			if (fPixelRatio < 0.5f) fPixelRatio = 0.5f;
			g_settings.m_ResInfo[m_Res[m_iCurRes]].fPixelRatio=fPixelRatio;
		break;
	}
	CGUIWindow::OnAction(action);
}

bool CGUIWindowSettingsScreenCalibration::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_WINDOW_DEINIT:
		{
      CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0,0,0,NULL);
	    g_application.m_guiWindowOSD.OnMessage(msg);	// Send an init msg to the OSD
			g_application.EnableOverlay();
			g_settings.Save();
			g_graphicsContext.SetCalibrating(false);
			// reset our screen resolution to what it was initially
			g_graphicsContext.SetGUIResolution(g_stSettings.m_ScreenResolution);
			// Inform the player so we can update the resolution
			if (g_application.m_pPlayer)
				g_application.m_pPlayer->Update();	
		}
		break;

		case GUI_MSG_WINDOW_INIT:
		{
			CGUIWindow::OnMessage(message);
			g_application.DisableOverlay();
			m_iControl=CONTROL_TOP_LEFT;
			m_iSpeed=1;
			m_iCountU=0;
			m_iCountD=0;
			m_iCountL=0;
			m_iCountR=0;
			g_graphicsContext.SetCalibrating(true);
			// Inform the player so we can update the resolution
			if (g_application.IsPlayingVideo())
				g_application.m_pPlayer->Update();
			// Get the allowable resolutions that we can calibrate...
			m_Res.clear();
			if (g_application.IsPlayingVideo())
			{	// don't allow resolution switching if we are playing a video
				m_iCurRes = 0;
				m_Res.push_back(g_graphicsContext.GetVideoResolution());
			}
			else
			{
				g_graphicsContext.GetAllowedResolutions(m_Res, true);
				// find our starting resolution
				for (UINT i=0; i<m_Res.size(); i++)
				{
					if (m_Res[i] == g_graphicsContext.GetVideoResolution())
						m_iCurRes = i;
				}
			}
			// disable the UI calibration for our controls...
			CGUIImage *pControl=(CGUIImage*)GetControl(CONTROL_BOTTOM_RIGHT);
      if (pControl)
      {
			  pControl->EnableCalibration(false);
			  pControl=(CGUIImage*)GetControl(CONTROL_TOP_LEFT);
			  pControl->EnableCalibration(false);
			  pControl=(CGUIImage*)GetControl(CONTROL_SUBTITLES);
			  pControl->EnableCalibration(false);
			  pControl=(CGUIImage*)GetControl(CONTROL_PIXEL_RATIO);
			  pControl->EnableCalibration(false);
			  pControl=(CGUIImage*)GetControl(CONTROL_OSD);
			  pControl->EnableCalibration(false);
			  m_fPixelRatioBoxHeight=(float)pControl->GetHeight();
      }

      CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0,0,0,NULL);
	    g_application.m_guiWindowOSD.OnMessage(msg);	// Send an init msg to the OSD
			return true;
		}
		break;
	}
 return CGUIWindow::OnMessage(message);
}

void CGUIWindowSettingsScreenCalibration::Render()
{
	// hide all our controls
	CGUIImage* pControl=(CGUIImage*)GetControl(CONTROL_BOTTOM_RIGHT);
  if (pControl)
	  pControl->SetVisible(false);
	pControl=(CGUIImage*)GetControl(CONTROL_SUBTITLES);
  if (pControl)
	  pControl->SetVisible(false);
	pControl=(CGUIImage*)GetControl(CONTROL_TOP_LEFT);
  if (pControl)
	  pControl->SetVisible(false);
	pControl=(CGUIImage*)GetControl(CONTROL_PIXEL_RATIO);
  if (pControl)
	  pControl->SetVisible(false);
  pControl=(CGUIImage*)GetControl(CONTROL_OSD);
  if (pControl)
	  pControl->SetVisible(false);

	int iXOff,iYOff;
	CStdString strStatus;
	switch (m_iControl)
	{
		case CONTROL_TOP_LEFT:
		{
			iXOff = g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.left;
			iYOff = g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.top;
			pControl=(CGUIImage*)GetControl(CONTROL_TOP_LEFT);
      if (pControl)
      {
			  pControl->SetVisible(true);
			  pControl->SetPosition(iXOff, iYOff);
      }
			CStdString strMode;
			CUtil::Unicode2Ansi(g_localizeStrings.Get(272).c_str(),strMode);
			strStatus.Format("%s (%i,%i)",strMode,iXOff,iYOff);
			SET_CONTROL_LABEL(GetID(), CONTROL_LABEL_ROW2,	276);
		}
		break;
		case CONTROL_BOTTOM_RIGHT:
		{
			iXOff = g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.left;
			iYOff = g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.top;
			iXOff += g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.width;
			iYOff += g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.height;
			pControl=(CGUIImage*)GetControl(CONTROL_BOTTOM_RIGHT);
			if (pControl) 
      {
        pControl->SetVisible(true);
			  int iTextureWidth = pControl->GetTextureWidth();
			  int iTextureHeight = pControl->GetTextureHeight();
			  pControl->SetPosition(iXOff-iTextureWidth,iYOff-iTextureHeight);
			  int iXOff1 = g_settings.m_ResInfo[m_Res[m_iCurRes]].iWidth - iXOff;
			  int iYOff1 = g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight - iYOff;
			  CStdString strMode;
			  CUtil::Unicode2Ansi(g_localizeStrings.Get(273).c_str(),strMode);
			  strStatus.Format("%s (%i,%i)",strMode,iXOff1,iYOff1);
			  SET_CONTROL_LABEL(GetID(), CONTROL_LABEL_ROW2,	276);
      }
		}
		break;
		case CONTROL_SUBTITLES:
		{
			iXOff = g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.left;
			iYOff = g_settings.m_ResInfo[m_Res[m_iCurRes]].iSubtitles;

			int iScreenWidth = g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.width;

			pControl=(CGUIImage*)GetControl(CONTROL_SUBTITLES);
			if (pControl) 
      {
        pControl->SetVisible(true);
			  int iTextureWidth = pControl->GetTextureWidth();
			  int iTextureHeight = pControl->GetTextureHeight();

			  pControl->SetPosition(iXOff+(iScreenWidth-iTextureWidth)/2, iYOff-iTextureHeight);
			  CStdString strMode;
			  CUtil::Unicode2Ansi(g_localizeStrings.Get(274).c_str(),strMode);
			  strStatus.Format("%s (%i)",strMode,iYOff);
			  SET_CONTROL_LABEL(GetID(), CONTROL_LABEL_ROW2,	277);
      }
		}
		break;
		case CONTROL_PIXEL_RATIO:
		{
			float fSqrtRatio = sqrt(g_settings.m_ResInfo[m_Res[m_iCurRes]].fPixelRatio);
			pControl=(CGUIImage*)GetControl(CONTROL_PIXEL_RATIO);
			if (pControl) 
      {
        pControl->SetVisible(true);
			  int iControlHeight = (int)(m_fPixelRatioBoxHeight*fSqrtRatio);
			  int iControlWidth = (int)(m_fPixelRatioBoxHeight / fSqrtRatio);
			  pControl->SetWidth(iControlWidth);
			  pControl->SetHeight(iControlHeight);
			  iXOff = g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.left;
			  iYOff = g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.top;
			  int iScreenWidth = g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.width;
			  int iScreenHeight = g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.height;
			  pControl->SetPosition(iXOff + (iScreenWidth-iControlWidth)/2,iYOff + (iScreenHeight-iControlHeight)/2);
			  CStdString strMode;
			  CUtil::Unicode2Ansi(g_localizeStrings.Get(275).c_str(),strMode);
			  strStatus.Format("%s (%5.3f)",strMode,g_settings.m_ResInfo[m_Res[m_iCurRes]].fPixelRatio);
			  SET_CONTROL_LABEL(GetID(), CONTROL_LABEL_ROW2,	278);
      }
		}
		break;

		case CONTROL_OSD:
		{
      iXOff = g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.left;
			iYOff = g_settings.m_ResInfo[m_Res[m_iCurRes]].iSubtitles;
			iYOff = (g_settings.m_ResInfo[m_Res[m_iCurRes]].iHeight + g_settings.m_ResInfo[m_Res[m_iCurRes]].iOSDYOffset);

			int iScreenWidth = g_settings.m_ResInfo[m_Res[m_iCurRes]].Overscan.width;

			pControl=(CGUIImage*)GetControl(CONTROL_OSD);
			if (pControl) 
			{
			  //pControl->SetVisible(true);
			  int iTextureWidth = pControl->GetTextureWidth();
			  int iTextureHeight = pControl->GetTextureHeight();

			  pControl->SetPosition(iXOff+(iScreenWidth-iTextureWidth)/2, iYOff-iTextureHeight);
			  CStdString strMode;
			  CUtil::Unicode2Ansi(g_localizeStrings.Get(469).c_str(),strMode);
			  strStatus.Format("%s (%i, Offset=%i)",strMode,iYOff,g_settings.m_ResInfo[m_Res[m_iCurRes]].iOSDYOffset);
			  SET_CONTROL_LABEL(GetID(), CONTROL_LABEL_ROW2,	468);
			}
			
		}
		break;
	}

	CStdString strText;
	strText.Format("%s | %s",g_settings.m_ResInfo[m_Res[m_iCurRes]].strMode,strStatus.c_str());
	SET_CONTROL_LABEL(GetID(), CONTROL_LABEL_ROW1,  strText);

	CGUIWindow::Render();

	// render the subtitles
	if (g_application.m_pPlayer)
	{
		g_application.m_pPlayer->UpdateSubtitlePosition();
		g_application.m_pPlayer->RenderSubtitles();
	}
  if (m_iControl==CONTROL_OSD)
  {
    g_application.m_guiWindowOSD.Render();
  }
}

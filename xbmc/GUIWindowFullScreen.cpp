
#include "stdafx.h"
#include "GUIWindowFullScreen.h"
#include "settings.h"
#include "application.h"
#include "util.h"
#include "utils/log.h"
#include "cores/mplayer/mplayer.h"
#include "utils/singlelock.h"
#include "videodatabase.h"
#include "cores/mplayer/ASyncDirectSound.h"
#include "playlistplayer.h"
#include "utils/CharsetConverter.h"

#include <stdio.h>

#define BLUE_BAR    0
#define LABEL_ROW1 10
#define LABEL_ROW2 11
#define LABEL_ROW3 12

#define BTN_OSD_VIDEO 13
#define BTN_OSD_AUDIO 14
#define BTN_OSD_SUBTITLE 15

#define MENU_ACTION_AVDELAY       1 
#define MENU_ACTION_SEEK          2
#define MENU_ACTION_SUBTITLEDELAY 3
#define MENU_ACTION_SUBTITLEONOFF 4
#define MENU_ACTION_SUBTITLELANGUAGE 5
#define MENU_ACTION_INTERLEAVED 6
#define MENU_ACTION_FRAMERATECONVERSIONS 7
#define MENU_ACTION_AUDIO_STREAM 8

#define MENU_ACTION_NEW_BOOKMARK  9
#define MENU_ACTION_NEXT_BOOKMARK  10
#define MENU_ACTION_CLEAR_BOOKMARK  11

#define MENU_ACTION_NOCACHE 12

#define IMG_PAUSE     16
#define IMG_2X	      17
#define IMG_4X	      18
#define IMG_8X		  19
#define IMG_16X       20
#define IMG_32X       21

#define IMG_2Xr	      117
#define IMG_4Xr		  118
#define IMG_8Xr		  119
#define IMG_16Xr	  120
#define IMG_32Xr      121

#define LABEL_CURRENT_TIME 22

extern IDirectSoundRenderer* m_pAudioDecoder;
CGUIWindowFullScreen::CGUIWindowFullScreen(void)
:CGUIWindow(0)
{
	m_strTimeStamp[0]=0;
	m_iTimeCodePosition=0;
	m_bShowTime=false;
	m_bShowCodecInfo=false;
	m_bShowViewModeInfo=false;
	m_dwShowViewModeTimeout=0;
	m_bShowCurrentTime=false;
	m_dwTimeCodeTimeout=0;
	m_fFPS=0;
	m_fFrameCounter=0.0f;
	m_dwFPSTime=timeGetTime();
	m_bSmoothFFwdRewd = false;
	m_bDiscreteFFwdRewd = false;
	m_subtitleFont = NULL;

  // audio
  //  - language
  //  - volume
  //  - stream

  // video
  //  - Create Bookmark (294)
  //  - Cycle bookmarks (295)
  //  - Clear bookmarks (296)
  //  - jump to specific time
  //  - slider
  //  - av delay

  // subtitles
  //  - delay
  //  - language
  
}

CGUIWindowFullScreen::~CGUIWindowFullScreen(void)
{
}

void CGUIWindowFullScreen::AllocResources()
{
  CGUIWindow::AllocResources();
  g_application.m_guiWindowOSD.AllocResources();
}

void CGUIWindowFullScreen::FreeResources()
{
	g_settings.Save();
  g_application.m_guiWindowOSD.FreeResources();
  CGUIWindow::FreeResources();
}

void CGUIWindowFullScreen::OnAction(const CAction &action)
{
  
	if (m_bOSDVisible)
	{
		if (action.wID == ACTION_SHOW_OSD && !g_application.m_guiWindowOSD.SubMenuVisible())	// hide the OSD
		{
      CSingleLock lock(m_section);         
      OutputDebugString("CGUIWindowFullScreen::HIDEOSD\n");
			CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0,0,0,NULL);
			g_application.m_guiWindowOSD.OnMessage(msg);	// Send a de-init msg to the OSD
			m_bOSDVisible=false;

		}
		else
		{
      OutputDebugString("CGUIWindowFullScreen::OnAction() reset timeout\n");
      m_dwOSDTimeOut=timeGetTime();
			g_application.m_guiWindowOSD.OnAction(action);	// route keys to OSD window
		}
		return;
	}
  
  // if the player has handled the message, just return
  if (g_application.m_pPlayer != NULL && g_application.m_pPlayer->OnAction(action)) return;
  
	switch (action.wID)
	{

    // previous : play previous song from playlist
    case ACTION_PREV_ITEM:
	  {
		  g_playlistPlayer.PlayPrevious();
	  }
    break;

    // next : play next song from playlist
    case ACTION_NEXT_ITEM:
	  {
		  g_playlistPlayer.PlayNext();
	  }
    break;

		case ACTION_SHOW_GUI:
    {
			// switch back to the menu
			OutputDebugString("Switching to GUI\n");
			m_gWindowManager.PreviousWindow();
			if (g_application.m_pPlayer)
				g_application.m_pPlayer->Update();
			OutputDebugString("Now in GUI\n");

			return;
    }
    break;

		case ACTION_STEP_BACK:
      SeekPercentage(g_application.m_pPlayer->GetPercentage()-2);
		break;

		case ACTION_STEP_FORWARD:
			SeekPercentage(g_application.m_pPlayer->GetPercentage()+2);
		break;

		case ACTION_BIG_STEP_BACK:
			SeekPercentage(g_application.m_pPlayer->GetPercentage()-10);
		break;

		case ACTION_BIG_STEP_FORWARD:
			SeekPercentage(g_application.m_pPlayer->GetPercentage()+10);
		break;

		case ACTION_SHOW_MPLAYER_OSD:
			g_application.m_pPlayer->ToggleOSD();
		break;

		case ACTION_SHOW_OSD:	// Show the OSD
    {	
      //CSingleLock lock(m_section);      
      OutputDebugString("CGUIWindowFullScreen:SHOWOSD\n");
      m_dwOSDTimeOut=timeGetTime();
      
	    CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0,0,0,NULL);
	    g_application.m_guiWindowOSD.OnMessage(msg);	// Send an init msg to the OSD
      m_bOSDVisible=true;
      
    }
    break;
			
		case ACTION_SHOW_SUBTITLES:
    {	
      g_application.m_pPlayer->ToggleSubtitles();
    }
		break;

		case ACTION_SHOW_CODEC:
		{
		  m_bShowCodecInfo = !m_bShowCodecInfo;
		}
		break;

		case ACTION_NEXT_SUBTITLE:
    {
      g_application.m_pPlayer->SwitchToNextLanguage();
    }
		break;

		case ACTION_STOP:
    {
      g_application.StopPlaying();
    }
		break;

		// PAUSE action is handled globally in the Application class
		case ACTION_PAUSE:
			if (g_application.m_pPlayer) g_application.m_pPlayer->Pause();
		break;

    case ACTION_SUBTITLE_DELAY_MIN:
			g_application.m_pPlayer->SubtitleOffset(false);
		break;
		case ACTION_SUBTITLE_DELAY_PLUS:
			g_application.m_pPlayer->SubtitleOffset(true);
		break;
		case ACTION_AUDIO_DELAY_MIN:
			g_application.m_pPlayer->AudioOffset(false);
		break;
		case ACTION_AUDIO_DELAY_PLUS:
			g_application.m_pPlayer->AudioOffset(true);
		break;
		case ACTION_AUDIO_NEXT_LANGUAGE:
			//g_application.m_pPlayer->AudioOffset(false);
		break;
		case ACTION_PLAY:
      if (g_application.m_pPlayer->IsPaused()) {
				g_application.m_pPlayer->Pause();
      }
      g_application.SetPlaySpeed(1);
		m_bShowCurrentTime = true;
    m_dwTimeCodeTimeout=timeGetTime();
		break;
		case ACTION_REWIND:
		case ACTION_FORWARD:
			ChangetheSpeed(action.wID);
		break;
		case ACTION_ANALOG_REWIND:
		case ACTION_ANALOG_FORWARD:
			{
				// calculate the speed based on the amount the button is held down
				int iPower = (int)(action.fAmount1*5.5f);
				// returns 0 -> 5
				int iSpeed = 1 << iPower;
				if (iSpeed==1)
				{
					m_bSmoothFFwdRewd = false;
				}
				else
				{
					m_bSmoothFFwdRewd = true;
					if (action.wID == ACTION_ANALOG_REWIND)
						iSpeed = -iSpeed;
				}
				g_application.SetPlaySpeed(iSpeed);
			}
		break;
		case REMOTE_0:
			ChangetheTimeCode(REMOTE_0);
		break;
		case REMOTE_1:
			ChangetheTimeCode(REMOTE_1);
		break;
		case REMOTE_2:
			ChangetheTimeCode(REMOTE_2);
		break;
		case REMOTE_3:
			ChangetheTimeCode(REMOTE_3);
		break;
		case REMOTE_4:
			ChangetheTimeCode(REMOTE_4);
		break;
		case REMOTE_5:
			ChangetheTimeCode(REMOTE_5);
		break;
		case REMOTE_6:
			ChangetheTimeCode(REMOTE_6);
		break;
		case REMOTE_7:
			ChangetheTimeCode(REMOTE_7);
		break;
		case REMOTE_8:
			ChangetheTimeCode(REMOTE_8);
		break;
		case REMOTE_9:
			ChangetheTimeCode(REMOTE_9);
		break;

		case ACTION_ASPECT_RATIO:
		{	// toggle the aspect ratio mode (only if the info is onscreen)
		  if (m_bShowViewModeInfo)
		    {
					SetViewMode(++g_stSettings.m_currentVideoSettings.m_ViewMode);
		    }
		  m_bShowViewModeInfo = true;
		  m_dwShowViewModeTimeout = timeGetTime();
		}
		break;
 		case ACTION_SMALL_STEP_BACK:
    {
			// unpause the player so that it seeks nicely
			bool bNeedPause(false);
			if (g_application.m_pPlayer->IsPaused())
			{
				g_application.m_pPlayer->Pause();
				bNeedPause = true;
			}
 			int orgpos=(int)g_application.m_pPlayer->GetTime();
			int triesleft=g_stSettings.m_iSmallStepBackTries;
      int jumpsize = g_stSettings.m_iSmallStepBackSeconds; // secs
      int setpos=(orgpos > jumpsize) ? orgpos-jumpsize : 0; // First jump = 2*jumpsize
      int newpos;
      do
      {
				setpos = (setpos > jumpsize) ? setpos-jumpsize : 0;
 				g_application.m_pPlayer->SeekTime(setpos*1000);
 				Sleep(g_stSettings.m_iSmallStepBackDelay); // delay to let mplayer finish its seek (in ms)
 				newpos = (int)g_application.m_pPlayer->GetTime();
 			} while ( (newpos>orgpos-jumpsize) && (setpos>0) && (--triesleft>0));
			// repause player if needed
			if (bNeedPause) g_application.m_pPlayer->Pause();
  	}
    break;
	}
	CGUIWindow::OnAction(action);
}

bool CGUIWindowFullScreen::OnMessage(CGUIMessage& message)
{	
  
	if (m_bOSDVisible)
	{
		//if (timeGetTime()-m_dwOSDTimeOut > 5000)
		if (g_guiSettings.GetInt("MyVideos.OSDTimeout"))
		{
			if ( (timeGetTime() - m_dwOSDTimeOut) > (DWORD)(g_guiSettings.GetInt("MyVideos.OSDTimeout") * 1000))
			{
				CSingleLock lock(m_section);
				CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0,0,0,NULL);
				g_application.m_guiWindowOSD.OnMessage(msg);	// Send a de-init msg to the OSD
				m_bOSDVisible=false;
				return g_application.m_guiWindowOSD.OnMessage(message);	// route messages to OSD window
			}
		}

		switch (message.GetMessage())
		{
			case GUI_MSG_SETFOCUS:
			case GUI_MSG_LOSTFOCUS:
			case GUI_MSG_CLICKED:
			case GUI_MSG_WINDOW_INIT:
			case GUI_MSG_WINDOW_DEINIT:
				OutputDebugString("CGUIWindowFullScreen::OnMessage() reset timeout\n");
				m_dwOSDTimeOut=timeGetTime();
			break;
		}
		return g_application.m_guiWindowOSD.OnMessage(message);	// route messages to OSD window
	}

	switch (message.GetMessage())
	{
		case GUI_MSG_WINDOW_INIT:
		{
			m_bLastRender=false;
			m_bOSDVisible=false;

			CGUIWindow::OnMessage(message);

			CUtil::SetBrightnessContrastGammaPercent(g_stSettings.m_currentVideoSettings.m_Brightness,g_stSettings.m_currentVideoSettings.m_Contrast,g_stSettings.m_currentVideoSettings.m_Gamma,true);

			g_graphicsContext.Lock();
			g_graphicsContext.SetFullScreenVideo( true );
			g_graphicsContext.Unlock();

			if (g_application.m_pPlayer)
				g_application.m_pPlayer->Update();

			HideOSD();

			m_iCurrentBookmark=0;
			m_bShowCodecInfo = false;
			m_bShowViewModeInfo = false;

			// set the correct view mode
			SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);

			if (CUtil::IsUsingTTFSubtitles())
			{
				CSingleLock lock(m_fontLock);

				if (m_subtitleFont)
				{
					delete m_subtitleFont;
					m_subtitleFont = NULL;
				}

				m_subtitleFont = new CGUIFontTTF("__subtitle__");
				CStdString fontPath = "Q:\\Media\\Fonts\\";
				fontPath += g_guiSettings.GetString("Subtitles.Font");
				if (!m_subtitleFont->Load(fontPath, g_guiSettings.GetInt("Subtitles.Height"), g_guiSettings.GetInt("Subtitles.Style")))
				{
					delete m_subtitleFont;
					m_subtitleFont = NULL;
				}
			}
			else
				m_subtitleFont = NULL;

			return true;
		}
		case GUI_MSG_WINDOW_DEINIT:
		{
			// Pause player before lock or the app will deadlock
			if (g_application.m_pPlayer)
				g_application.m_pPlayer->Update(true);	
			// Pause so that we make sure that our fullscreen renderer has finished...
			Sleep(100);

			CSingleLock lock(m_section);

			if (m_bOSDVisible)
			{
					CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0,0,0,NULL);
					g_application.m_guiWindowOSD.OnMessage(msg);	// Send a de-init msg to the OSD
			}

			m_bOSDVisible=false;

			CGUIWindow::OnMessage(message);

			CUtil::RestoreBrightnessContrastGamma();

			g_graphicsContext.Lock();
			g_graphicsContext.SetFullScreenVideo(false);
			g_graphicsContext.SetGUIResolution(g_guiSettings.m_LookAndFeelResolution);
			g_graphicsContext.Unlock();
      
			m_iCurrentBookmark=0;

			HideOSD();

			CSingleLock lockFont(m_fontLock);
			if (m_subtitleFont)
			{
				delete m_subtitleFont;
				m_subtitleFont = NULL;
			}

			return true;
		}
    case GUI_MSG_SETFOCUS:
    case GUI_MSG_LOSTFOCUS:
      if (m_bOSDVisible) return true;
      if (message.GetSenderId() != WINDOW_FULLSCREEN_VIDEO) return true;
    break;
	}

	return CGUIWindow::OnMessage(message);
}

void CGUIWindowFullScreen::OnMouse()
{
	if (g_Mouse.bClick[MOUSE_RIGHT_BUTTON])
	{	// no control found to absorb this click - go back to GUI
		CAction action;
		action.wID = ACTION_SHOW_GUI;
		OnAction(action);
		return;
	}
	if (g_Mouse.bClick[MOUSE_LEFT_BUTTON])
	{	// no control found to absorb this click - toggle the OSD
		CAction action;
		action.wID = ACTION_SHOW_OSD;
		OnAction(action);
	}
}

// Dummy override of Render() - RenderFullScreen() is where the action takes place
// this is called via mplayer when the video window is flipped (indicating a frame
// change) so that we get smooth video playback
void CGUIWindowFullScreen::Render()
{
	return;
}

bool CGUIWindowFullScreen::NeedRenderFullScreen()
{
  CSingleLock lock(m_section);      
	if (g_application.m_pPlayer) 
  {
    if (g_application.m_pPlayer->IsPaused() )return true;
  }
  
  if (g_application.GetPlaySpeed() != 1) 
  {
		m_bShowCurrentTime = true;
    m_dwTimeCodeTimeout=timeGetTime();
  }
  if (m_bShowTime) return true;
  if (m_bShowCodecInfo) return true;
  if (m_bShowViewModeInfo) return true;
  if (m_bShowCurrentTime) return true;
	if (m_gWindowManager.IsRouted()) return true;
//  if (g_application.m_guiDialogVolumeBar.IsRunning()) return true; // volume bar is onscreen
//  if (g_application.m_guiDialogKaiToast.IsRunning()) return true; // kai toast is onscreen
  if (m_bOSDVisible) return true;
  if (g_Mouse.IsActive()) return true;
  if (CUtil::IsUsingTTFSubtitles() && g_application.m_pPlayer->GetSubtitleVisible() && m_subtitleFont)
	  return true;
  if (m_bLastRender)
  {
    m_bLastRender=false;
  }
 
  return false;
}

void CGUIWindowFullScreen::RenderFullScreen()
{
	// check whether we should stop ffwd/rewd
	if (!m_bSmoothFFwdRewd && g_application.GetPlaySpeed() != 1 && !m_bDiscreteFFwdRewd)
		g_application.SetPlaySpeed(1);

  if (g_application.GetPlaySpeed() != 1) 
  {
		m_bShowCurrentTime = true;
    m_dwTimeCodeTimeout=timeGetTime();
  }

  m_bLastRender=true;
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

  bool bRenderGUI(false);
	if (g_application.m_pPlayer->IsPaused() )
  {
    SET_CONTROL_VISIBLE(IMG_PAUSE);  
    bRenderGUI=true;
  }
  else
  {
    SET_CONTROL_HIDDEN(IMG_PAUSE);  
  }
 
	//------------------------
	if (m_bShowCodecInfo) 
	{
		bRenderGUI=true;
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
	}
	//----------------------
	// ViewMode Information
	//----------------------
	if (m_bShowViewModeInfo && timeGetTime() - m_dwShowViewModeTimeout > 2500)
	{
		m_bShowViewModeInfo = false;
	}
	if (m_bShowViewModeInfo)
	{
		bRenderGUI=true;
		{
			// get the "View Mode" string
			CStdString strTitle = g_localizeStrings.Get(629);
			CStdString strMode = g_localizeStrings.Get(630 + g_stSettings.m_currentVideoSettings.m_ViewMode);
			CStdString strInfo;
			strInfo.Format("%s : %s",strTitle.c_str(), strMode.c_str());
			CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW1); 
			msg.SetLabel(strInfo); 
			OnMessage(msg);
		}
		// show sizing information
		RECT SrcRect, DestRect;
		float fAR;
		g_application.m_pPlayer->GetVideoRect(SrcRect, DestRect);
		g_application.m_pPlayer->GetVideoAspectRatio(fAR);
		{
			CStdString strSizing;
			strSizing.Format("Sizing: (%i,%i)->(%i,%i) (Zoom x%2.2f) AR:%2.2f:1 (Pixels: %2.2f:1)", 
												SrcRect.right-SrcRect.left,SrcRect.bottom-SrcRect.top,
												DestRect.right-DestRect.left,DestRect.bottom-DestRect.top, g_stSettings.m_fZoomAmount, fAR*g_stSettings.m_fPixelRatio, g_stSettings.m_fPixelRatio);
			CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW2); 
			msg.SetLabel(strSizing);
			OnMessage(msg);
		}
		// show resolution information
		int iResolution=g_graphicsContext.GetVideoResolution();
		{
			CStdString strStatus;
			strStatus.Format("%ix%i %s", g_settings.m_ResInfo[iResolution].iWidth, g_settings.m_ResInfo[iResolution].iHeight, g_settings.m_ResInfo[iResolution].strMode);
			if (g_guiSettings.GetBool("Filters.Soften"))
				strStatus += "  |  Soften";
			else
				strStatus += "  |  No Soften";

			CStdString strFilter;
			strFilter.Format("  |  Flicker Filter: %i", g_guiSettings.GetInt("Filters.Flicker"));
			strStatus += strFilter;
			CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW3); 
			msg.SetLabel(strStatus); 
			OnMessage(msg);
		}
	}

	if (m_gWindowManager.IsRouted())
		m_gWindowManager.Render();
/*
  // Check if we need to render the popup volume bar...
  if (g_application.m_guiDialogVolumeBar.IsRunning())
  {
	  g_application.m_guiDialogVolumeBar.Render();
  }
  // Check if we need to render the popup kai toast...
  if (g_application.m_guiDialogKaiToast.IsRunning())
  {
	  g_application.m_guiDialogKaiToast.Render();
  }
*/
  if (m_bOSDVisible)
  {
	  // tell the OSD window to draw itself
    CSingleLock lock(m_section);
	  g_application.m_guiWindowOSD.Render();
	  // Render the mouse pointer, if visible...
	  if (g_Mouse.IsActive()) g_application.m_guiPointer.Render();
    return;
  }

	RenderTTFSubtitles();

	if (m_bShowTime && m_iTimeCodePosition != 0)
	{
		if ( (timeGetTime() - m_dwTimeCodeTimeout) >=2500)
		{
			m_bShowTime=false;
			m_iTimeCodePosition = 0;
			return;
		}
		bRenderGUI=true;
		char displaytime[32] = "??:??/??:??:?? [??:??:??]";
		CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW1); 
		for(int count = 0; count < m_iTimeCodePosition; count++)
		{
			if(m_strTimeStamp[count] == -1)
				displaytime[count] = ':';
			else
				displaytime[count] = (char)m_strTimeStamp[count]+48;
		}
		unsigned int tmpvar = g_application.m_pPlayer->GetTotalTime();
		if(tmpvar != 0)
		{
			int ihour = tmpvar / 3600;
			int imin  = (tmpvar-ihour*3600) / 60;
			int isec = (tmpvar-ihour*3600) % 60;
			sprintf(&displaytime[5], "/%2.2d:%2.2d:%2.2d", ihour,imin,isec);
		}
    else 
		{
			sprintf(&displaytime[5], "/00:00:00");
		}
		__int64 iCurrentTime=g_application.m_pPlayer->GetTime();
		if(iCurrentTime != 0)
		{
			__int64 ihour = iCurrentTime / (__int64)3600;
			__int64 imin  = (iCurrentTime-ihour*3600) / 60;
			__int64 isec = (iCurrentTime-ihour*3600) % 60;
			sprintf(&displaytime[14], " [%2.2d:%2.2d:%2.2d]", (int)ihour,(int)imin,(int)isec);
		}
		else
		{
			sprintf(&displaytime[14], " [??:??:??]");
		}
		msg.SetLabel(displaytime); 
		OnMessage(msg);
	}	

	// check if we have bidirectional controls and set controlIds
	bool bRewIcons = (GetControl(IMG_2Xr)!=NULL);

	int iSpeed=g_application.GetPlaySpeed();
	// hide all speed indicators first
	SET_CONTROL_HIDDEN(IMG_2X);
	SET_CONTROL_HIDDEN(IMG_4X);
	SET_CONTROL_HIDDEN(IMG_8X);
	SET_CONTROL_HIDDEN(IMG_16X);
	SET_CONTROL_HIDDEN(IMG_32X);

	if(bRewIcons)
	{
		SET_CONTROL_HIDDEN(IMG_2Xr);
		SET_CONTROL_HIDDEN(IMG_4Xr);
		SET_CONTROL_HIDDEN(IMG_8Xr);
		SET_CONTROL_HIDDEN(IMG_16Xr);
		SET_CONTROL_HIDDEN(IMG_32Xr);
	}

	if(iSpeed!=1)
	{
		bRenderGUI=true;
		switch(iSpeed)
		{
			case 2:
				SET_CONTROL_VISIBLE(IMG_2X);
				break;
			case -2:
				SET_CONTROL_VISIBLE(bRewIcons ? IMG_2Xr : IMG_2X);
				break;
			case 4:
				SET_CONTROL_VISIBLE(IMG_4X);
				break;
			case -4:
				SET_CONTROL_VISIBLE(bRewIcons ? IMG_4Xr : IMG_4X);
				break;
			case 8:
				SET_CONTROL_VISIBLE(IMG_8X);
				break;
			case -8:
				SET_CONTROL_VISIBLE(bRewIcons ? IMG_8Xr : IMG_8X);
				break;
			case 16:
				SET_CONTROL_VISIBLE(IMG_16X);
				break;
			case -16:
				SET_CONTROL_VISIBLE(bRewIcons ? IMG_16Xr : IMG_16X);
				break;
			case 32:
				SET_CONTROL_VISIBLE(IMG_32X);
				break;
			case -32:
				SET_CONTROL_VISIBLE(bRewIcons ? IMG_32Xr : IMG_32X);
				break;
			default:
				// do nothing, leave them all invisible
				break;
		}
	}

	// Render current time if requested
	if (m_bShowCurrentTime)
	{
		CStdString strTime;
		bRenderGUI =true;
		SET_CONTROL_VISIBLE(LABEL_CURRENT_TIME);
		__int64 lPTS=10*g_application.m_pPlayer->GetTime();
		int hh = (int)(lPTS / 36000) % 100;
		int mm = (int)((lPTS / 600) % 60);
		int ss = (int)((lPTS /  10) % 60);
	  
		if (hh>=1)
			strTime.Format("%02.2i:%02.2i:%02.2i",hh,mm,ss);
		else
			strTime.Format("%02.2i:%02.2i",mm,ss);
		CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_CURRENT_TIME); 
		msg.SetLabel(strTime); 
		OnMessage(msg); 
		if ( (timeGetTime() - m_dwTimeCodeTimeout) >=2500)
		{
			m_bShowCurrentTime = false;
		}
	}
	else
	{
		SET_CONTROL_HIDDEN(LABEL_CURRENT_TIME);
	}

	if ( bRenderGUI)
	{
		if (g_application.m_pPlayer->IsPaused() || iSpeed != 1)
		{
			SET_CONTROL_HIDDEN(LABEL_ROW1);
			SET_CONTROL_HIDDEN(LABEL_ROW2);
			SET_CONTROL_HIDDEN(LABEL_ROW3);
			SET_CONTROL_HIDDEN(BLUE_BAR);
		}
		else if (m_bShowCodecInfo || m_bShowViewModeInfo)
		{
			SET_CONTROL_VISIBLE(LABEL_ROW1);
			SET_CONTROL_VISIBLE(LABEL_ROW2);
			SET_CONTROL_VISIBLE(LABEL_ROW3);
			SET_CONTROL_VISIBLE(BLUE_BAR);
		}
		else if (m_bShowTime)
		{
			SET_CONTROL_VISIBLE(LABEL_ROW1);
			SET_CONTROL_HIDDEN(LABEL_ROW2);
			SET_CONTROL_HIDDEN(LABEL_ROW3);
			SET_CONTROL_VISIBLE(BLUE_BAR);
		}
		else
		{
			SET_CONTROL_HIDDEN(LABEL_ROW1);
			SET_CONTROL_HIDDEN(LABEL_ROW2);
			SET_CONTROL_HIDDEN(LABEL_ROW3);
			SET_CONTROL_HIDDEN(BLUE_BAR);
		}
		CGUIWindow::Render();
	}
	// and lastly render the mouse pointer...
	if (g_Mouse.IsActive()) g_application.m_guiPointer.Render();
}

void CGUIWindowFullScreen::RenderTTFSubtitles()
{
	if (CUtil::IsUsingTTFSubtitles() && g_application.m_pPlayer->GetSubtitleVisible() && m_subtitleFont)
	{
		CSingleLock lock(m_fontLock);

		subtitle* sub = mplayer_GetCurrentSubtitle();

		if (sub != NULL)
		{
			CStdStringW subtitleText = L"";

			for (int i = 0; i < sub->lines; i++)
			{
				if (i != 0)
				{
					subtitleText += L"\n";
				}

				CStdStringA S = sub->text[i];
				CStdStringW W;
				g_charsetConverter.subtitleCharsetToFontCharset(S, W);
				subtitleText += W;
			}

			int m_iResolution = g_graphicsContext.GetVideoResolution();

			float w;
			float h;
			m_subtitleFont->GetTextExtent(subtitleText.c_str(), &w, &h);

			float x = (float) (g_settings.m_ResInfo[m_iResolution].iWidth) / 2;
			float y = (float) g_settings.m_ResInfo[m_iResolution].iSubtitles - h;

			float outlinewidth = (float)g_guiSettings.GetInt("Subtitles.Height")/8;

			m_subtitleFont->DrawText(x-outlinewidth, y, 0, subtitleText.c_str(), XBFONT_CENTER_X);
			m_subtitleFont->DrawText(x+outlinewidth, y, 0, subtitleText.c_str(), XBFONT_CENTER_X);
			m_subtitleFont->DrawText(x, y+outlinewidth, 0, subtitleText.c_str(), XBFONT_CENTER_X);
			m_subtitleFont->DrawText(x, y-outlinewidth, 0, subtitleText.c_str(), XBFONT_CENTER_X);
			m_subtitleFont->DrawText(x, y, g_guiSettings.GetInt("Subtitles.Color")==SUBTITLE_COLOR_YELLOW ? 0xFFFF00 : 0xFFFFFF, subtitleText.c_str(), XBFONT_CENTER_X);
		}
	}
}

void CGUIWindowFullScreen::HideOSD()
{
	CSingleLock lock(m_section);      
	m_osdMenu.Clear();
	SET_CONTROL_HIDDEN(BTN_OSD_VIDEO);
	SET_CONTROL_HIDDEN(BTN_OSD_AUDIO);
	SET_CONTROL_HIDDEN(BTN_OSD_SUBTITLE);

	SET_CONTROL_VISIBLE(LABEL_ROW1);
	SET_CONTROL_VISIBLE(LABEL_ROW2);
	SET_CONTROL_VISIBLE(LABEL_ROW3);
	SET_CONTROL_VISIBLE(BLUE_BAR);
}

void CGUIWindowFullScreen::ShowOSD()
{

}

bool CGUIWindowFullScreen::OSDVisible() const
{
  return m_bOSDVisible;
}

void CGUIWindowFullScreen::OnExecute(int iAction, const IOSDOption* option)
{

}
void CGUIWindowFullScreen::ChangetheTimeCode(DWORD remote)
{
	if(remote >=58 && remote <= 67) //Make sure it's only for the remote
	{
		m_bShowTime = true;
		m_dwTimeCodeTimeout=timeGetTime();
		int	itime = remote - 58;
		if(m_iTimeCodePosition <= 4 && m_iTimeCodePosition != 2)
		{
			m_strTimeStamp[m_iTimeCodePosition++] = itime;
			if(m_iTimeCodePosition == 2)
				m_strTimeStamp[m_iTimeCodePosition++] = -1;
		}
    if(m_iTimeCodePosition > 4)
		{
			long itotal,ih,im,is=0;                 
			ih =  (m_strTimeStamp[0]-0)*10;
			ih += (m_strTimeStamp[1]-0);   
			im =  (m_strTimeStamp[3]-0)*10;   
			im += (m_strTimeStamp[4]-0);   
			im*=60;
			ih*=3600; 
			itotal = ih+im+is;
			bool bNeedsPause(false);
			if (g_application.m_pPlayer->IsPaused())
			{
				bNeedsPause = true;
				g_application.m_pPlayer->Pause();
			}
			if(itotal < g_application.m_pPlayer->GetTotalTime())
				g_application.m_pPlayer->SeekTime(itotal*1000);
			if (bNeedsPause)
			{
				Sleep(g_stSettings.m_iSmallStepBackDelay);	// allow mplayer to finish it's seek (nasty hack)
				g_application.m_pPlayer->Pause();
			}
			m_iTimeCodePosition = 0;
      m_bShowTime=false;
		}
	}
}
void CGUIWindowFullScreen::ChangetheSpeed(DWORD action)
{
  int iSpeed=g_application.GetPlaySpeed();
	if (action == ACTION_REWIND && iSpeed == 1) // Enables Rewinding
		iSpeed *=-2;
	else if (action == ACTION_REWIND && iSpeed > 1) //goes down a notch if you're FFing
		iSpeed /=2;
	else if (action == ACTION_FORWARD && iSpeed < 1) //goes up a notch if you're RWing
		iSpeed /= 2;
	else 
		iSpeed *= 2;

	if (action == ACTION_FORWARD && iSpeed == -1) //sets iSpeed back to 1 if -1 (didn't plan for a -1)
		iSpeed = 1;
	if (iSpeed > 32 || iSpeed < -32)
		iSpeed = 1;
	m_bDiscreteFFwdRewd = (iSpeed != 1);
  g_application.SetPlaySpeed(iSpeed);
}	

void CGUIWindowFullScreen::Update()
{

}

void CGUIWindowFullScreen::SetViewMode(int iViewMode)
{
	if (iViewMode<VIEW_MODE_NORMAL || iViewMode>VIEW_MODE_CUSTOM) iViewMode=VIEW_MODE_NORMAL;
	g_stSettings.m_currentVideoSettings.m_ViewMode = iViewMode;

	if (g_stSettings.m_currentVideoSettings.m_ViewMode == VIEW_MODE_NORMAL)
	{	// normal mode...
		g_stSettings.m_fPixelRatio = 1.0;
		g_stSettings.m_fZoomAmount = 1.0;
		return;
	}
	if (g_stSettings.m_currentVideoSettings.m_ViewMode == VIEW_MODE_CUSTOM)
	{
		g_stSettings.m_fZoomAmount = g_stSettings.m_currentVideoSettings.m_CustomZoomAmount;
		g_stSettings.m_fPixelRatio = g_stSettings.m_currentVideoSettings.m_CustomPixelRatio;
		return;
	}
	
	// get our calibrated full screen resolution
	RESOLUTION iRes = g_graphicsContext.GetVideoResolution();
	float fOffsetX1 = (float)g_settings.m_ResInfo[iRes].Overscan.left;
	float fOffsetY1 = (float)g_settings.m_ResInfo[iRes].Overscan.top;
	float fScreenWidth = (float)(g_settings.m_ResInfo[iRes].Overscan.right-g_settings.m_ResInfo[iRes].Overscan.left);
	float fScreenHeight = (float)(g_settings.m_ResInfo[iRes].Overscan.bottom-g_settings.m_ResInfo[iRes].Overscan.top);
	// and the source frame ratio
	float fSourceFrameRatio;
	g_application.m_pPlayer->GetVideoAspectRatio(fSourceFrameRatio);

	if (g_stSettings.m_currentVideoSettings.m_ViewMode == VIEW_MODE_ZOOM)
	{	// zoom image so no black bars
		g_stSettings.m_fPixelRatio = 1.0;
		// calculate the desired output ratio
		float fOutputFrameRatio = fSourceFrameRatio * g_stSettings.m_fPixelRatio / g_settings.m_ResInfo[iRes].fPixelRatio; 
		// now calculate the correct zoom amount.  First zoom to full height.
		float fNewHeight = fScreenHeight;
		float fNewWidth = fNewHeight*fOutputFrameRatio;
		g_stSettings.m_fZoomAmount = fNewWidth/fScreenWidth;
		if (fNewWidth < fScreenWidth)
		{	// zoom to full width
			fNewWidth = fScreenWidth;
			fNewHeight = fNewWidth/fOutputFrameRatio;
			g_stSettings.m_fZoomAmount = fNewHeight/fScreenHeight;
		}
	}
	else if (g_stSettings.m_currentVideoSettings.m_ViewMode == VIEW_MODE_STRETCH_4x3)
	{	// stretch image to 4:3 ratio
		g_stSettings.m_fZoomAmount = 1.0;
		if (iRes == PAL_4x3 || iRes == PAL60_4x3 || iRes == NTSC_4x3 || iRes == HDTV_480p_4x3)
		{	// stretch to the limits of the 4:3 screen.
			// incorrect behaviour, but it's what the users want, so...
			g_stSettings.m_fPixelRatio = (fScreenWidth/fScreenHeight)*g_settings.m_ResInfo[iRes].fPixelRatio/fSourceFrameRatio;
		}
		else
		{
			// now we need to set g_stSettings.m_fPixelRatio so that 
			// fOutputFrameRatio = 4:3.
			g_stSettings.m_fPixelRatio = (4.0f/3.0f)/fSourceFrameRatio;
		}
	}
	else if (g_stSettings.m_currentVideoSettings.m_ViewMode == VIEW_MODE_STRETCH_14x9)
	{	// stretch image to 14:9 ratio
		g_stSettings.m_fZoomAmount = 1.0;
		// now we need to set g_stSettings.m_fPixelRatio so that 
		// fOutputFrameRatio = 14:9.
		g_stSettings.m_fPixelRatio = (14.0f/9.0f)/fSourceFrameRatio;
	}
	else if (g_stSettings.m_currentVideoSettings.m_ViewMode == VIEW_MODE_STRETCH_16x9)
	{	// stretch image to 16:9 ratio
		g_stSettings.m_fZoomAmount = 1.0;
		if (iRes == PAL_4x3 || iRes == PAL60_4x3 || iRes == NTSC_4x3 || iRes == HDTV_480p_4x3)
		{	// now we need to set g_stSettings.m_fPixelRatio so that 
			// fOutputFrameRatio = 16:9.
			g_stSettings.m_fPixelRatio = (16.0f/9.0f)/fSourceFrameRatio;
		}
		else
		{	// stretch to the limits of the 16:9 screen.
			// incorrect behaviour, but it's what the users want, so...
			g_stSettings.m_fPixelRatio = (fScreenWidth/fScreenHeight)*g_settings.m_ResInfo[iRes].fPixelRatio/fSourceFrameRatio;
		}
	}
	else// if (g_stSettings.m_currentVideoSettings.m_ViewMode == VIEW_MODE_ORIGINAL)
	{	// zoom image so that the height is the original size
		g_stSettings.m_fPixelRatio = 1.0;
		// get the size of the media file
		RECT srcRect, destRect;
		g_application.m_pPlayer->GetVideoRect(srcRect, destRect);
		// calculate the desired output ratio
		float fOutputFrameRatio = fSourceFrameRatio * g_stSettings.m_fPixelRatio / g_settings.m_ResInfo[iRes].fPixelRatio; 
		// now calculate the correct zoom amount.  First zoom to full width.
		float fNewWidth = fScreenWidth;
		float fNewHeight = fNewWidth/fOutputFrameRatio;
		if (fNewHeight > fScreenHeight)
		{	// zoom to full height
			fNewHeight = fScreenHeight;
			fNewWidth = fNewHeight*fOutputFrameRatio;
		}
		// now work out the zoom amount so that no zoom is done
		g_stSettings.m_fZoomAmount = (srcRect.bottom-srcRect.top)/fNewHeight;
	}
}

void CGUIWindowFullScreen::SeekPercentage(int iPercent)
{
	if (iPercent<0) iPercent=0;
	if (iPercent>100) iPercent=100;
	// Unpause mplayer if necessary
	bool bNeedsPause(false);
	if (g_application.m_pPlayer->IsPaused())
	{
		g_application.m_pPlayer->Pause();
		bNeedsPause = true;
	}
	g_application.m_pPlayer->SeekPercentage(iPercent);
	// And repause it
	if (bNeedsPause)
	{
		Sleep(g_stSettings.m_iSmallStepBackDelay);	// allow mplayer to finish it's seek (nasty hack)
		g_application.m_pPlayer->Pause();
	}

}

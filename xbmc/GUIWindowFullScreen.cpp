/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIWindowFullScreen.h"
#include "Application.h"
#include "Util.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#include "utils/GUIInfoManager.h"
#include "GUIProgressControl.h"
#include "GUIAudioManager.h"
#include "GUILabelControl.h"
#include "GUIWindowOSD.h"
#include "GUIFontManager.h"

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
#define IMG_2X        17
#define IMG_4X        18
#define IMG_8X      19
#define IMG_16X       20
#define IMG_32X       21

#define IMG_2Xr       117
#define IMG_4Xr     118
#define IMG_8Xr     119
#define IMG_16Xr    120
#define IMG_32Xr      121

//Displays current position, visible after seek or when forced
//Alt, use conditional visibility Player.DisplayAfterSeek
#define LABEL_CURRENT_TIME 22

//Displays when video is rebuffering
//Alt, use conditional visibility Player.IsCaching
#define LABEL_BUFFERING 24

//Progressbar used for buffering status and after seeking
#define CONTROL_PROGRESS 23


static DWORD color[6] = { 0xFFFFFF00, 0xFFFFFFFF, 0xFF0099FF, 0xFF00FF00, 0xFFCCFF00, 0xFF00FFFF };

CGUIWindowFullScreen::CGUIWindowFullScreen(void)
    : CGUIWindow(WINDOW_FULLSCREEN_VIDEO, "VideoFullscreen.xml")
{
  m_timeCodeStamp[0] = 0;
  m_timeCodePosition = 0;
  m_timeCodeShow = false;
  m_timeCodeTimeout = 0;
  m_bShowViewModeInfo = false;
  m_dwShowViewModeTimeout = 0;
  m_bShowCurrentTime = false;
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
{}

void CGUIWindowFullScreen::PreloadDialog(unsigned int windowID)
{
  CGUIWindow *pWindow = m_gWindowManager.GetWindow(windowID);
  if (pWindow)
  {
    pWindow->Initialize();
    pWindow->DynamicResourceAlloc(false);
    pWindow->AllocResources(false);
  }
}

void CGUIWindowFullScreen::UnloadDialog(unsigned int windowID)
{
  CGUIWindow *pWindow = m_gWindowManager.GetWindow(windowID);
  if (pWindow) {
    pWindow->FreeResources(pWindow->GetLoadOnDemand());
  }
}

void CGUIWindowFullScreen::AllocResources(bool forceLoad)
{
  CGUIWindow::AllocResources(forceLoad);
  DynamicResourceAlloc(false);
  PreloadDialog(WINDOW_OSD);
  PreloadDialog(WINDOW_DIALOG_VIDEO_OSD_SETTINGS);
  PreloadDialog(WINDOW_DIALOG_AUDIO_OSD_SETTINGS);
  // No need to preload these here, as they're preloaded by our app
//  PreloadDialog(WINDOW_DIALOG_SEEK_BAR);
//  PreloadDialog(WINDOW_DIALOG_VOLUME_BAR);
//  PreloadDialog(WINDOW_DIALOG_MUTE_BUG);
}

void CGUIWindowFullScreen::FreeResources(bool forceUnload)
{
  g_settings.Save();
  DynamicResourceAlloc(true);
  UnloadDialog(WINDOW_OSD);
  UnloadDialog(WINDOW_DIALOG_VIDEO_OSD_SETTINGS);
  UnloadDialog(WINDOW_DIALOG_AUDIO_OSD_SETTINGS);
  // No need to unload these here, as they're preloaded by our app
//  UnloadDialog(WINDOW_DIALOG_SEEK_BAR);
//  UnloadDialog(WINDOW_DIALOG_VOLUME_BAR);
//  UnloadDialog(WINDOW_DIALOG_MUTE_BUG);
  CGUIWindow::FreeResources(forceUnload);
}

bool CGUIWindowFullScreen::OnAction(const CAction &action)
{
  if (g_application.m_pPlayer != NULL && g_application.m_pPlayer->OnAction(action))
    return true;

  switch (action.wID)
  {

  case ACTION_SHOW_GUI:
    {
      // switch back to the menu
      OutputDebugString("Switching to GUI\n");
      m_gWindowManager.PreviousWindow();
      OutputDebugString("Now in GUI\n");
      return true;
    }
    break;

  case ACTION_STEP_BACK:
    Seek(false, false);
    return true;
    break;

  case ACTION_STEP_FORWARD:
    Seek(true, false);
    return true;
    break;

  case ACTION_BIG_STEP_BACK:
    Seek(false, true);
    return true;
    break;

  case ACTION_BIG_STEP_FORWARD:
    Seek(true, true);
    return true;
    break;

  case ACTION_SHOW_OSD_TIME:
    m_bShowCurrentTime = !m_bShowCurrentTime;
    if(!m_bShowCurrentTime)
      g_infoManager.SetDisplayAfterSeek(0); //Force display off
    g_infoManager.SetShowTime(m_bShowCurrentTime);
    return true;
    break;

  case ACTION_SHOW_OSD:  // Show the OSD
    {
      CGUIWindowOSD *pOSD = (CGUIWindowOSD *)m_gWindowManager.GetWindow(WINDOW_OSD);
      if (pOSD) pOSD->DoModal();
      return true;
    }
    break;

  case ACTION_SHOW_SUBTITLES:
    {
      g_application.m_pPlayer->SetSubtitleVisible(!g_application.m_pPlayer->GetSubtitleVisible());
    }
    return true;
    break;

  case ACTION_NEXT_SUBTITLE:
    {
      if (g_application.m_pPlayer->GetSubtitleCount() == 1)
        return true;

      g_stSettings.m_currentVideoSettings.m_SubtitleStream++;
      if (g_stSettings.m_currentVideoSettings.m_SubtitleStream >= g_application.m_pPlayer->GetSubtitleCount())
        g_stSettings.m_currentVideoSettings.m_SubtitleStream = 0;
      g_application.m_pPlayer->SetSubtitle(g_stSettings.m_currentVideoSettings.m_SubtitleStream);
      return true;
    }
    return true;
    break;

  case ACTION_SUBTITLE_DELAY_MIN:
    g_stSettings.m_currentVideoSettings.m_SubtitleDelay -= 0.1f;
    if (g_stSettings.m_currentVideoSettings.m_SubtitleDelay < -g_advancedSettings.m_videoSubsDelayRange)
      g_stSettings.m_currentVideoSettings.m_SubtitleDelay = -g_advancedSettings.m_videoSubsDelayRange;
    if (g_application.m_pPlayer)
      g_application.m_pPlayer->SetSubTitleDelay(g_stSettings.m_currentVideoSettings.m_SubtitleDelay);
    return true;
    break;
  case ACTION_SUBTITLE_DELAY_PLUS:
    g_stSettings.m_currentVideoSettings.m_SubtitleDelay += 0.1f;
    if (g_stSettings.m_currentVideoSettings.m_SubtitleDelay > g_advancedSettings.m_videoSubsDelayRange)
      g_stSettings.m_currentVideoSettings.m_SubtitleDelay = g_advancedSettings.m_videoSubsDelayRange;
    if (g_application.m_pPlayer)
      g_application.m_pPlayer->SetSubTitleDelay(g_stSettings.m_currentVideoSettings.m_SubtitleDelay);
    return true;
    break;
  case ACTION_AUDIO_DELAY_MIN:
    g_stSettings.m_currentVideoSettings.m_AudioDelay -= 0.1f;
    if (g_stSettings.m_currentVideoSettings.m_AudioDelay < -g_advancedSettings.m_videoAudioDelayRange)
      g_stSettings.m_currentVideoSettings.m_AudioDelay = -g_advancedSettings.m_videoAudioDelayRange;
    if (g_application.m_pPlayer)
      g_application.m_pPlayer->SetAVDelay(g_stSettings.m_currentVideoSettings.m_AudioDelay);
    return true;
    break;
  case ACTION_AUDIO_DELAY_PLUS:
    g_stSettings.m_currentVideoSettings.m_AudioDelay += 0.1f;
    if (g_stSettings.m_currentVideoSettings.m_AudioDelay > g_advancedSettings.m_videoAudioDelayRange)
      g_stSettings.m_currentVideoSettings.m_AudioDelay = g_advancedSettings.m_videoAudioDelayRange;
    if (g_application.m_pPlayer)
      g_application.m_pPlayer->SetAVDelay(g_stSettings.m_currentVideoSettings.m_AudioDelay);
    return true;
    break;
  case ACTION_AUDIO_NEXT_LANGUAGE:
      if (g_application.m_pPlayer->GetAudioStreamCount() == 1)
        return true;

      g_stSettings.m_currentVideoSettings.m_AudioStream++;
      if (g_stSettings.m_currentVideoSettings.m_AudioStream >= g_application.m_pPlayer->GetAudioStreamCount())
        g_stSettings.m_currentVideoSettings.m_AudioStream = 0;
      g_application.m_pPlayer->SetAudioStream(g_stSettings.m_currentVideoSettings.m_AudioStream);    // Set the audio stream to the one selected
    return true;
    break;
  case REMOTE_0:
  case REMOTE_1:
  case REMOTE_2:
  case REMOTE_3:
  case REMOTE_4:
  case REMOTE_5:
  case REMOTE_6:
  case REMOTE_7:
  case REMOTE_8:
  case REMOTE_9:
    ChangetheTimeCode(action.wID);
    return true;
    break;

  case ACTION_ASPECT_RATIO:
    { // toggle the aspect ratio mode (only if the info is onscreen)
      if (m_bShowViewModeInfo)
      {
#ifdef HAS_VIDEO_PLAYBACK
        g_renderManager.SetViewMode(++g_stSettings.m_currentVideoSettings.m_ViewMode);
#endif
      }
      m_bShowViewModeInfo = true;
      m_dwShowViewModeTimeout = timeGetTime();
    }
    return true;
    break;
  case ACTION_SMALL_STEP_BACK:
    {
      int orgpos = (int)g_application.GetTime();
      int triesleft = g_advancedSettings.m_videoSmallStepBackTries;
      int jumpsize = g_advancedSettings.m_videoSmallStepBackSeconds; // secs
      int setpos = (orgpos > jumpsize) ? orgpos - jumpsize : 0; // First jump = 2*jumpsize
      int newpos;
      do
      {
        setpos = (setpos > jumpsize) ? setpos - jumpsize : 0;
        g_application.SeekTime((double)setpos);
        Sleep(g_advancedSettings.m_videoSmallStepBackDelay); // delay to let mplayer finish its seek (in ms)
        newpos = (int)g_application.GetTime();
      }
      while ( (newpos > orgpos - jumpsize) && (setpos > 0) && (--triesleft > 0));

      //Make sure gui items are visible
      g_infoManager.SetDisplayAfterSeek();
    }
    return true;
    break;
  }
  return CGUIWindow::OnAction(action);
}

void CGUIWindowFullScreen::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();

  CGUIProgressControl* pProgress = (CGUIProgressControl*)GetControl(CONTROL_PROGRESS);
  if(pProgress)
  {
    if( pProgress->GetInfo() == 0 || pProgress->GetVisibleCondition() == 0)
    {
      pProgress->SetInfo(PLAYER_PROGRESS);
      pProgress->SetVisibleCondition(PLAYER_DISPLAY_AFTER_SEEK, false);
      pProgress->SetVisible(true);
    }
  }

  CGUILabelControl* pLabel = (CGUILabelControl*)GetControl(LABEL_BUFFERING);
  if(pLabel && pLabel->GetVisibleCondition() == 0)
  {
    pLabel->SetVisibleCondition(PLAYER_CACHING, false);
    pLabel->SetVisible(true);
  }

  pLabel = (CGUILabelControl*)GetControl(LABEL_CURRENT_TIME);
  if(pLabel && pLabel->GetVisibleCondition() == 0)
  {
    pLabel->SetVisibleCondition(PLAYER_DISPLAY_AFTER_SEEK, false);
    pLabel->SetVisible(true);
    pLabel->SetLabel("$INFO(VIDEOPLAYER.TIME) / $INFO(VIDEOPLAYER.DURATION)");
  }
}

bool CGUIWindowFullScreen::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_INIT:
    {
      // check whether we've come back here from a window during which time we've actually
      // stopped playing videos
      if (message.GetParam1() == WINDOW_INVALID && !g_application.IsPlayingVideo())
      { // why are we here if nothing is playing???
        m_gWindowManager.PreviousWindow();
        return true;
      }
      m_bLastRender = false;
      g_infoManager.SetShowInfo(false);
      g_infoManager.SetShowCodec(false);
      m_bShowCurrentTime = false;
      g_infoManager.SetDisplayAfterSeek(0); // Make sure display after seek is off.

      //  Disable nav sounds if spindown is active as they are loaded
      //  from HDD all the time.
      if (
        !g_application.CurrentFileItem().IsHD() &&
        (g_guiSettings.GetInt("harddisk.remoteplayspindown") || g_guiSettings.GetInt("harddisk.spindowntime"))
      )
      {
        if (!g_guiSettings.GetBool("lookandfeel.soundsduringplayback"))
          g_audioManager.Enable(false);
      }


      // setup the brightness, contrast and resolution
      CUtil::SetBrightnessContrastGammaPercent(g_stSettings.m_currentVideoSettings.m_Brightness, g_stSettings.m_currentVideoSettings.m_Contrast, g_stSettings.m_currentVideoSettings.m_Gamma, false);

      // switch resolution
      CSingleLock lock (g_graphicsContext);
      g_graphicsContext.SetFullScreenVideo(true);
#ifdef HAS_VIDEO_PLAYBACK
      RESOLUTION res = g_renderManager.GetResolution();
      g_graphicsContext.SetVideoResolution(res, false, false);
#endif
      lock.Leave();

#ifdef HAS_VIDEO_PLAYBACK
      // make sure renderer is uptospeed
      g_renderManager.Update(false);
#endif
      // now call the base class to load our windows
      CGUIWindow::OnMessage(message);

      m_bShowViewModeInfo = false;

      if (CUtil::IsUsingTTFSubtitles())
      {
        CSingleLock lock (m_fontLock);

        if (m_subtitleFont)
        {
          delete m_subtitleFont;
          m_subtitleFont = NULL;
        }

        CStdString fontPath = "Q:\\Media\\Fonts\\";
        fontPath += g_guiSettings.GetString("subtitles.font");
        m_subtitleFont = g_fontManager.LoadTTF("__subtitle__", fontPath, color[g_guiSettings.GetInt("subtitles.color")], 0, g_guiSettings.GetInt("subtitles.height"), g_guiSettings.GetInt("subtitles.style"));
        if (!m_subtitleFont)
          CLog::Log(LOGERROR, "CGUIWindowFullScreen::OnMessage(WINDOW_INIT) - Unable to load subtitle font");
      }
      else
        m_subtitleFont = NULL;

      return true;
    }
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIWindow::OnMessage(message);

      CGUIDialog *pDialog = (CGUIDialog *)m_gWindowManager.GetWindow(WINDOW_OSD);
      if (pDialog) pDialog->Close(true);

      FreeResources(true);

      CSingleLock lock (g_graphicsContext);
      CUtil::RestoreBrightnessContrastGamma();
      g_graphicsContext.SetFullScreenVideo(false);
      g_graphicsContext.SetVideoResolution(g_guiSettings.m_LookAndFeelResolution, TRUE);
      lock.Leave();

#ifdef HAS_VIDEO_PLAYBACK
      // make sure renderer is uptospeed
      g_renderManager.Update(false);
#endif

      CSingleLock lockFont(m_fontLock);
      if (m_subtitleFont)
      {
        g_fontManager.Unload("__subtitle__");
        m_subtitleFont = NULL;
      }

      if (g_guiSettings.GetBool("lookandfeel.soundsduringplayback"))
        g_audioManager.Enable(true);
      return true;
    }
  case GUI_MSG_SETFOCUS:
  case GUI_MSG_LOSTFOCUS:
    if (message.GetSenderId() != WINDOW_FULLSCREEN_VIDEO) return true;
    break;
  }

  return CGUIWindow::OnMessage(message);
}

bool CGUIWindowFullScreen::OnMouse(const CPoint &point)
{
  if (g_Mouse.bClick[MOUSE_RIGHT_BUTTON])
  { // no control found to absorb this click - go back to GUI
    CAction action;
    action.wID = ACTION_SHOW_GUI;
    OnAction(action);
    return true;
  }
  if (g_Mouse.bClick[MOUSE_LEFT_BUTTON])
  { // no control found to absorb this click - toggle the OSD
    CAction action;
    action.wID = ACTION_SHOW_OSD;
    OnAction(action);
  }
  return true;
}

// Override of Render() - RenderFullScreen() is where the action takes place
// this is called from the rendermanager, normally we won't come this way
// as player thread will handle rendering, and call this itself. 
void CGUIWindowFullScreen::Render()
{
#ifdef HAS_VIDEO_PLAYBACK
  g_renderManager.RenderUpdate(true);
#endif
}

bool CGUIWindowFullScreen::NeedRenderFullScreen()
{
  CSingleLock lock (g_graphicsContext);
  if (g_application.m_pPlayer)
  {
    if (g_application.m_pPlayer->IsPaused() ) return true;
    if (g_application.m_pPlayer->IsCaching() ) return true;
    if (!g_application.m_pPlayer->IsPlaying() ) return true;
  }
  if (g_application.GetPlaySpeed() != 1) return true;
  if (m_timeCodeShow) return true;
  if (g_infoManager.GetBool(PLAYER_SHOWCODEC)) return true;
  if (g_infoManager.GetBool(PLAYER_SHOWINFO)) return true;
  if (IsAnimating(ANIM_TYPE_HIDDEN)) return true; // for the above info conditions
  if (m_bShowViewModeInfo) return true;
  if (m_bShowCurrentTime) return true;
  if (g_infoManager.GetDisplayAfterSeek()) return true;
  if (g_infoManager.GetBool(PLAYER_SEEKBAR, GetID())) return true;
  if (CUtil::IsUsingTTFSubtitles() && g_application.m_pPlayer->GetSubtitleVisible() && m_subtitleFont)
    return true;
  if (m_bLastRender)
  {
    m_bLastRender = false;
  }

  return false;
}

void CGUIWindowFullScreen::RenderFullScreen()
{
  if (g_application.GetPlaySpeed() != 1)
    g_infoManager.SetDisplayAfterSeek();
  if (m_bShowCurrentTime)
    g_infoManager.SetDisplayAfterSeek();

  m_bLastRender = true;
  if (!g_application.m_pPlayer) return ;

  if( g_application.m_pPlayer->IsCaching() )
  {
    g_infoManager.SetDisplayAfterSeek(0); //Make sure these stuff aren't visible now
  }

  //------------------------
  if (g_infoManager.GetBool(PLAYER_SHOWCODEC))
  {
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
      float fCpuUsage = CUtil::CurrentCpuUsage();

      strGeneralFPS.Format("fps:%02.2f cpu:%02.2f %s", g_infoManager.GetFPS(), fCpuUsage, strGeneral.c_str() );
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
    {
      // get the "View Mode" string
      CStdString strTitle = g_localizeStrings.Get(629);
      CStdString strMode = g_localizeStrings.Get(630 + g_stSettings.m_currentVideoSettings.m_ViewMode);
      CStdString strInfo;
      strInfo.Format("%s : %s", strTitle.c_str(), strMode.c_str());
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
                       SrcRect.right - SrcRect.left, SrcRect.bottom - SrcRect.top,
                       DestRect.right - DestRect.left, DestRect.bottom - DestRect.top, g_stSettings.m_fZoomAmount, fAR*g_stSettings.m_fPixelRatio, g_stSettings.m_fPixelRatio);
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW2);
      msg.SetLabel(strSizing);
      OnMessage(msg);
    }
    // show resolution information
    int iResolution = g_graphicsContext.GetVideoResolution();
    {
      CStdString strStatus;
      strStatus.Format("%ix%i %s", g_settings.m_ResInfo[iResolution].iWidth, g_settings.m_ResInfo[iResolution].iHeight, g_settings.m_ResInfo[iResolution].strMode);
      if (g_guiSettings.GetBool("videoplayer.soften"))
        strStatus += "  |  Soften";
      else
        strStatus += "  |  No Soften";

      CStdString strFilter;
      strFilter.Format("  |  Flicker Filter: %i", g_guiSettings.GetInt("videoplayer.flicker"));
      strStatus += strFilter;
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW3);
      msg.SetLabel(strStatus);
      OnMessage(msg);
    }
  }

  RenderTTFSubtitles();

  if (m_timeCodeShow && m_timeCodePosition != 0)
  {
    if ( (timeGetTime() - m_timeCodeTimeout) >= 2500)
    {
      m_timeCodeShow = false;
      m_timeCodePosition = 0;
    }
    CStdString strDispTime = "??:??";

    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW1);
    for (int count = 0; count < m_timeCodePosition; count++)
    {
      if (m_timeCodeStamp[count] == -1)
        strDispTime[count] = ':';
      else
        strDispTime[count] = (char)m_timeCodeStamp[count] + 48;
    }
    strDispTime += "/" + g_infoManager.GetVideoLabel(257) + " [" + g_infoManager.GetVideoLabel(254) + "]"; // duration [ time ]
    msg.SetLabel(strDispTime);
    OnMessage(msg);
  }

  int iSpeed = g_application.GetPlaySpeed();

  if (g_infoManager.GetBool(PLAYER_SHOWCODEC) || m_bShowViewModeInfo)
  {
    SET_CONTROL_VISIBLE(LABEL_ROW1);
    SET_CONTROL_VISIBLE(LABEL_ROW2);
    SET_CONTROL_VISIBLE(LABEL_ROW3);
    SET_CONTROL_VISIBLE(BLUE_BAR);
  }
  else if (m_timeCodeShow)
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

void CGUIWindowFullScreen::RenderTTFSubtitles()
{
  //if ( g_application.GetCurrentPlayer() == EPC_MPLAYER && CUtil::IsUsingTTFSubtitles() && g_application.m_pPlayer->GetSubtitleVisible() && m_subtitleFont)
  if ((g_application.GetCurrentPlayer() == EPC_MPLAYER || g_application.GetCurrentPlayer() == EPC_DVDPLAYER) &&
      CUtil::IsUsingTTFSubtitles() && g_application.m_pPlayer->GetSubtitleVisible())
  {
    CSingleLock lock (m_fontLock);

    if(!m_subtitleFont)
      return;    

    CStdString subtitleText = "";
    if (g_application.m_pPlayer->GetCurrentSubtitle(subtitleText))
    {      
      RESOLUTION res = g_graphicsContext.GetVideoResolution();
      g_graphicsContext.SetScalingResolution(res, 0, 0, false);

      float w;
      float h;
      CStdStringW utf16Sub;
      g_charsetConverter.utf8ToUTF16(subtitleText, utf16Sub, true);
      m_subtitleFont->GetTextExtent(utf16Sub.c_str(), &w, &h);

      float maxWidth = (float) g_settings.m_ResInfo[res].Overscan.right - g_settings.m_ResInfo[res].Overscan.left;
      if (maxWidth*0.9f < w)
      {
        CGUILabelControl::WrapText(utf16Sub, m_subtitleFont, maxWidth*0.9f);
        m_subtitleFont->GetTextExtent(utf16Sub.c_str(), &w, &h);
      }
      float x = (float) maxWidth / 2 + g_settings.m_ResInfo[res].Overscan.left;
      float y = (float) g_settings.m_ResInfo[res].iSubtitles - h;

      float outlinewidth = 3;

      m_subtitleFont->DrawOutlineText(x, y, 0, 0xFF000000, 3, utf16Sub.c_str(), XBFONT_CENTER_X);
    }
  }
}

void CGUIWindowFullScreen::ChangetheTimeCode(DWORD remote)
{
  if (remote >= 58 && remote <= 67) //Make sure it's only for the remote
  {
    m_timeCodeShow = true;
    m_timeCodeTimeout = timeGetTime();
    int itime = remote - 58;
    if (m_timeCodePosition <= 4 && m_timeCodePosition != 2)
    {
      m_timeCodeStamp[m_timeCodePosition++] = itime;
      if (m_timeCodePosition == 2)
        m_timeCodeStamp[m_timeCodePosition++] = -1;
    }
    if (m_timeCodePosition > 4)
    {
      long itotal, ih, im, is = 0;
      ih = (m_timeCodeStamp[0] - 0) * 10;
      ih += (m_timeCodeStamp[1] - 0);
      im = (m_timeCodeStamp[3] - 0) * 10;
      im += (m_timeCodeStamp[4] - 0);
      im *= 60;
      ih *= 3600;
      itotal = ih + im + is;

      if (itotal < g_application.GetTotalTime())
        g_application.SeekTime((double)itotal);

      m_timeCodePosition = 0;
      m_timeCodeShow = false;
    }
  }
}

void CGUIWindowFullScreen::Seek(bool bPlus, bool bLargeStep)
{
  g_application.m_pPlayer->Seek(bPlus, bLargeStep);

  //Make sure gui items are visible
  g_infoManager.SetDisplayAfterSeek();
}

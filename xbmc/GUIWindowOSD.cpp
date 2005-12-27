#include "stdafx.h"
#include "GUIWindowOSD.h"
#include "application.h"
#include "GUIProgressControl.h"
#include "GUISliderControl.h"
#include "GUIToggleButtonControl.h"
#include "guilistcontrol.h"
#include "GUIImage.h"
#include "util.h"
#include "utils/GUIInfoManager.h"
#include "cores/VideoRenderers/RenderManager.h"

#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
#include "SkinInfo.h"
#endif

#define OSD_VIDEOPROGRESS 101
#define OSD_SKIPBWD 210
#define OSD_REWIND 211
#define OSD_STOP 212
#define OSD_PLAY 213
#define OSD_FFWD 214
#define OSD_SKIPFWD 215
#define OSD_MUTE 216 
//#define OSD_SYNC 217 - not used
#define OSD_SUBTITLES 218
#define OSD_BOOKMARKS 219
#define OSD_VIDEO 220
#define OSD_AUDIO 221

#define OSD_VOLUMESLIDER 400

#define OSD_AVDELAY 500
#define OSD_AVDELAY_LABEL 550
#define OSD_AUDIOSTREAM_LIST 501
#define OSD_AUDIO_VOLUME_SLIDER 502
#define OSD_AUDIO_VOLUME_LABEL 552

#define OSD_CREATEBOOKMARK 600
#define OSD_BOOKMARKS_LIST 601
#define OSD_BOOKMARKS_LIST_LABEL 650
#define OSD_CLEARBOOKMARKS 602

#define OSD_VIDEOPOS 700
#define OSD_VIDEOPOS_LABEL 750
#define OSD_NONINTERLEAVED 701
#define OSD_NOCACHE 702
#define OSD_ADJFRAMERATE 703

#define OSD_ZOOM 707
#define OSD_ZOOMLABEL 751
#define OSD_PIXELRATIO 708
#define OSD_PIXELRATIO_LABEL 755
#define OSD_AUTOCROP  709

#define OSD_BRIGHTNESS 704
#define OSD_BRIGHTNESSLABEL 752

#define OSD_CONTRAST 705
#define OSD_CONTRASTLABEL 753

#define OSD_GAMMA 706
#define OSD_GAMMALABEL 754

#define OSD_SUBTITLE_DELAY 800
#define OSD_SUBTITLE_DELAY_LABEL 850
#define OSD_SUBTITLE_ONOFF 801
#define OSD_SUBTITLE_LIST 802

#define OSD_TIMEINFO 100
#define OSD_ESTENDTIME 102
#define OSD_MOVIENAME 103
#define OSD_SUBMENU_BG_VOL 300 
//#define OSD_SUBMENU_BG_SYNC 301 - not used
#define OSD_SUBMENU_BG_SUBTITLES 302
#define OSD_SUBMENU_BG_BOOKMARKS 303
#define OSD_SUBMENU_BG_VIDEO 304
#define OSD_SUBMENU_BG_AUDIO 305
#define OSD_SUBMENU_NIB 350

#define HIDE_CONTROL(dwSenderId, dwControlID) \
{ \
 CGUIMessage msg(GUI_MSG_HIDDEN, dwSenderId, dwControlID); \
 OnMessage(msg); \
}

#define FOCUS_CONTROL(dwSenderId, dwControlID, dwParam) \
{ \
 CGUIMessage msg(GUI_MSG_SETFOCUS, dwSenderId, dwControlID, dwParam); \
 OnMessage(msg); \
}


CGUIWindowOSD::CGUIWindowOSD(void)
    : CGUIDialog(WINDOW_OSD, "VideoOSD.xml")
{
  m_bSubMenuOn = false;
  m_iActiveMenu = 0;
  m_iActiveMenuButtonID = 0;
  m_iCurrentBookmark = 0;
  m_loadOnDemand = false;
}

CGUIWindowOSD::~CGUIWindowOSD(void)
{}

bool CGUIWindowOSD::SubMenuVisible()
{
  return m_bSubMenuOn;
}

void CGUIWindowOSD::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  m_bRelativeCoords = true;
}

void CGUIWindowOSD::Render()
{
  if (g_SkinInfo.GetVersion() < 1.85)
  {
    SetVideoProgress();   // get the percentage of playback complete so far
    Get_TimeInfo();    // show the time elapsed/total playing time
  }
  if ( g_guiSettings.GetInt("MyVideos.OSDTimeout") &&
      !m_gWindowManager.IsWindowActive(WINDOW_DIALOG_VIDEO_OSD_SETTINGS) &&
      !m_gWindowManager.IsWindowActive(WINDOW_DIALOG_VIDEO_BOOKMARKS) )
  {
    if ( (timeGetTime() - m_dwOSDTimeOut) > (DWORD)(g_guiSettings.GetInt("MyVideos.OSDTimeout") * 1000))
    {
      Close();
    }
  }
  CGUIDialog::Render();  // render our controls to the screen
}

bool CGUIWindowOSD::OnAction(const CAction &action)
{
  m_dwOSDTimeOut = timeGetTime();
  // ACTION_SHOW_OSD should take the OSD away too!
  if (action.wID == ACTION_SHOW_OSD)
  {
    Close();
    return true;
  }
  if (g_SkinInfo.GetVersion() < 1.85)
  {
    switch (action.wID)
    {
    case ACTION_SHOW_OSD:
      {
        Close();
        return true;
      }
      break;
    case ACTION_CLOSE_DIALOG:
    case ACTION_PREVIOUS_MENU:
      {
        if (m_bSubMenuOn)      // is sub menu on?
        {
          FOCUS_CONTROL(GetID(), m_iActiveMenuButtonID, 0); // set focus to last menu button
          ToggleSubMenu(0, m_iActiveMenu);      // hide the currently active sub-menu
        }
        else
          Close();
        return true;
      }
      break;

    case ACTION_STOP:
      {
        // push a message through to this window to handle the remote control button
        CGUIMessage msgSet(GUI_MSG_CLICKED, OSD_STOP, OSD_STOP, 0, 0, NULL);
        OnMessage(msgSet);
        return true;
      }
      break;

  /*  case ACTION_OSD_SHOW_VALUE_MAX:
      {
        // push a message through to this window to handle the remote control button
        CGUIMessage msgSet(GUI_MSG_CLICKED, OSD_SKIPFWD, OSD_SKIPFWD, 0, 0, NULL);
        OnMessage(msgSet);
        return true;
      }
      break;

    case ACTION_OSD_SHOW_VALUE_MIN:
      {
        // push a message through to this window to handle the remote control button
        CGUIMessage msgSet(GUI_MSG_CLICKED, OSD_SKIPBWD, OSD_SKIPBWD, 0, 0, NULL);
        OnMessage(msgSet);
        return true;
      }
      break;*/
    }
  }
  return CGUIDialog::OnAction(action);
}

bool CGUIWindowOSD::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:  // fired when OSD is hidden
    {
      if (g_SkinInfo.GetVersion() < 1.85)
      {
      // no need to lock the graphics context here, as we already grab a lock in GUIWindowFullScreen
      // whenever we need to render.
      ClearAudioStreamItems();
      ClearSubTitleItems();
      ClearBookmarkItems();
      }
      // don't save the settings here, it's bad for the hd-spindown feature (causes spinup)
      // settings are saved in FreeResources in GUIWindowFullScreen
      //g_settings.Save();
      // Remove our subdialogs if visible
      CGUIDialog *pDialog = (CGUIDialog *)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_OSD_SETTINGS);
      if (pDialog && pDialog->IsRunning()) pDialog->Close();
      pDialog = (CGUIDialog *)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_BOOKMARKS);
      if (pDialog && pDialog->IsRunning()) pDialog->Close();
      //return true;
    }
    break;

  case GUI_MSG_WINDOW_INIT:  // fired when OSD is shown
    {
      OutputDebugString("OSD:INIT\n");
      CGUIDialog::OnMessage(message);
      //if (g_application.m_pPlayer) g_application.m_pPlayer->ShowOSD(false);
      // position correctly
      int iResolution = g_graphicsContext.GetVideoResolution();
      SetPosition(0, g_settings.m_ResInfo[iResolution].iOSDYOffset);
      m_bSubMenuOn = false;
      m_iActiveMenuButtonID = 0;
      m_iActiveMenu = 0;
      m_dwOSDTimeOut = timeGetTime();
      Reset();

      // Set a dummy time value if the osd is used in screen calibration
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), OSD_TIMEINFO);
      msg.SetLabel("00:00:00/10:00:00");
      OnMessage(msg);

      FOCUS_CONTROL(GetID(), OSD_PLAY, 0); // set focus to play button by default when window is shown

      return true;
    }
    break;


  case GUI_MSG_SETFOCUS:
  case GUI_MSG_LOSTFOCUS:
    {
      if (message.GetControlId() == 13) return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      if (g_SkinInfo.GetVersion() >= 1.85)
        return CGUIDialog::OnMessage(message);

      int iControl = message.GetSenderId();  // get the ID of the control sending us a message

      if (iControl >= OSD_VOLUMESLIDER)  // one of the settings (sub menu) controls is sending us a message
      {
        Handle_ControlSetting(iControl, message.GetParam1());
      }

      if (iControl == OSD_STOP)
      {
        if (m_bSubMenuOn) // sub menu currently active ?
        {
          FOCUS_CONTROL(GetID(), m_iActiveMenuButtonID, 0); // set focus to last menu button
          ToggleSubMenu(0, m_iActiveMenu);      // hide the currently active sub-menu
        }
        OutputDebugString("OSD:STOP\n");
        Close();
        g_application.StopPlaying();      // close our media
      }

      if (iControl == OSD_SKIPBWD)
      {
        g_application.m_pPlayer->Seek(false, true);
        ToggleButton(OSD_SKIPBWD, false);  // pop the button back to it's up state
      }

      if (iControl == OSD_SKIPFWD)
      {
        g_application.m_pPlayer->Seek(true, true);
        ToggleButton(OSD_SKIPFWD, false);  // pop the button back to it's up state
      }

      if (iControl == OSD_MUTE)
      {
        ToggleSubMenu(iControl, OSD_SUBMENU_BG_VOL);   // hide or show the sub-menu
        if (m_bSubMenuOn)          // is sub menu on?
        {
          SET_CONTROL_VISIBLE(OSD_VOLUMESLIDER);  // show the volume control
          FOCUS_CONTROL(GetID(), OSD_VOLUMESLIDER, 0); // set focus to it
        }
        else             // sub menu is off
        {
          FOCUS_CONTROL(GetID(), OSD_MUTE, 0);   // set focus to the mute button
        }
      }

      /* not used
      if (iControl == OSD_SYNC)
      {
       ToggleSubMenu(iControl, OSD_SUBMENU_BG_SYNC);  // hide or show the sub-menu
      }
      */

      if (iControl == OSD_SUBTITLES)
      {
        ToggleSubMenu(iControl, OSD_SUBMENU_BG_SUBTITLES); // hide or show the sub-menu
        if (m_bSubMenuOn)
        {
          // set the controls values
          SetSliderValue( -g_stSettings.m_fSubsDelayRange, g_stSettings.m_fSubsDelayRange, g_application.m_pPlayer->GetSubTitleDelay(), OSD_SUBTITLE_DELAY);
          SetCheckmarkValue(g_application.m_pPlayer->GetSubtitleVisible(), OSD_SUBTITLE_ONOFF);

          // show the controls on this sub menu
          SET_CONTROL_VISIBLE(OSD_SUBTITLE_DELAY);
          SET_CONTROL_VISIBLE(OSD_SUBTITLE_DELAY_LABEL);
          SET_CONTROL_VISIBLE(OSD_SUBTITLE_ONOFF);
          SET_CONTROL_VISIBLE(OSD_SUBTITLE_LIST);

          FOCUS_CONTROL(GetID(), OSD_SUBTITLE_DELAY, 0); // set focus to the first control in our group
          PopulateSubTitles(); // populate the list control with subtitles for this video
        }
      }

      if (iControl == OSD_BOOKMARKS)
      {
        ToggleSubMenu(iControl, OSD_SUBMENU_BG_BOOKMARKS); // hide or show the sub-menu
        if (m_bSubMenuOn)
        {
          // show the controls on this sub menu
          SET_CONTROL_VISIBLE(OSD_CREATEBOOKMARK);
          SET_CONTROL_VISIBLE(OSD_BOOKMARKS_LIST);
          SET_CONTROL_VISIBLE(OSD_BOOKMARKS_LIST_LABEL);
          SET_CONTROL_VISIBLE(OSD_CLEARBOOKMARKS);

          FOCUS_CONTROL(GetID(), OSD_CREATEBOOKMARK, 0); // set focus to the first control in our group
          PopulateBookmarks(); // populate the list control with bookmarks for this video
        }
      }

      if (iControl == OSD_VIDEO)
      {
        ToggleSubMenu(iControl, OSD_SUBMENU_BG_VIDEO);  // hide or show the sub-menu
        if (m_bSubMenuOn)      // is sub menu on?
        {
          // set the controls values
          SetSliderValue(0.0f, 100.0f, g_application.GetPercentage(), OSD_VIDEOPOS);
          SetSliderValue(0.5f, 2.0f, g_stSettings.m_currentVideoSettings.m_CustomZoomAmount, OSD_ZOOM, 0.01f);
          SetSliderValue(0.5f, 2.0f, g_stSettings.m_currentVideoSettings.m_CustomPixelRatio, OSD_PIXELRATIO, 0.01f);
          SetSliderValue(0.0f, 100.0f, (float) g_stSettings.m_currentVideoSettings.m_Brightness, OSD_BRIGHTNESS);
          SetSliderValue(0.0f, 100.0f, (float) g_stSettings.m_currentVideoSettings.m_Contrast, OSD_CONTRAST);
          SetSliderValue(0.0f, 100.0f, (float) g_stSettings.m_currentVideoSettings.m_Gamma, OSD_GAMMA);

          SetCheckmarkValue(g_stSettings.m_currentVideoSettings.m_NonInterleaved, OSD_NONINTERLEAVED);
          SetCheckmarkValue(g_stSettings.m_currentVideoSettings.m_NoCache, OSD_NOCACHE);
          SetCheckmarkValue(g_stSettings.m_currentVideoSettings.m_AdjustFrameRate, OSD_ADJFRAMERATE);
          SetCheckmarkValue(g_stSettings.m_currentVideoSettings.m_Crop, OSD_AUTOCROP);

          // show the controls on this sub menu
          SET_CONTROL_VISIBLE(OSD_VIDEOPOS);
          SET_CONTROL_VISIBLE(OSD_NONINTERLEAVED);
          SET_CONTROL_VISIBLE(OSD_NOCACHE);
          SET_CONTROL_VISIBLE(OSD_ADJFRAMERATE);
          SET_CONTROL_VISIBLE(OSD_VIDEOPOS_LABEL);
          SET_CONTROL_VISIBLE(OSD_ZOOM);
          SET_CONTROL_VISIBLE(OSD_ZOOMLABEL);
          SET_CONTROL_VISIBLE(OSD_PIXELRATIO);
          SET_CONTROL_VISIBLE(OSD_PIXELRATIO_LABEL);
          SET_CONTROL_VISIBLE(OSD_AUTOCROP);
          SET_CONTROL_VISIBLE(OSD_BRIGHTNESS);
          SET_CONTROL_VISIBLE(OSD_BRIGHTNESSLABEL);
          SET_CONTROL_VISIBLE(OSD_CONTRAST);
          SET_CONTROL_VISIBLE(OSD_CONTRASTLABEL);
          SET_CONTROL_VISIBLE(OSD_GAMMA);
          SET_CONTROL_VISIBLE(OSD_GAMMALABEL);
          FOCUS_CONTROL(GetID(), OSD_VIDEOPOS, 0); // set focus to the first control in our group
        }
      }

      if (iControl == OSD_AUDIO)
      {
        ToggleSubMenu( iControl, OSD_SUBMENU_BG_AUDIO);  // hide or show the sub-menu
        if (m_bSubMenuOn)      // is sub menu on?
        {
          // set the controls values
          SetSliderValue( -g_stSettings.m_fAudioDelayRange, g_stSettings.m_fAudioDelayRange, g_application.m_pPlayer->GetAVDelay(), OSD_AVDELAY);
          SetSliderValue( 0.0f, 100.0f, (float)g_application.GetVolume(), OSD_AUDIO_VOLUME_SLIDER);

          // show the controls on this sub menu
          SET_CONTROL_VISIBLE(OSD_AVDELAY);
          SET_CONTROL_VISIBLE(OSD_AVDELAY_LABEL);
          SET_CONTROL_VISIBLE(OSD_AUDIOSTREAM_LIST);
          SET_CONTROL_VISIBLE(OSD_AUDIO_VOLUME_SLIDER);
          SET_CONTROL_VISIBLE(OSD_AUDIO_VOLUME_LABEL);

          // set focus to the first control in our group
          if (GetControl(OSD_AUDIO_VOLUME_SLIDER))
          {
            FOCUS_CONTROL(GetID(), OSD_AUDIO_VOLUME_SLIDER, 0);
          }
          else
          {
            FOCUS_CONTROL(GetID(), OSD_AVDELAY, 0);
          }

          // populate the list control with audio streams for this video
          PopulateAudioStreams();  
        }
      }

      return true;
    }
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIWindowOSD::SetVideoProgress()
{
  if (g_application.m_pPlayer)
  {
    float fValue = g_application.GetPercentage(); // Find out where we are at the moment

    CGUIProgressControl* pControl = (CGUIProgressControl*)GetControl(OSD_VIDEOPROGRESS);
    if (pControl) pControl->SetPercentage(fValue);   // Update our progress bar accordingly ...

    CGUISliderControl* pSlider = (CGUISliderControl*)GetControl(OSD_VIDEOPOS);
    if (pSlider) pSlider->SetFloatValue(fValue);    // Update our position bar accordingly ...

    // Update our volume bar(s) accordingly ...
    int iValue = g_application.GetVolume();
    pSlider = (CGUISliderControl*)GetControl(OSD_VOLUMESLIDER);
    if (pSlider) pSlider->SetPercentage(iValue);
    pSlider = (CGUISliderControl*)GetControl(OSD_AUDIO_VOLUME_SLIDER);
    if (pSlider) pSlider->SetPercentage(iValue);
  }
}

void CGUIWindowOSD::Get_TimeInfo()
{
  if (!g_application.m_pPlayer) return ;
  if (!g_application.m_pPlayer->HasVideo()) return ;

  // get the current playing time position and the total play back time
  SET_CONTROL_LABEL(OSD_TIMEINFO, g_infoManager.GetVideoLabel(254) + "/" + g_infoManager.GetVideoLabel(257)); // time / duration

  // Get the estimated end time
  SYSTEMTIME time;
  WCHAR szETATime[32];
  GetLocalTime(&time);
  unsigned int tmpvar = time.wHour * 3600 + time.wMinute * 60 + (unsigned int)g_application.GetTotalTime() - (unsigned int)g_application.GetTime();
  if (tmpvar != 0)
  {
    int iHour = tmpvar / 3600;
    int iMin = (tmpvar - iHour * 3600) / 60;
    if (iHour >= 24) iHour -= 24;
    swprintf(szETATime, L"%02d:%02d", iHour, iMin);
  }

  CGUIMessage msg1(GUI_MSG_LABEL_SET, GetID(), OSD_ESTENDTIME);
  msg1.SetLabel(szETATime);
  OnMessage(msg1);

  // Set the movie file name label
  CStdString strMovieFileName = CUtil::GetFileName(g_application.CurrentFile());
  CGUIMessage msg2(GUI_MSG_LABEL_SET, GetID(), OSD_MOVIENAME);
  msg2.SetLabel(strMovieFileName);
  OnMessage(msg2);
}

void CGUIWindowOSD::ToggleButton(DWORD iButtonID, bool bSelected)
{
  CGUIControl* pControl = (CGUIControl*)GetControl(iButtonID);

  if (pControl)
  {
    if (bSelected) // do we want the button to appear down?
    {
      CGUIMessage msg(GUI_MSG_SELECTED, GetID(), iButtonID);
      OnMessage(msg);
    }
    else   // or appear up?
    {
      CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), iButtonID);
      OnMessage(msg);
    }
  }
}

void CGUIWindowOSD::ToggleSubMenu(DWORD iButtonID, DWORD iBackID)
{
  int iX, iY;

  CGUIImage* pImgNib = (CGUIImage*)GetControl(OSD_SUBMENU_NIB); // pointer to the nib graphic
  CGUIImage* pImgBG = (CGUIImage*)GetControl(iBackID);   // pointer to the background graphic
  CGUIToggleButtonControl* pButton = (CGUIToggleButtonControl*)GetControl(iButtonID); // pointer to the OSD menu button

  // check to see if we are currently showing a sub-menu and it's position is different
  if (m_bSubMenuOn && iBackID != m_iActiveMenu)
  {
    m_bSubMenuOn = false; // toggle it ready for the new menu requested
    // set the current menu invisible
    SET_CONTROL_HIDDEN(m_iActiveMenu);
  }

  // Get button position
  if (pButton)
  {
    iX = (pButton->GetXPosition() + (pButton->GetWidth() / 2)); // center of button
    iY = pButton->GetYPosition();
  }
  else
  {
    iX = 0;
    iY = 0;
  }

  // Set nib position
  if (pImgNib && pImgBG)
  {
    pImgNib->SetPosition(iX - (pImgNib->GetTextureWidth() / 2), iY - pImgNib->GetTextureHeight());

    if (!m_bSubMenuOn) // sub menu not currently showing?
    {
      pImgNib->SetVisible(true);  // make it show
      pImgBG->SetVisible(true);  // make it show
    }
    else
    {
      pImgNib->SetVisible(false);  // hide it
      pImgBG->SetVisible(false);  // hide it
    }
  }

  m_bSubMenuOn = !m_bSubMenuOn;  // toggle sub menu visible status

  // Set all sub menu controls to hidden
  HIDE_CONTROL(GetID(), OSD_VOLUMESLIDER);
  HIDE_CONTROL(GetID(), OSD_VIDEOPOS);
  HIDE_CONTROL(GetID(), OSD_VIDEOPOS_LABEL);
  HIDE_CONTROL(GetID(), OSD_NONINTERLEAVED);
  HIDE_CONTROL(GetID(), OSD_NOCACHE);
  HIDE_CONTROL(GetID(), OSD_ADJFRAMERATE);
  HIDE_CONTROL(GetID(), OSD_AVDELAY);
  HIDE_CONTROL(GetID(), OSD_AVDELAY_LABEL);
  HIDE_CONTROL(GetID(), OSD_AUDIOSTREAM_LIST);
  HIDE_CONTROL(GetID(), OSD_AUDIO_VOLUME_SLIDER);
  HIDE_CONTROL(GetID(), OSD_AUDIO_VOLUME_LABEL);

  HIDE_CONTROL(GetID(), OSD_ZOOM);
  HIDE_CONTROL(GetID(), OSD_ZOOMLABEL);
  HIDE_CONTROL(GetID(), OSD_PIXELRATIO);
  HIDE_CONTROL(GetID(), OSD_PIXELRATIO_LABEL);
  HIDE_CONTROL(GetID(), OSD_AUTOCROP);

  HIDE_CONTROL(GetID(), OSD_BRIGHTNESS);
  HIDE_CONTROL(GetID(), OSD_BRIGHTNESSLABEL);

  HIDE_CONTROL(GetID(), OSD_GAMMA);
  HIDE_CONTROL(GetID(), OSD_GAMMALABEL);

  HIDE_CONTROL(GetID(), OSD_CONTRAST);
  HIDE_CONTROL(GetID(), OSD_CONTRASTLABEL);

  HIDE_CONTROL(GetID(), OSD_CREATEBOOKMARK);
  HIDE_CONTROL(GetID(), OSD_BOOKMARKS_LIST);
  HIDE_CONTROL(GetID(), OSD_BOOKMARKS_LIST_LABEL);
  HIDE_CONTROL(GetID(), OSD_CLEARBOOKMARKS);
  HIDE_CONTROL(GetID(), OSD_SUBTITLE_DELAY);
  HIDE_CONTROL(GetID(), OSD_SUBTITLE_DELAY_LABEL);
  HIDE_CONTROL(GetID(), OSD_SUBTITLE_ONOFF);
  HIDE_CONTROL(GetID(), OSD_SUBTITLE_LIST);

  // Reset the other buttons back to up except the one that's active
  if (iButtonID != OSD_MUTE) ToggleButton(OSD_MUTE, false);
  //if (iButtonID != OSD_SYNC) ToggleButton(OSD_SYNC, false); - not used
  if (iButtonID != OSD_SUBTITLES) ToggleButton(OSD_SUBTITLES, false);
  if (iButtonID != OSD_BOOKMARKS) ToggleButton(OSD_BOOKMARKS, false);
  if (iButtonID != OSD_VIDEO) ToggleButton(OSD_VIDEO, false);
  if (iButtonID != OSD_AUDIO) ToggleButton(OSD_AUDIO, false);

  m_iActiveMenu = iBackID;
  m_iActiveMenuButtonID = iButtonID;
}

void CGUIWindowOSD::SetSliderValue(float fMin, float fMax, float fValue, DWORD iControlID, float fInterval)
{
  CGUISliderControl* pControl = (CGUISliderControl*)GetControl(iControlID);

  if (pControl)
  {
    switch (pControl->GetType())
    {
    case SPIN_CONTROL_TYPE_FLOAT:
      pControl->SetFloatRange(fMin, fMax);
      pControl->SetFloatValue(fValue);
      if (fInterval) pControl->SetFloatInterval(fInterval);
      break;

    case SPIN_CONTROL_TYPE_INT:
      pControl->SetRange((int) fMin, (int) fMax);
      pControl->SetIntValue((int) fValue);
      break;

    default:
      pControl->SetPercentage((int) fValue);
      break;
    }
  }
}

void CGUIWindowOSD::SetCheckmarkValue(BOOL bValue, DWORD iControlID)
{
  if (bValue)
  {
    CGUIMessage msg(GUI_MSG_SELECTED, GetID(), iControlID, 0, 0, NULL);
    OnMessage(msg);
  }
  else
  {
    CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), iControlID, 0, 0, NULL);
    OnMessage(msg);
  }
}

extern void xbox_audio_switch_channel(int iAudioStream, bool bAudioOnAllSpeakers); //lowlevel audio

void CGUIWindowOSD::Handle_ControlSetting(DWORD iControlID, DWORD wID)
{
  const CStdString& strMovie = g_application.CurrentFile();
  CVideoDatabase dbs;
  VECBOOKMARKS bookmarks;

  switch (iControlID)
  {
  case OSD_AUDIO_VOLUME_SLIDER:
  case OSD_VOLUMESLIDER:
    {
      CGUISliderControl* pControl = (CGUISliderControl*)GetControl(iControlID);
      if (pControl)
      {
        // Set the global volume setting to the percentage requested
        int iPercentage = pControl->GetPercentage();
        g_application.SetVolume(iPercentage);
      }
    }
    break;

  case OSD_VIDEOPOS:
    {
      CGUISliderControl* pControl = (CGUISliderControl*)GetControl(iControlID);
      if (pControl)
      {
        // Set mplayer's seek position to the percentage requested by the user
        g_application.SeekPercentage(pControl->GetFloatValue());
      }
    }
    break;
  case OSD_ZOOM:
    {
      CGUISliderControl* pControl = (CGUISliderControl*)GetControl(iControlID);
      if (pControl)
      {
        g_stSettings.m_currentVideoSettings.m_CustomZoomAmount = pControl->GetFloatValue();
        g_renderManager.SetViewMode(VIEW_MODE_CUSTOM);
      }
    }
    break;
  case OSD_PIXELRATIO:
    {
      CGUISliderControl* pControl = (CGUISliderControl*)GetControl(iControlID);
      if (pControl)
      {
        g_stSettings.m_currentVideoSettings.m_CustomPixelRatio = (float)pControl->GetFloatValue();
        g_renderManager.SetViewMode(VIEW_MODE_CUSTOM);
      }
    }
    break;
  case OSD_BRIGHTNESS:
    {
      CGUISliderControl* pControl = (CGUISliderControl*)GetControl(iControlID);
      if (pControl)
      {
        // Set mplayer's brightness setting position to the percentage requested by the user
        g_stSettings.m_currentVideoSettings.m_Brightness = pControl->GetIntValue();
        CUtil::SetBrightnessContrastGammaPercent(g_stSettings.m_currentVideoSettings.m_Brightness, g_stSettings.m_currentVideoSettings.m_Contrast, g_stSettings.m_currentVideoSettings.m_Gamma, true);
      }
    }
    break;
  case OSD_CONTRAST:
    {
      CGUISliderControl* pControl = (CGUISliderControl*)GetControl(iControlID);
      if (pControl)
      {
        // Set mplayer's contrast setting to the percentage requested by the user
        g_stSettings.m_currentVideoSettings.m_Contrast = pControl->GetIntValue();
        CUtil::SetBrightnessContrastGammaPercent(g_stSettings.m_currentVideoSettings.m_Brightness, g_stSettings.m_currentVideoSettings.m_Contrast, g_stSettings.m_currentVideoSettings.m_Gamma, true);
      }
    }
    break;
  case OSD_GAMMA:
    {
      CGUISliderControl* pControl = (CGUISliderControl*)GetControl(iControlID);
      if (pControl)
      {
        // Set mplayer's gamma setting to the percentage requested by the user
        g_stSettings.m_currentVideoSettings.m_Gamma = pControl->GetIntValue();
        CUtil::SetBrightnessContrastGammaPercent(g_stSettings.m_currentVideoSettings.m_Brightness, g_stSettings.m_currentVideoSettings.m_Contrast, g_stSettings.m_currentVideoSettings.m_Gamma, true);
      }
    }
    break;

  case OSD_AUDIOSTREAM_LIST:
    {
      if (wID) // check to see if list control has an action ID, remote can cause 0 based events
      {
        // first check if it's a stereo track that we can change between stereo, left and right
        if (g_application.m_pPlayer->GetAudioStreamCount() == 1)
        {
          CGUIListControl *pList = (CGUIListControl *)GetControl(OSD_AUDIOSTREAM_LIST);
          if (pList->GetNumItems() == 3)
          { // we're in the case we want - call the code to switch channels etc.
            // update the screen setting...
            g_stSettings.m_currentVideoSettings.m_AudioStream = -1 - pList->GetSelectedItem();
            PopulateAudioStreams();
            // call monkeyh1's code here...
            bool bAudioOnAllSpeakers = (g_guiSettings.GetInt("AudioOutput.Mode") == AUDIO_DIGITAL) && g_guiSettings.GetBool("VideoPlayer.OutputToAllSpeakers");
            xbox_audio_switch_channel(pList->GetSelectedItem(), bAudioOnAllSpeakers);
            break;
          }
        }
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), OSD_AUDIOSTREAM_LIST, 0, 0, NULL);
        OnMessage(msg);
        // only change the audio stream if a different one has been asked for
        if (g_application.m_pPlayer->GetAudioStream() != msg.GetParam1())
        {
          g_application.m_pPlayer->SetAudioStream(msg.GetParam1());    // Set the audio stream to the one selected
          m_bSubMenuOn = false;          // hide the sub menu
        }
      }
    }
    break;

  case OSD_AVDELAY:
    {
      CGUISliderControl* pControl = (CGUISliderControl*)GetControl(iControlID);
      if (pControl)
      {
        // Set the AV Delay
        g_stSettings.m_currentVideoSettings.m_AudioDelay = pControl->GetFloatValue();
        g_application.m_pPlayer->SetAVDelay(pControl->GetFloatValue());
      }
    }
    break;

  case OSD_NONINTERLEAVED:
    {
      g_stSettings.m_currentVideoSettings.m_NonInterleaved = !g_stSettings.m_currentVideoSettings.m_NonInterleaved;
      m_bSubMenuOn = false;          // hide the sub menu
      OutputDebugString("OSD:RESTART2\n");
      g_application.Restart(true);        // restart to make the new setting active
    }
    break;

  case OSD_NOCACHE:
    {
      g_stSettings.m_currentVideoSettings.m_NoCache = !g_stSettings.m_currentVideoSettings.m_NoCache;
      m_bSubMenuOn = false;          // hide the sub menu
      OutputDebugString("OSD:RESTART3\n");
      g_application.Restart(true);        // restart to make the new setting active
    }
    break;

  case OSD_ADJFRAMERATE:
    {
      //g_guiSettings.ToggleBool("MyVideos.FrameRateConversions");
      g_stSettings.m_currentVideoSettings.m_AdjustFrameRate = !g_stSettings.m_currentVideoSettings.m_AdjustFrameRate;
      m_bSubMenuOn = false;          // hide the sub menu
      OutputDebugString("OSD:RESTART4\n");
      g_application.Restart(true);        // restart to make the new setting active
    }
    break;

  case OSD_AUTOCROP:
    {
      g_stSettings.m_currentVideoSettings.m_Crop = !g_stSettings.m_currentVideoSettings.m_Crop;
      g_renderManager.AutoCrop(g_stSettings.m_currentVideoSettings.m_Crop);
    }
    break;

  case OSD_CREATEBOOKMARK:
    {
      CBookmark bookmark;
      bookmark.timeInSeconds = (int)g_application.GetTime();
      bookmark.thumbNailImage.Empty();
      dbs.Open();             // open the bookmark d/b
      dbs.AddBookMarkToMovie(strMovie, bookmark);    // add the current timestamp
      dbs.Close();            // close the d/b
      PopulateBookmarks();          // refresh our list control
    }
    break;

  case OSD_BOOKMARKS_LIST:
    {
      if (wID) // check to see if list control has an action ID, remote can cause 0 based events
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), OSD_BOOKMARKS_LIST, 0, 0, NULL);
        OnMessage(msg);
        m_iCurrentBookmark = msg.GetParam1();     // index of bookmark user selected

        dbs.Open();            // open the bookmark d/b
        dbs.GetBookMarksForMovie(strMovie, bookmarks);   // load the stored bookmarks
        dbs.Close();           // close the d/b
        if (bookmarks.size() <= 0) return ;      // no bookmarks? leave if so ...

        g_application.SeekTime((double)bookmarks[m_iCurrentBookmark].timeInSeconds); // set mplayers play position
        PopulateBookmarks();
      }
    }
    break;

  case OSD_CLEARBOOKMARKS:
    {
      dbs.Open();            // open the bookmark d/b
      dbs.ClearBookMarksOfMovie(strMovie);     // empty the bookmarks table for this movie
      dbs.Close();           // close the d/b
      m_iCurrentBookmark = 0;         // reset current bookmark
      PopulateBookmarks();         // refresh our list control
    }
    break;

  case OSD_SUBTITLE_DELAY:
    {
      CGUISliderControl* pControl = (CGUISliderControl*)GetControl(iControlID);
      if (pControl)
      {
        // Set the subtitle delay
        g_stSettings.m_currentVideoSettings.m_SubtitleDelay = pControl->GetFloatValue();
        g_application.m_pPlayer->SetSubTitleDelay(pControl->GetFloatValue());
      }
    }
    break;

  case OSD_SUBTITLE_ONOFF:
    {
      // Toggle subtitles
      g_application.m_pPlayer->SetSubtitleVisible(!g_application.m_pPlayer->GetSubtitleVisible());
      PopulateSubTitles(); //Redrew subtitle menu
    }
    break;

  case OSD_SUBTITLE_LIST:
    {
      if (wID) // check to see if list control has an action ID, remote can cause 0 based events
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), OSD_SUBTITLE_LIST, 0, 0, NULL);
        OnMessage(msg); // retrieve the selected list item
        g_application.m_pPlayer->SetSubtitle(msg.GetParam1()); // set the current subtitle
        g_application.m_pPlayer->SetSubtitleVisible(true);
        SetCheckmarkValue(true, OSD_SUBTITLE_ONOFF);
        PopulateSubTitles();
      }
    }
    break;
  }
}

void CGUIWindowOSD::PopulateBookmarks()
{
  VECBOOKMARKS bookmarks;
  const CStdString& strMovie = g_application.CurrentFile();
  CVideoDatabase dbs;

  // lock our display (to prevent rendering while we're altering the contents of the listcontrol)
  g_graphicsContext.Lock();
  // tell the list control not to show the page x/y spin control
  CGUIListControl* pControl = (CGUIListControl*)GetControl(OSD_BOOKMARKS_LIST);
  if (pControl) pControl->SetPageControlVisible(false);

  // open the d/b and retrieve the bookmarks for the current movie
  dbs.Open();
  dbs.GetBookMarksForMovie(strMovie, bookmarks);
  dbs.Close();

  // empty the list ready for population
  ClearBookmarkItems();

  // cycle through each stored bookmark and add it to our list control
  for (int i = 0; i < (int)(bookmarks.size()); ++i)
  {
    CStdString strItem;
    _int64 lPTS1 = (_int64)(10 * bookmarks[i].timeInSeconds);
    int hh = (int)(lPTS1 / 36000) % 100;
    int mm = (int)((lPTS1 / 600) % 60);
    int ss = (int)((lPTS1 / 10) % 60);
    strItem.Format("%2i.   %02.2i:%02.2i:%02.2i", i + 1, hh, mm, ss);

    // create a list item object to add to the list
    CGUIListItem* pItem = new CGUIListItem();
    pItem->SetLabel(strItem);

    // add it ...
    CGUIMessage msg2(GUI_MSG_LABEL_ADD, GetID(), OSD_BOOKMARKS_LIST, 0, 0, (void*)pItem);
    OnMessage(msg2);
    m_vecBookmarksItems.push_back(pItem);
  }

  // set the currently active bookmark as the selected item in the list control
  CGUIMessage msgSet(GUI_MSG_ITEM_SELECT, GetID(), OSD_BOOKMARKS_LIST, m_iCurrentBookmark, 0, NULL);
  OnMessage(msgSet);
  g_graphicsContext.Unlock();
}

void CGUIWindowOSD::ClearBookmarkItems()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), OSD_BOOKMARKS_LIST, 0, 0, NULL);
  OnMessage(msg);

  for (int i = 0; i < (int)m_vecBookmarksItems.size(); ++i)
  {
    CGUIListItem* pItem = m_vecBookmarksItems[i];
    delete pItem;
  }

  m_vecBookmarksItems.erase(m_vecBookmarksItems.begin(), m_vecBookmarksItems.end());
}

void CGUIWindowOSD::PopulateAudioStreams()
{
  // get the number of audio strams for the current movie
  int iValue = g_application.m_pPlayer->GetAudioStreamCount();
  int iCurrent = g_application.m_pPlayer->GetAudioStream();

  // lock our display so that render isn't called while we are updating the list control
  g_graphicsContext.Lock();
  // tell the list control not to show the page x/y spin control
  CGUIListControl* pControl = (CGUIListControl*)GetControl(OSD_AUDIOSTREAM_LIST);
  if (pControl) pControl->SetPageControlVisible(false);

  // empty the list ready for population
  ClearAudioStreamItems();

  CStdString strLabel = g_localizeStrings.Get(460).c_str();     // "Audio Stream"
  CStdString strActiveLabel = (WCHAR*)g_localizeStrings.Get(461).c_str();  // "[active]"

  // check if we have a single, stereo stream, and if so, allow us to split into
  // left, right or both
  if (iValue == 1)
  {
    CStdString strAudioInfo;
    g_application.m_pPlayer->GetAudioInfo(strAudioInfo);
    int iNumChannels = atoi(strAudioInfo.Right(strAudioInfo.size() - strAudioInfo.Find("chns:") - 5).c_str());
    CStdString strAudioCodec = strAudioInfo.Mid(7, strAudioInfo.Find(") VBR") - 5);
    bool bDTS = strstr(strAudioCodec.c_str(), "DTS") != 0;
    bool bAC3 = strstr(strAudioCodec.c_str(), "AC3") != 0;
    if (iNumChannels == 2 && !(bDTS || bAC3))
    { // ok, enable these options
/*      if (g_stSettings.m_currentVideoSettings.m_AudioStream == -1)
      { // default to stereo stream
        g_stSettings.m_currentVideoSettings.m_AudioStream = 0;
      }*/
      for (int i = 0; i < 3; i++)
      {
        CStdString strLabel = g_localizeStrings.Get(13320 + i);
        if (i == -g_stSettings.m_currentVideoSettings.m_AudioStream - 1)
          strLabel += " " + strActiveLabel;

        CGUIListItem* pItem = new CGUIListItem();
        pItem->SetLabel(strLabel);
        CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), OSD_AUDIOSTREAM_LIST, 0, 0, (void*)pItem);
        OnMessage(msg);
        m_vecAudioStreamItems.push_back(pItem);
      }
      // set the current active audio stream as the selected item in the list control
      CGUIMessage msgSet(GUI_MSG_ITEM_SELECT, GetID(), OSD_AUDIOSTREAM_LIST, -1 - g_stSettings.m_currentVideoSettings.m_AudioStream, 0, NULL);
      OnMessage(msgSet);
      g_graphicsContext.Unlock();
      return ;
    }
  }

  // cycle through each audio stream and add it to our list control
  for (int i = 0; i < iValue; ++i)
  {
    CStdString strItem;
    g_application.m_pPlayer->GetAudioStreamName(i, strItem);
    if (strItem.length() == 0)
      strItem.Format(strLabel + " %2i", i + 1);

    if (iCurrent == i)
    {
      strItem += " ";
      strItem += strActiveLabel; // formats to 'Audio Stream X [active]'
    }

    // create a list item object to add to the list
    CGUIListItem* pItem = new CGUIListItem();
    pItem->SetLabel(strItem);

    // add it ...
    CGUIMessage msg2(GUI_MSG_LABEL_ADD, GetID(), OSD_AUDIOSTREAM_LIST, 0, 0, (void*)pItem);
    OnMessage(msg2);
    m_vecAudioStreamItems.push_back(pItem);
  }

  // set the current active audio stream as the selected item in the list control
  CGUIMessage msgSet(GUI_MSG_ITEM_SELECT, GetID(), OSD_AUDIOSTREAM_LIST, g_stSettings.m_currentVideoSettings.m_AudioStream, 0, NULL);
  OnMessage(msgSet);
  g_graphicsContext.Unlock();
}

void CGUIWindowOSD::ClearAudioStreamItems()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), OSD_AUDIOSTREAM_LIST, 0, 0, NULL);
  OnMessage(msg);

  for (int i = 0; i < (int)m_vecAudioStreamItems.size(); ++i)
  {
    CGUIListItem* pItem = m_vecAudioStreamItems[i];
    delete pItem;
  }

  m_vecAudioStreamItems.erase(m_vecAudioStreamItems.begin(), m_vecAudioStreamItems.end());
}

void CGUIWindowOSD::PopulateSubTitles()
{
  g_graphicsContext.Lock();
  CLog::DebugLog("Populating SubItems");
  // get the number of subtitles in the current movie
  int bEnabled = g_application.m_pPlayer->GetSubtitleVisible();
  int iValue = g_application.m_pPlayer->GetSubtitleCount();
  int iCurrent = g_application.m_pPlayer->GetSubtitle();

  CLog::DebugLog("total subs:%i current sub:%i", iValue, iCurrent);

  // tell the list control not to show the page x/y spin control
  CGUIListControl* pControl = (CGUIListControl*)GetControl(OSD_SUBTITLE_LIST);
  if (pControl) pControl->SetPageControlVisible(false);

  // empty the list ready for population
  ClearSubTitleItems();

  CStdString strLabel = g_localizeStrings.Get(462).c_str();     // "Subtitle"
  CStdString strActiveLabel = (WCHAR*)g_localizeStrings.Get(461).c_str();  // "[active]"



  // cycle through each subtitle and add it to our list control
  for (int i = 0; i < iValue; ++i)
  {
    CStdString strItem;
    g_application.m_pPlayer->GetSubtitleName(i, strItem);
    if (strItem.length() == 0)
      strItem.Format(strLabel + " %2i", i + 1);

    if (iCurrent == i && bEnabled)
    {
      strItem += " ";
      strItem += strActiveLabel; // formats to 'Subtitle X [active]'
    }


    // create a list item object to add to the list
    CGUIListItem* pItem = new CGUIListItem();
    pItem->SetLabel(strItem);

    // add it ...
    CGUIMessage msg2(GUI_MSG_LABEL_ADD, GetID(), OSD_SUBTITLE_LIST, 0, 0, (void*)pItem);
    OnMessage(msg2);
    m_vecSubTitlesItems.push_back(pItem);
  }

  // set the current active subtitle as the selected item in the list control
  CGUIMessage msgSet(GUI_MSG_ITEM_SELECT, GetID(), OSD_SUBTITLE_LIST, iCurrent, 0, NULL);
  OnMessage(msgSet);
  g_graphicsContext.Unlock();
  CLog::DebugLog("PopulateSubtitles done");
}

void CGUIWindowOSD::ClearSubTitleItems()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), OSD_SUBTITLE_LIST, 0, 0, NULL);
  OnMessage(msg);

  for (int i = 0; i < (int)m_vecSubTitlesItems.size(); ++i)
  {
    CGUIListItem* pItem = m_vecSubTitlesItems[i];
    delete pItem;
  }

  m_vecSubTitlesItems.erase(m_vecSubTitlesItems.begin(), m_vecSubTitlesItems.end());
}

void CGUIWindowOSD::Reset()
{
  // Set all sub menu controls to hidden

  HIDE_CONTROL(GetID(), OSD_SUBMENU_BG_AUDIO);
  HIDE_CONTROL(GetID(), OSD_SUBMENU_BG_VIDEO);
  HIDE_CONTROL(GetID(), OSD_SUBMENU_BG_BOOKMARKS);
  HIDE_CONTROL(GetID(), OSD_SUBMENU_BG_SUBTITLES);
  HIDE_CONTROL(GetID(), OSD_SUBMENU_BG_VOL);


  HIDE_CONTROL(GetID(), OSD_VOLUMESLIDER);
  HIDE_CONTROL(GetID(), OSD_VIDEOPOS);
  HIDE_CONTROL(GetID(), OSD_VIDEOPOS_LABEL);
  HIDE_CONTROL(GetID(), OSD_AUDIOSTREAM_LIST);
  HIDE_CONTROL(GetID(), OSD_AVDELAY);
  HIDE_CONTROL(GetID(), OSD_NONINTERLEAVED);
  HIDE_CONTROL(GetID(), OSD_NOCACHE);
  HIDE_CONTROL(GetID(), OSD_ADJFRAMERATE);
  HIDE_CONTROL(GetID(), OSD_AVDELAY_LABEL);
  HIDE_CONTROL(GetID(), OSD_AUDIO_VOLUME_SLIDER);
  HIDE_CONTROL(GetID(), OSD_AUDIO_VOLUME_LABEL);

  HIDE_CONTROL(GetID(), OSD_ZOOM);
  HIDE_CONTROL(GetID(), OSD_ZOOMLABEL);
  HIDE_CONTROL(GetID(), OSD_PIXELRATIO);
  HIDE_CONTROL(GetID(), OSD_PIXELRATIO_LABEL);
  HIDE_CONTROL(GetID(), OSD_AUTOCROP);

  HIDE_CONTROL(GetID(), OSD_BRIGHTNESS);
  HIDE_CONTROL(GetID(), OSD_BRIGHTNESSLABEL);

  HIDE_CONTROL(GetID(), OSD_GAMMA);
  HIDE_CONTROL(GetID(), OSD_GAMMALABEL);

  HIDE_CONTROL(GetID(), OSD_CONTRAST);
  HIDE_CONTROL(GetID(), OSD_CONTRASTLABEL);

  HIDE_CONTROL(GetID(), OSD_CREATEBOOKMARK);
  HIDE_CONTROL(GetID(), OSD_BOOKMARKS_LIST);
  HIDE_CONTROL(GetID(), OSD_BOOKMARKS_LIST_LABEL);
  HIDE_CONTROL(GetID(), OSD_CLEARBOOKMARKS);
  HIDE_CONTROL(GetID(), OSD_SUBTITLE_DELAY);
  HIDE_CONTROL(GetID(), OSD_SUBTITLE_DELAY_LABEL);
  HIDE_CONTROL(GetID(), OSD_SUBTITLE_ONOFF);
  HIDE_CONTROL(GetID(), OSD_SUBTITLE_LIST);

  ToggleButton(OSD_MUTE, false);
  ToggleButton(OSD_SUBTITLES, false);
  ToggleButton(OSD_BOOKMARKS, false);
  ToggleButton(OSD_VIDEO, false);
  ToggleButton(OSD_AUDIO, false);


}

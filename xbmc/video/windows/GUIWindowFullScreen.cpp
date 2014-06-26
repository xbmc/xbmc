/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "threads/SystemClock.h"
#include "system.h"
#include "GUIWindowFullScreen.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "Util.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#include "GUIInfoManager.h"
#include "guilib/GUIProgressControl.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/GUILabelControl.h"
#include "video/dialogs/GUIDialogVideoOSD.h"
#include "guilib/GUIFontManager.h"
#include "guilib/GUITextLayout.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/Key.h"
#include "video/dialogs/GUIDialogFullScreenInfo.h"
#include "dialogs/GUIDialogNumeric.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "video/VideoReferenceClock.h"
#include "settings/AdvancedSettings.h"
#include "utils/CPUInfo.h"
#include "guilib/LocalizeStrings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "XBDateTime.h"
#include "input/ButtonTranslator.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "windowing/WindowingFactory.h"
#include "cores/IPlayer.h"
#include "filesystem/File.h"

#include <stdio.h>
#include <algorithm>
#if defined(TARGET_DARWIN)
#include "linux/LinuxResourceCounter.h"
#endif

using namespace PVR;

#define BLUE_BAR                          0
#define LABEL_ROW1                       10
#define LABEL_ROW2                       11
#define LABEL_ROW3                       12
#define CONTROL_GROUP_CHOOSER            503

//Displays current position, visible after seek or when forced
//Alt, use conditional visibility Player.DisplayAfterSeek
#define LABEL_CURRENT_TIME               22

//Displays when video is rebuffering
//Alt, use conditional visibility Player.IsCaching
#define LABEL_BUFFERING                  24

//Progressbar used for buffering status and after seeking
#define CONTROL_PROGRESS                 23

#if defined(TARGET_DARWIN)
static CLinuxResourceCounter m_resourceCounter;
#endif

CGUIWindowFullScreen::CGUIWindowFullScreen(void)
    : CGUIWindow(WINDOW_FULLSCREEN_VIDEO, "VideoFullScreen.xml")
{
  m_timeCodeStamp[0] = 0;
  m_timeCodePosition = 0;
  m_timeCodeShow = false;
  m_timeCodeTimeout = 0;
  m_bShowViewModeInfo = false;
  m_dwShowViewModeTimeout = 0;
  m_bShowCurrentTime = false;
  m_bGroupSelectShow = false;
  m_loadType = KEEP_IN_MEMORY;
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

bool CGUIWindowFullScreen::OnAction(const CAction &action)
{
  if (m_timeCodePosition > 0 && action.GetButtonCode())
  { // check whether we have a mapping in our virtual videotimeseek "window" and have a select action
    CKey key(action.GetButtonCode());
    CAction timeSeek = CButtonTranslator::GetInstance().GetAction(WINDOW_VIDEO_TIME_SEEK, key, false);
    if (timeSeek.GetID() == ACTION_SELECT_ITEM)
    {
      SeekToTimeCodeStamp(SEEK_ABSOLUTE);
      return true;
    }
  }

  switch (action.GetID())
  {
  case ACTION_SHOW_OSD:
    ToggleOSD();
    return true;

  case ACTION_TRIGGER_OSD:
    TriggerOSD();
    return true;

  case ACTION_SHOW_GUI:
    {
      // switch back to the menu
      g_windowManager.PreviousWindow();
      return true;
    }
    break;

  case ACTION_PLAYER_PLAY:
  case ACTION_PAUSE:
    if (m_timeCodePosition > 0)
    {
      SeekToTimeCodeStamp(SEEK_ABSOLUTE);
      return true;
    }
    break;

  case ACTION_STEP_BACK:
    if (m_timeCodePosition > 0)
      SeekToTimeCodeStamp(SEEK_RELATIVE, SEEK_BACKWARD);
    else
      g_application.m_pPlayer->Seek(false, false);
    return true;

  case ACTION_STEP_FORWARD:
    if (m_timeCodePosition > 0)
      SeekToTimeCodeStamp(SEEK_RELATIVE, SEEK_FORWARD);
    else
      g_application.m_pPlayer->Seek(true, false);
    return true;

  case ACTION_BIG_STEP_BACK:
  case ACTION_CHAPTER_OR_BIG_STEP_BACK:
    if (m_timeCodePosition > 0)
      SeekToTimeCodeStamp(SEEK_RELATIVE, SEEK_BACKWARD);
    else
      g_application.m_pPlayer->Seek(false, true, action.GetID() == ACTION_CHAPTER_OR_BIG_STEP_BACK);
    return true;

  case ACTION_BIG_STEP_FORWARD:
  case ACTION_CHAPTER_OR_BIG_STEP_FORWARD:
    if (m_timeCodePosition > 0)
      SeekToTimeCodeStamp(SEEK_RELATIVE, SEEK_FORWARD);
    else
      g_application.m_pPlayer->Seek(true, true, action.GetID() == ACTION_CHAPTER_OR_BIG_STEP_FORWARD);
    return true;

  case ACTION_NEXT_SCENE:
    if (g_application.m_pPlayer->SeekScene(true))
      g_infoManager.SetDisplayAfterSeek();
    return true;
    break;

  case ACTION_PREV_SCENE:
    if (g_application.m_pPlayer->SeekScene(false))
      g_infoManager.SetDisplayAfterSeek();
    return true;
    break;

  case ACTION_SHOW_OSD_TIME:
    m_bShowCurrentTime = !m_bShowCurrentTime;
    if(!m_bShowCurrentTime)
      g_infoManager.SetDisplayAfterSeek(0); //Force display off
    g_infoManager.SetShowTime(m_bShowCurrentTime);
    return true;
    break;

  case ACTION_SHOW_INFO:
    {
      CGUIDialogFullScreenInfo* pDialog = (CGUIDialogFullScreenInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_FULLSCREEN_INFO);
      if (pDialog)
      {
        CFileItem item(g_application.CurrentFileItem());
        pDialog->DoModal();
        return true;
      }
      break;
    }

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
    {
      if (g_application.CurrentFileItem().IsLiveTV())
      {
        if(CPVRManager::Get().IsPlaying())
        {
          // pvr client addon
          CPVRChannelPtr playingChannel;
          if(!g_PVRManager.GetCurrentChannel(playingChannel))
            return false;

          if (action.GetID() == REMOTE_0)
          {
            CPVRChannelGroupPtr group = g_PVRChannelGroups->GetPreviousPlayedGroup();
            if (group)
            {
              g_PVRManager.SetPlayingGroup(group);
              CFileItemPtr fileItem = group->GetLastPlayedChannel(playingChannel->ChannelID());
              if (fileItem && fileItem->HasPVRChannelInfoTag())
              {
                CLog::Log(LOGDEBUG, "%s - switch to channel number %d", __FUNCTION__, fileItem->GetPVRChannelInfoTag()->ChannelNumber());
                g_application.OnAction(CAction(ACTION_CHANNEL_SWITCH, (float) fileItem->GetPVRChannelInfoTag()->ChannelNumber()));
              }
            }
          }
          else
          {
            int autoCloseTime = CSettings::Get().GetBool("pvrplayback.confirmchannelswitch") ? 0 : g_advancedSettings.m_iPVRNumericChannelSwitchTimeout;
            std::string strChannel = StringUtils::Format("%i", action.GetID() - REMOTE_0);
            if (CGUIDialogNumeric::ShowAndGetNumber(strChannel, g_localizeStrings.Get(19000), autoCloseTime) || autoCloseTime)
            {
              int iChannelNumber = atoi(strChannel.c_str());
              if (iChannelNumber > 0 && iChannelNumber != playingChannel->ChannelNumber())
              {
                CPVRChannelGroupPtr selectedGroup = g_PVRManager.GetPlayingGroup(playingChannel->IsRadio());
                CFileItemPtr channel = selectedGroup->GetByChannelNumber(iChannelNumber);
                if (!channel || !channel->HasPVRChannelInfoTag())
                  return false;

                g_application.OnAction(CAction(ACTION_CHANNEL_SWITCH, (float)iChannelNumber));
              }
            }
          }
        }
        else
        {
          // filesystem provider like slingbox, cmyth, etc
          int iChannelNumber = -1;
          std::string strChannel = StringUtils::Format("%i", action.GetID() - REMOTE_0);
          if (CGUIDialogNumeric::ShowAndGetNumber(strChannel, g_localizeStrings.Get(19000)))
            iChannelNumber = atoi(strChannel.c_str());
            
          if (iChannelNumber > 0)
            g_application.OnAction(CAction(ACTION_CHANNEL_SWITCH, (float)iChannelNumber));
        }
      }
      else
      {
        ChangetheTimeCode(action.GetID());
      }
      return true;
    }
    break;

  case ACTION_ASPECT_RATIO:
    { // toggle the aspect ratio mode (only if the info is onscreen)
      if (m_bShowViewModeInfo)
      {
#ifdef HAS_VIDEO_PLAYBACK
        g_renderManager.SetViewMode(++CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode);
#endif
      }
      m_bShowViewModeInfo = true;
      m_dwShowViewModeTimeout = XbmcThreads::SystemClockMillis();
    }
    return true;
    break;
  case ACTION_SMALL_STEP_BACK:
    if (m_timeCodePosition > 0)
      SeekToTimeCodeStamp(SEEK_RELATIVE, SEEK_BACKWARD);
    else
    {
      int orgpos = (int)g_application.GetTime();
      int jumpsize = g_advancedSettings.m_videoSmallStepBackSeconds; // secs
      int setpos = (orgpos > jumpsize) ? orgpos - jumpsize : 0;
      g_application.SeekTime((double)setpos);
    }
    return true;
    break;
  case ACTION_SHOW_PLAYLIST:
    {
      CFileItem item(g_application.CurrentFileItem());
      if (item.HasPVRChannelInfoTag())
        g_windowManager.ActivateWindow(WINDOW_DIALOG_PVR_OSD_CHANNELS);
      else if (item.HasVideoInfoTag())
        g_windowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
      else if (item.HasMusicInfoTag())
        g_windowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    }
    return true;
    break;
  case ACTION_PREVIOUS_CHANNELGROUP:
    {
      if (g_application.CurrentFileItem().HasPVRChannelInfoTag())
        ChangetheTVGroup(false);
      return true;
    }
  case ACTION_NEXT_CHANNELGROUP:
    {
      if (g_application.CurrentFileItem().HasPVRChannelInfoTag())
        ChangetheTVGroup(true);
      return true;
    }
  default:
      break;
  }

  return CGUIWindow::OnAction(action);
}

void CGUIWindowFullScreen::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  // override the clear colour - we must never clear fullscreen
  m_clearBackground = 0;

  CGUIProgressControl* pProgress = dynamic_cast<CGUIProgressControl*>(GetControl(CONTROL_PROGRESS));
  if(pProgress)
  {
    if( pProgress->GetInfo() == 0 || !pProgress->HasVisibleCondition())
    {
      pProgress->SetInfo(PLAYER_PROGRESS);
      pProgress->SetVisibleCondition("player.displayafterseek");
      pProgress->SetVisible(true);
    }
  }

  CGUILabelControl* pLabel = dynamic_cast<CGUILabelControl*>(GetControl(LABEL_BUFFERING));
  if(pLabel && !pLabel->HasVisibleCondition())
  {
    pLabel->SetVisibleCondition("player.caching");
    pLabel->SetVisible(true);
  }

  pLabel = dynamic_cast<CGUILabelControl*>(GetControl(LABEL_CURRENT_TIME));
  if(pLabel && !pLabel->HasVisibleCondition())
  {
    pLabel->SetVisibleCondition("player.displayafterseek");
    pLabel->SetVisible(true);
    pLabel->SetLabel("$INFO(VIDEOPLAYER.TIME) / $INFO(VIDEOPLAYER.DURATION)");
  }

  m_showCodec.Parse("player.showcodec", GetID());

  FillInTVGroups();
}

bool CGUIWindowFullScreen::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_INIT:
    {
      // check whether we've come back here from a window during which time we've actually
      // stopped playing videos
      if (message.GetParam1() == WINDOW_INVALID && !g_application.m_pPlayer->IsPlayingVideo())
      { // why are we here if nothing is playing???
        g_windowManager.PreviousWindow();
        return true;
      }
      g_infoManager.SetShowInfo(false);
      g_infoManager.SetShowCodec(false);
      m_bShowCurrentTime = false;
      m_bGroupSelectShow = false;
      g_infoManager.SetDisplayAfterSeek(0); // Make sure display after seek is off.

      // switch resolution
      g_graphicsContext.SetFullScreenVideo(true);

#ifdef HAS_VIDEO_PLAYBACK
      // make sure renderer is uptospeed
      g_renderManager.Update();
#endif
      // now call the base class to load our windows
      CGUIWindow::OnMessage(message);

      m_bShowViewModeInfo = false;

      return true;
    }
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIDialog *pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_OSD_TELETEXT);
      if (pDialog) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_SLIDER);
      if (pDialog) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_OSD);
      if (pDialog) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_FULLSCREEN_INFO);
      if (pDialog) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_OSD_CHANNELS);
      if (pDialog) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_OSD_GUIDE);
      if (pDialog) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_OSD_DIRECTOR);
      if (pDialog) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_OSD_CUTTER);
      if (pDialog) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_SUBTITLES);
      if (pDialog) pDialog->Close(true);

      CGUIWindow::OnMessage(message);

      CSettings::Get().Save();

      CSingleLock lock (g_graphicsContext);
      g_graphicsContext.SetFullScreenVideo(false);
      lock.Leave();

#ifdef HAS_VIDEO_PLAYBACK
      // make sure renderer is uptospeed
      g_renderManager.Update();
#endif
      return true;
    }
  case GUI_MSG_CLICKED:
    {
      unsigned int iControl = message.GetSenderId();
      if (iControl == CONTROL_GROUP_CHOOSER && g_PVRManager.IsStarted())
      {
        // Get the currently selected label of the Select button
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
        OnMessage(msg);
        std::string strLabel = msg.GetLabel();

        CPVRChannelPtr playingChannel;
        if (g_PVRManager.GetCurrentChannel(playingChannel))
        {
          CPVRChannelGroupPtr selectedGroup = g_PVRChannelGroups->Get(playingChannel->IsRadio())->GetByName(strLabel);
          if (selectedGroup)
          {
            g_PVRManager.SetPlayingGroup(selectedGroup);
            CLog::Log(LOGDEBUG, "%s - switched to group '%s'", __FUNCTION__, selectedGroup->GroupName().c_str());

            if (!selectedGroup->IsGroupMember(*playingChannel))
            {
              CLog::Log(LOGDEBUG, "%s - channel '%s' is not a member of '%s', switching to channel 1 of the new group",
                  __FUNCTION__, playingChannel->ChannelName().c_str(), selectedGroup->GroupName().c_str());
              CFileItemPtr switchChannel = selectedGroup->GetByChannelNumber(1);

              if (switchChannel && switchChannel->HasPVRChannelInfoTag())
                g_application.OnAction(CAction(ACTION_CHANNEL_SWITCH, (float) switchChannel->GetPVRChannelInfoTag()->ChannelNumber()));
              else
              {
                CLog::Log(LOGERROR, "%s - cannot find channel '1' in group %s", __FUNCTION__, selectedGroup->GroupName().c_str());
                CApplicationMessenger::Get().MediaStop(false);
              }
            }
          }
          else
          {
            CLog::Log(LOGERROR, "%s - could not switch to group '%s'", __FUNCTION__, selectedGroup->GroupName().c_str());
            CApplicationMessenger::Get().MediaStop(false);
          }
        }
        else
        {
          CLog::Log(LOGERROR, "%s - cannot find the current channel", __FUNCTION__);
          CApplicationMessenger::Get().MediaStop(false);
        }

        // hide the control and reset focus
        m_bGroupSelectShow = false;
        SET_CONTROL_HIDDEN(CONTROL_GROUP_CHOOSER);
//      SET_CONTROL_FOCUS(0, 0);

        return true;
      }
      break;
    }
  case GUI_MSG_SETFOCUS:
  case GUI_MSG_LOSTFOCUS:
    if (message.GetSenderId() != WINDOW_FULLSCREEN_VIDEO) return true;
    break;
  }

  return CGUIWindow::OnMessage(message);
}

EVENT_RESULT CGUIWindowFullScreen::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_RIGHT_CLICK)
  { // no control found to absorb this click - go back to GUI
    OnAction(CAction(ACTION_SHOW_GUI));
    return EVENT_RESULT_HANDLED;
  }
  if (event.m_id == ACTION_MOUSE_WHEEL_UP)
  {
    return g_application.OnAction(CAction(ACTION_ANALOG_SEEK_FORWARD, 0.5f)) ? EVENT_RESULT_HANDLED : EVENT_RESULT_UNHANDLED;
  }
  if (event.m_id == ACTION_MOUSE_WHEEL_DOWN)
  {
    return g_application.OnAction(CAction(ACTION_ANALOG_SEEK_BACK, 0.5f)) ? EVENT_RESULT_HANDLED : EVENT_RESULT_UNHANDLED;
  }
  if (event.m_id >= ACTION_GESTURE_NOTIFY && event.m_id <= ACTION_GESTURE_END) // gestures
    return EVENT_RESULT_UNHANDLED;
  return EVENT_RESULT_UNHANDLED;
}

void CGUIWindowFullScreen::FrameMove()
{
  if (g_application.m_pPlayer->GetPlaySpeed() != 1)
    g_infoManager.SetDisplayAfterSeek();
  if (m_bShowCurrentTime)
    g_infoManager.SetDisplayAfterSeek();

  if (!g_application.m_pPlayer->HasPlayer()) return;

  if( g_application.m_pPlayer->IsCaching() )
  {
    g_infoManager.SetDisplayAfterSeek(0); //Make sure these stuff aren't visible now
  }

  //------------------------
  m_showCodec.Update();
  if (m_showCodec)
  {
    // show audio codec info
    std::string strAudio, strVideo, strGeneral;
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
      std::string strGeneralFPS;
#if defined(TARGET_DARWIN)
      // We show CPU usage for the entire process, as it's arguably more useful.
      double dCPU = m_resourceCounter.GetCPUUsage();
      std::string strCores;
      strCores = StringUtils::Format("cpu:%.0f%%", dCPU);
#else
      std::string strCores = g_cpuInfo.GetCoresUsageString();
#endif
      int    missedvblanks;
      double refreshrate;
      double clockspeed;
      std::string strClock;

      if (g_VideoReferenceClock.GetClockInfo(missedvblanks, clockspeed, refreshrate))
        strClock = StringUtils::Format("S( refresh:%.3f missed:%i speed:%+.3f%% %s )"
                                       , refreshrate
                                       , missedvblanks
                                       , clockspeed - 100.0
                                       , g_renderManager.GetVSyncState().c_str());

      strGeneralFPS = StringUtils::Format("%s\nW( fps:%02.2f %s )\n%s"
                                          , strGeneral.c_str()
                                          , g_infoManager.GetFPS()
                                          , strCores.c_str(), strClock.c_str() );

      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW3);
      msg.SetLabel(strGeneralFPS);
      OnMessage(msg);
    }
  }
  //----------------------
  // ViewMode Information
  //----------------------
  if (m_bShowViewModeInfo && XbmcThreads::SystemClockMillis() - m_dwShowViewModeTimeout > 2500)
  {
    m_bShowViewModeInfo = false;
  }
  if (m_bShowViewModeInfo)
  {
    RESOLUTION_INFO res = g_graphicsContext.GetResInfo();

    {
      // get the "View Mode" string
      std::string strTitle = g_localizeStrings.Get(629);
      std::string strMode = g_localizeStrings.Get(630 + CMediaSettings::Get().GetCurrentVideoSettings().m_ViewMode);
      std::string strInfo = StringUtils::Format("%s : %s", strTitle.c_str(), strMode.c_str());
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW1);
      msg.SetLabel(strInfo);
      OnMessage(msg);
    }
    // show sizing information
    SPlayerVideoStreamInfo info;
    g_application.m_pPlayer->GetVideoStreamInfo(info);
    {
      // Splitres scaling factor
      float xscale = (float)res.iScreenWidth  / (float)res.iWidth;
      float yscale = (float)res.iScreenHeight / (float)res.iHeight;

      std::string strSizing = StringUtils::Format(g_localizeStrings.Get(245).c_str(),
                                                 (int)info.SrcRect.Width(),
                                                 (int)info.SrcRect.Height(),
                                                 (int)(info.DestRect.Width() * xscale),
                                                 (int)(info.DestRect.Height() * yscale),
                                                 CDisplaySettings::Get().GetZoomAmount(),
                                                 info.videoAspectRatio*CDisplaySettings::Get().GetPixelRatio(),
                                                 CDisplaySettings::Get().GetPixelRatio(),
                                                 CDisplaySettings::Get().GetVerticalShift());
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW2);
      msg.SetLabel(strSizing);
      OnMessage(msg);
    }
    // show resolution information
    {
      std::string strStatus;
      if (g_Windowing.IsFullScreen())
        strStatus = StringUtils::Format("%s %ix%i@%.2fHz - %s",
                                        g_localizeStrings.Get(13287).c_str(),
                                        res.iScreenWidth,
                                        res.iScreenHeight,
                                        res.fRefreshRate,
                                        g_localizeStrings.Get(244).c_str());
      else
        strStatus = StringUtils::Format("%s %ix%i - %s",
                                        g_localizeStrings.Get(13287).c_str(),
                                        res.iScreenWidth,
                                        res.iScreenHeight,
                                        g_localizeStrings.Get(242).c_str());

      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW3);
      msg.SetLabel(strStatus);
      OnMessage(msg);
    }
  }

  if (m_timeCodeShow && m_timeCodePosition != 0)
  {
    if ( (XbmcThreads::SystemClockMillis() - m_timeCodeTimeout) >= 2500)
    {
      m_timeCodeShow = false;
      m_timeCodePosition = 0;
    }
    std::string strDispTime = "00:00:00";

    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW1);

    for (int pos = 7, i = m_timeCodePosition; pos >= 0 && i > 0; pos--)
    {
      if (strDispTime[pos] != ':')
      {
        i -= 1;
        strDispTime[pos] = (char)m_timeCodeStamp[i] + '0';
      }
    }

    strDispTime += "/" + g_infoManager.GetDuration(TIME_FORMAT_HH_MM_SS) + " [" + g_infoManager.GetCurrentPlayTime(TIME_FORMAT_HH_MM_SS) + "]"; // duration [ time ]
    msg.SetLabel(strDispTime);
    OnMessage(msg);
  }

  if (m_showCodec || m_bShowViewModeInfo)
  {
    SET_CONTROL_VISIBLE(LABEL_ROW1);
    SET_CONTROL_VISIBLE(LABEL_ROW2);
    SET_CONTROL_VISIBLE(LABEL_ROW3);
    SET_CONTROL_VISIBLE(BLUE_BAR);
    SET_CONTROL_HIDDEN(CONTROL_GROUP_CHOOSER);
  }
  else if (m_timeCodeShow)
  {
    SET_CONTROL_VISIBLE(LABEL_ROW1);
    SET_CONTROL_HIDDEN(LABEL_ROW2);
    SET_CONTROL_HIDDEN(LABEL_ROW3);
    SET_CONTROL_VISIBLE(BLUE_BAR);
    SET_CONTROL_HIDDEN(CONTROL_GROUP_CHOOSER);
  }
  else if (m_bGroupSelectShow)
  {
    SET_CONTROL_HIDDEN(LABEL_ROW1);
    SET_CONTROL_HIDDEN(LABEL_ROW2);
    SET_CONTROL_HIDDEN(LABEL_ROW3);
    SET_CONTROL_HIDDEN(BLUE_BAR);
    SET_CONTROL_VISIBLE(CONTROL_GROUP_CHOOSER);
  }
  else
  {
    SET_CONTROL_HIDDEN(LABEL_ROW1);
    SET_CONTROL_HIDDEN(LABEL_ROW2);
    SET_CONTROL_HIDDEN(LABEL_ROW3);
    SET_CONTROL_HIDDEN(BLUE_BAR);
    SET_CONTROL_HIDDEN(CONTROL_GROUP_CHOOSER);
  }
}

void CGUIWindowFullScreen::Process(unsigned int currentTime, CDirtyRegionList &dirtyregion)
{
  // TODO: This isn't quite optimal - ideally we'd only be dirtying up the actual video render rect
  //       which is probably the job of the renderer as it can more easily track resizing etc.
  MarkDirtyRegion();
  CGUIWindow::Process(currentTime, dirtyregion);
  m_renderRegion.SetRect(0, 0, (float)g_graphicsContext.GetWidth(), (float)g_graphicsContext.GetHeight());
}

void CGUIWindowFullScreen::Render()
{
  CGUIWindow::Render();
}

void CGUIWindowFullScreen::ChangetheTimeCode(int remote)
{
  if (remote >= REMOTE_0 && remote <= REMOTE_9)
  {
    m_timeCodeShow = true;
    m_timeCodeTimeout = XbmcThreads::SystemClockMillis();

    if (m_timeCodePosition < 6)
      m_timeCodeStamp[m_timeCodePosition++] = remote - REMOTE_0;
    else
    {
      // rotate around
      for (int i = 0; i < 5; i++)
        m_timeCodeStamp[i] = m_timeCodeStamp[i+1];
      m_timeCodeStamp[5] = remote - REMOTE_0;
    }
  }
}

void CGUIWindowFullScreen::SeekToTimeCodeStamp(SEEK_TYPE type, SEEK_DIRECTION direction)
{
  double total = GetTimeCodeStamp();
  if (type == SEEK_RELATIVE)
    total = g_application.GetTime() + (((direction == SEEK_FORWARD) ? 1 : -1) * total);

  if (total < g_application.GetTotalTime())
    g_application.SeekTime(total);

  m_timeCodePosition = 0;
  m_timeCodeShow = false;
}

double CGUIWindowFullScreen::GetTimeCodeStamp()
{
  // Convert the timestamp into an integer
  int tot = 0;
  for (int i = 0; i < m_timeCodePosition; i++)
    tot = tot * 10 + m_timeCodeStamp[i];

  // Interpret result as HHMMSS
  int s = tot % 100; tot /= 100;
  int m = tot % 100; tot /= 100;
  int h = tot % 100;
  return h * 3600 + m * 60 + s;
}

void CGUIWindowFullScreen::SeekChapter(int iChapter)
{
  g_application.m_pPlayer->SeekChapter(iChapter);

  // Make sure gui items are visible.
  g_infoManager.SetDisplayAfterSeek();
}

void CGUIWindowFullScreen::FillInTVGroups()
{
  if (!g_PVRManager.IsStarted())
    return;

  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_GROUP_CHOOSER);
  g_windowManager.SendMessage(msgReset);

  const CPVRChannelGroups *groups = g_PVRChannelGroups->Get(g_PVRManager.IsPlayingRadio());
  if (groups)
    groups->FillGroupsGUI(GetID(), CONTROL_GROUP_CHOOSER);
}

void CGUIWindowFullScreen::ChangetheTVGroup(bool next)
{
  if (!g_PVRManager.IsStarted())
    return;

  CGUIControl* pButton = GetControl(CONTROL_GROUP_CHOOSER);
  if (!pButton)
    return;

  if (!m_bGroupSelectShow)
  {
    SET_CONTROL_VISIBLE(CONTROL_GROUP_CHOOSER);
    SET_CONTROL_FOCUS(CONTROL_GROUP_CHOOSER, 0);

    // fire off an event that we've pressed this button...
    OnAction(CAction(ACTION_SELECT_ITEM));

    m_bGroupSelectShow = true;
  }
  else
  {
    if (next)
      pButton->OnRight();
    else
      pButton->OnLeft();
  }
}

void CGUIWindowFullScreen::ToggleOSD()
{
  CGUIDialogVideoOSD *pOSD = (CGUIDialogVideoOSD *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_OSD);
  if (pOSD)
  {
    if (pOSD->IsDialogRunning())
      pOSD->Close();
    else
      pOSD->DoModal();
  }
}

void CGUIWindowFullScreen::TriggerOSD()
{
  CGUIDialogVideoOSD *pOSD = (CGUIDialogVideoOSD *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_OSD);
  if (pOSD && !pOSD->IsDialogRunning())
  {
    pOSD->SetAutoClose(3000);
    pOSD->DoModal();
  }
}

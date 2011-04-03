/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Application.h"
#include "settings/GUISettings.h"
#include "Util.h"
#include "guilib/GUIWindowManager.h"
#include "windows/GUIWindowPVR.h"
#include "GUIInfoManager.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "guilib/LocalizeStrings.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/Settings.h"
#include "filesystem/StackDirectory.h"

/* GUI Messages includes */
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"

#include "PVRManager.h"
#include "addons/PVRClients.h"
#include "channels/PVRChannelGroupsContainer.h"
#include "epg/PVREpgContainer.h"
#include "recordings/PVRRecordings.h"
#include "timers/PVRTimers.h"

using namespace std;
using namespace XFILE;
using namespace MUSIC_INFO;
using namespace ADDON;

CPVRManager *CPVRManager::m_instance = NULL;

CPVRManager::CPVRManager() :
    Observer()
{
  m_bFirstStart                 = true;
  m_bLoaded                     = false;

  m_addons                      = new CPVRClients();
  m_channelGroups               = new CPVRChannelGroupsContainer();
  m_epg                         = new CPVREpgContainer();
  m_recordings                  = new CPVRRecordings();
  m_timers                      = new CPVRTimers();

  ResetProperties();
}

CPVRManager::~CPVRManager()
{
  Stop();
  delete m_epg;
  delete m_recordings;
  delete m_timers;
  delete m_channelGroups;
  delete m_addons;
  CLog::Log(LOGDEBUG,"PVRManager - destroyed");
}

void CPVRManager::Notify(const Observable &obs, const CStdString& msg)
{
}

CPVRManager *CPVRManager::Get(void)
{
  if (!m_instance)
    m_instance = new CPVRManager();

  return m_instance;
}

void CPVRManager::Cleanup(void)
{
  /* stop and clean up the EPG thread */
  m_epg->RemoveObserver(this);
  m_epg->Stop();

  /* unload the rest */
  m_recordings->Unload();
  m_timers->Unload();
  m_channelGroups->Unload();
  m_addons->Unload();
  m_bLoaded = false;
}

bool CPVRManager::ChannelUp(unsigned int *iNewChannelNumber, bool bPreview /* = false*/)
{
  return ChannelUpDown(iNewChannelNumber, bPreview, true);
}

bool CPVRManager::ChannelDown(unsigned int *iNewChannelNumber, bool bPreview /* = false*/)
{
  return ChannelUpDown(iNewChannelNumber, bPreview, false);
}

bool CPVRManager::ChannelSwitch(unsigned int iChannel)
{
  const CPVRChannel *channel = NULL;

  if (m_addons->IsPlayingRadio())
  {
    if (m_currentRadioGroup != NULL)
      channel = m_currentRadioGroup->GetByChannelNumber(iChannel);

    if (channel == NULL)
      channel = GetChannelGroups()->GetGroupAllRadio()->GetByChannelNumber(iChannel);
  }
  else
  {
    /* always use the TV group if we're not playing a radio channel */
    if (m_currentTVGroup != NULL)
      channel = m_currentTVGroup->GetByChannelNumber(iChannel);

    if (channel == NULL)
      channel = GetChannelGroups()->GetGroupAllTV()->GetByChannelNumber(iChannel);
  }

  if (channel == NULL)
  {
    CLog::Log(LOGERROR, "PVRManager - %s - cannot find channel %d", __FUNCTION__, iChannel);
    return false;
  }

  return PerformChannelSwitch(*channel, false);
}

bool CPVRManager::ChannelUpDown(unsigned int *iNewChannelNumber, bool bPreview, bool bUp)
{
  bool bReturn = false;

  CPVRChannel currentChannel;
  if (m_addons->GetPlayingChannel(&currentChannel))
  {
    const CPVRChannelGroup *group = GetPlayingGroup(currentChannel.IsRadio());
    if (group)
    {
      const CPVRChannel *newChannel = bUp ? group->GetByChannelUp(currentChannel) : group->GetByChannelDown(currentChannel);
      if (PerformChannelSwitch(*newChannel, bPreview))
      {
        *iNewChannelNumber = newChannel->ChannelNumber();
        bReturn = true;
      }
    }
  }

  return bReturn;
}

bool CPVRManager::ContinueLastChannel()
{
  bool bReturn = false;
  m_bFirstStart = false;

  const CPVRChannel *channel = GetChannelGroups()->GetGroupAllTV()->GetByIndex(0);
  for (int i = 0; i < GetChannelGroups()->GetGroupAllTV()->GetNumChannels(); i++)
  {
    const CPVRChannel *nextChannel = GetChannelGroups()->GetGroupAllTV()->GetByIndex(i);
    if (nextChannel->ClientID() < 0 || !m_addons->IsValidClient(nextChannel->ClientID()))
      continue;
    channel = channel->LastWatched() > nextChannel->LastWatched() ? channel : nextChannel;
  }
  for (int i = 0; i < GetChannelGroups()->GetGroupAllRadio()->GetNumChannels(); i++)
  {
    const CPVRChannel *nextChannel = GetChannelGroups()->GetGroupAllRadio()->GetByIndex(i);
    if (nextChannel->ClientID() < 0 || !m_addons->IsValidClient(nextChannel->ClientID()))
      continue;
    channel = channel->LastWatched() > nextChannel->LastWatched() ? channel : nextChannel;
  }

  if (channel)
  {
    CLog::Log(LOGNOTICE, "PVRManager - %s - continue playback on channel '%s'",
        __FUNCTION__, channel->ChannelName().c_str());
    bReturn = StartPlayback(channel, (g_guiSettings.GetInt("pvrplayback.startlast") == START_LAST_CHANNEL_MIN));
  }

  return bReturn;
}

CPVRChannelGroupsContainer *CPVRManager::GetChannelGroups(void)
{
  return Get()->m_channelGroups;
}

CPVREpgContainer *CPVRManager::GetEpg(void)
{
  return Get()->m_epg;
}

CPVRRecordings *CPVRManager::GetRecordings(void)
{
  return Get()->m_recordings;
}

CPVRTimers *CPVRManager::GetTimers(void)
{
  return Get()->m_timers;
}

CPVRClients *CPVRManager::GetClients(void)
{
  return Get()->m_addons;
}

void CPVRManager::Destroy(void)
{
  if (m_instance)
  {
    delete m_instance;
    m_instance = NULL;
  }
}

void CPVRManager::Start()
{
  /* first stop and remove any clients */
  Stop();

  /* don't start if Settings->Video->TV->Enable isn't checked */
  if (!g_guiSettings.GetBool("pvrmanager.enabled"))
    return;

  CLog::Log(LOGNOTICE, "PVRManager - starting up");

  ResetProperties();

  /* create the supervisor thread to do all background activities */
  Create();
  SetName("XBMC PVRManager");
  SetPriority(-1);
  CLog::Log(LOGNOTICE, "PVRManager - starting up");
}

void CPVRManager::Stop()
{
  CLog::Log(LOGNOTICE, "PVRManager - stopping");

  if (IsPlaying())
  {
    CLog::Log(LOGNOTICE,"PVRManager - %s - stopping PVR playback", __FUNCTION__);
    g_application.StopPlaying();
  }

  StopThreads();
  Cleanup();
}

void CPVRManager::StartThreads()
{
  m_epg->Create();
  Create();
}

void CPVRManager::StopThreads()
{
  m_epg->StopThread();
  StopThread();
}

void CPVRManager::UpdateWindow(PVRWindow window)
{
  CGUIWindowPVR *pWindow = (CGUIWindowPVR *) g_windowManager.GetWindow(WINDOW_PVR);
  if (pWindow)
    pWindow->UpdateWindow(window);
}

bool CPVRManager::IsRunningChannelScan(void)
{
  return m_addons->IsRunningChannelScan();
}

void CPVRManager::ResetProperties(void)
{
  m_bTriggerChannelsUpdate      = false;
  m_bTriggerRecordingsUpdate    = false;
  m_bTriggerTimersUpdate        = false;
  m_bTriggerChannelGroupsUpdate = false;
  m_currentFile                 = NULL;
  m_NextRecording               = NULL;
  m_strActiveTimerTitle         = "";
  m_strActiveTimerChannelName   = "";
  m_strActiveTimerTime          = "";
  m_strNextRecordingTitle       = "";
  m_strNextRecordingChannelName = "";
  m_strNextRecordingTime        = "";
  m_strNextTimerInfo            = "";
  m_strPlayingDuration          = "";
  m_strPlayingTime              = "";
  m_NowRecording.clear();

  m_bHasRecordings              = false;
  m_bIsRecording                = false;
  m_bHasTimers                  = false;
  m_currentRadioGroup           = NULL;
  m_currentTVGroup              = NULL;
  m_PreviousChannel[0]          = -1;
  m_PreviousChannel[1]          = -1;
  m_PreviousChannelIndex        = 0;
  m_recordingToggleStart        = NULL;
  m_recordingToggleCurrent      = 0;
  m_LastChannel                 = 0;
}

void CPVRManager::UpdateTimers(void)
{
  CSingleLock lock(m_critSectionTriggers);
  if (!m_bTriggerTimersUpdate)
    return;
  lock.Leave();

  CLog::Log(LOGDEBUG, "PVRManager - %s - updating timers", __FUNCTION__);

  m_timers->Update();
  UpdateTimersCache();
  UpdateWindow(PVR_WINDOW_TIMERS);

  lock.Enter();
  m_bTriggerTimersUpdate = false;
  lock.Leave();
}

void CPVRManager::UpdateRecordings(void)
{
  CSingleLock lock(m_critSectionTriggers);
  if (!m_bTriggerRecordingsUpdate)
    return;
  lock.Leave();

  CLog::Log(LOGDEBUG, "PVRManager - %s - updating recordings list", __FUNCTION__);

  m_recordings->Update();
  UpdateRecordingsCache();
  UpdateWindow(PVR_WINDOW_RECORDINGS);

  lock.Enter();
  m_bTriggerRecordingsUpdate = false;
  lock.Leave();
}

void CPVRManager::UpdateChannels(void)
{
  CSingleLock lock(m_critSectionTriggers);
  if (!m_bTriggerChannelsUpdate && !m_bTriggerChannelGroupsUpdate)
    return;
  lock.Leave();

  CLog::Log(LOGDEBUG, "PVRManager - %s - updating %s list",
      __FUNCTION__, m_bTriggerChannelGroupsUpdate ? "channels" : "channel groups");

  m_channelGroups->Update(!m_bTriggerChannelGroupsUpdate);
  UpdateTimersCache();
  UpdateWindow(PVR_WINDOW_CHANNELS_TV);
  UpdateWindow(PVR_WINDOW_CHANNELS_RADIO);

  lock.Enter();
  m_bTriggerChannelsUpdate      = false;
  m_bTriggerChannelGroupsUpdate = false;
  lock.Leave();
}

bool CPVRManager::DisableIfNoClients(void)
{
  bool bReturn = false;

  if (!m_addons->HasClients())
  {
    g_guiSettings.SetBool("pvrmanager.enabled", false);
    CLog::Log(LOGNOTICE,"PVRManager - no clients enabled. pvrmanager disabled.");
    bReturn = true;
  }

  return bReturn;
}

void CPVRManager::Process()
{
  while (!m_addons->HasActiveClients())
  {
    m_addons->TryLoadClients(1);

    if (m_addons->HasActiveClients())
    {
      CLog::Log(LOGDEBUG, "PVRManager - %s - active clients found. continue to start", __FUNCTION__);

      /* load all channels and groups */
      m_channelGroups->Load();
      m_bTriggerChannelsUpdate = false;
      m_bTriggerChannelGroupsUpdate = false;

      /* get timers from the backends */
      m_timers->Load();
      m_bTriggerTimersUpdate = false;

      /* get recordings from the backend */
      m_recordings->Load();
      m_bTriggerRecordingsUpdate = false;
    }

    /* check if there are (still) any enabled addons */
    if (DisableIfNoClients())
    {
      CLog::Log(LOGDEBUG, "PVRManager - %s - no clients could be found. aborting startup", __FUNCTION__);
      return;
    }
  }

  m_bLoaded = true;

  /* start the EPG thread */
  m_epg->AddObserver(this);
  m_epg->Start();

  /* continue last watched channel after first startup */
  if (!m_bStop && m_bFirstStart && g_guiSettings.GetInt("pvrplayback.startlast") != START_LAST_CHANNEL_OFF)
    ContinueLastChannel();

  /* keep trying to load remaining clients */
  if (!m_addons->AllClientsLoaded())
    m_addons->TryLoadClients(0);

  CLog::Log(LOGDEBUG, "PVRManager - %s - entering main loop", __FUNCTION__);

  /* main loop */
  while (!m_bStop)
  {
    /* check if the (still) are any enabled addons */
    if (DisableIfNoClients())
      return;

    UpdateChannels();
    UpdateRecordings();
    UpdateTimers();
    m_addons->UpdateSignalQuality();

    Sleep(1000);
  }
}

void CPVRManager::ResetDatabase(bool bShowProgress /* = true */)
{
  CLog::Log(LOGNOTICE,"PVRManager - %s - clearing the PVR database", __FUNCTION__);

  CGUIDialogProgress* pDlgProgress = NULL;

  if (bShowProgress)
  {
    pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    pDlgProgress->SetLine(0, "");
    pDlgProgress->SetLine(1, g_localizeStrings.Get(19186));
    pDlgProgress->SetLine(2, "");
    pDlgProgress->StartModal();
    pDlgProgress->Progress();
  }

  if (m_addons->IsPlaying())
  {
    CLog::Log(LOGNOTICE,"PVRManager - %s - stopping playback", __FUNCTION__);
    g_application.StopPlaying();
  }

  if (bShowProgress)
    pDlgProgress->SetPercentage(10);

  /* stop the thread */
  Stop();
  if (bShowProgress)
    pDlgProgress->SetPercentage(20);

  if (m_database.Open())
  {
    /* clean the EPG database */
    m_epg->Clear(true);
    if (bShowProgress)
      pDlgProgress->SetPercentage(30);

    /* delete all TV channel groups */
    m_database.DeleteChannelGroups(false);
    if (bShowProgress)
      pDlgProgress->SetPercentage(50);

    /* delete all radio channel groups */
    m_database.DeleteChannelGroups(true);
    if (bShowProgress)
      pDlgProgress->SetPercentage(60);

    /* delete all channels */
    m_database.DeleteChannels();
    if (bShowProgress)
      pDlgProgress->SetPercentage(70);

    /* delete all channel settings */
    m_database.DeleteChannelSettings();
    if (bShowProgress)
      pDlgProgress->SetPercentage(80);

    /* delete all client information */
    m_database.DeleteClients();
    if (bShowProgress)
      pDlgProgress->SetPercentage(90);

    m_database.Close();
  }

  CLog::Log(LOGNOTICE,"PVRManager - %s - PVR database cleared. restarting the PVRManager", __FUNCTION__);

  Start();

  if (bShowProgress)
  {
    pDlgProgress->SetPercentage(100);
    pDlgProgress->Close();
  }
}

void CPVRManager::ResetEPG(void)
{
  StopThreads();
  m_epg->Reset();
  StartThreads();
}

bool CPVRManager::IsPlaying(void)
{
  return m_bLoaded && m_addons->IsPlaying();
}

bool CPVRManager::GetCurrentChannel(CPVRChannel *channel)
{
  return m_addons->GetPlayingChannel(channel);
}

int CPVRManager::GetCurrentEpg(CFileItemList *results)
{
  int iReturn = -1;

  CPVRChannel channel;
  if (m_addons->GetPlayingChannel(&channel))
    iReturn = channel.GetEPG(results);
  else
    CLog::Log(LOGDEBUG,"PVRManager - %s - no current channel set", __FUNCTION__);

  return iReturn;
}

int CPVRManager::GetPreviousChannel(void)
{
  //XXX this must be the craziest way to store the last channel
  int iReturn = -1;
  CPVRChannel channel;
  if (m_addons->GetPlayingChannel(&channel))
  {
    int iLastChannel = channel.ChannelNumber();

    if ((m_PreviousChannel[m_PreviousChannelIndex ^ 1] == iLastChannel || iLastChannel != m_PreviousChannel[0]) &&
        iLastChannel != m_PreviousChannel[1])
      m_PreviousChannelIndex ^= 1;

    iReturn = m_PreviousChannel[m_PreviousChannelIndex ^= 1];
  }

  return iReturn;
}

bool CPVRManager::StartRecordingOnPlayingChannel(bool bOnOff)
{
  bool bReturn = false;

  CPVRChannel channel;
  if (!m_addons->GetPlayingChannel(&channel))
    return bReturn;

  if (m_addons->HasTimerSupport(channel.ClientID()))
  {
    /* timers are supported on this channel */
    if (bOnOff && !channel.IsRecording())
    {
      CPVRTimerInfoTag *newTimer = m_timers->InstantTimer(&channel);
      if (!newTimer)
        CGUIDialogOK::ShowAndGetInput(19033,0,19164,0);
      else
        bReturn = true;
    }
    else if (!bOnOff && channel.IsRecording())
    {
      /* delete active timers */
      bReturn = m_timers->DeleteTimersOnChannel(channel, false, true);
    }
  }

  return bReturn;
}

void CPVRManager::SaveCurrentChannelSettings()
{
  CPVRChannel channel;
  if (!m_addons->GetPlayingChannel(&channel))
    return;

  if (g_settings.m_currentVideoSettings != g_settings.m_defaultVideoSettings &&
      m_database.Open())
  {
    /* only save settings if they differ from the default settings */
    m_database.PersistChannelSettings(channel, g_settings.m_currentVideoSettings);
    m_database.Close();
  }
  else if (!(g_settings.m_currentVideoSettings != g_settings.m_defaultVideoSettings) &&
      m_database.Open())
  {
    /* delete record which might differ from the default settings */
    m_database.DeleteChannelSettings(channel);
    m_database.Close();
  }
}

void CPVRManager::LoadCurrentChannelSettings()
{
  CPVRChannel channel;
  if (!m_addons->GetPlayingChannel(&channel))
    return;

  if (g_application.m_pPlayer)
  {
    /* set the default settings first */
    CVideoSettings loadedChannelSettings = g_settings.m_defaultVideoSettings;

    /* try to load the settings from the database */
    if (m_database.Open())
    {
      m_database.GetChannelSettings(channel, loadedChannelSettings);
      m_database.Close();
    }

    g_settings.m_currentVideoSettings = g_settings.m_defaultVideoSettings;
    g_settings.m_currentVideoSettings.m_Brightness          = loadedChannelSettings.m_Brightness;
    g_settings.m_currentVideoSettings.m_Contrast            = loadedChannelSettings.m_Contrast;
    g_settings.m_currentVideoSettings.m_Gamma               = loadedChannelSettings.m_Gamma;
    g_settings.m_currentVideoSettings.m_Crop                = loadedChannelSettings.m_Crop;
    g_settings.m_currentVideoSettings.m_CropLeft            = loadedChannelSettings.m_CropLeft;
    g_settings.m_currentVideoSettings.m_CropRight           = loadedChannelSettings.m_CropRight;
    g_settings.m_currentVideoSettings.m_CropTop             = loadedChannelSettings.m_CropTop;
    g_settings.m_currentVideoSettings.m_CropBottom          = loadedChannelSettings.m_CropBottom;
    g_settings.m_currentVideoSettings.m_CustomPixelRatio    = loadedChannelSettings.m_CustomPixelRatio;
    g_settings.m_currentVideoSettings.m_CustomZoomAmount    = loadedChannelSettings.m_CustomZoomAmount;
    g_settings.m_currentVideoSettings.m_NoiseReduction      = loadedChannelSettings.m_NoiseReduction;
    g_settings.m_currentVideoSettings.m_Sharpness           = loadedChannelSettings.m_Sharpness;
    g_settings.m_currentVideoSettings.m_InterlaceMethod     = loadedChannelSettings.m_InterlaceMethod;
    g_settings.m_currentVideoSettings.m_OutputToAllSpeakers = loadedChannelSettings.m_OutputToAllSpeakers;
    g_settings.m_currentVideoSettings.m_AudioDelay          = loadedChannelSettings.m_AudioDelay;
    g_settings.m_currentVideoSettings.m_AudioStream         = loadedChannelSettings.m_AudioStream;
    g_settings.m_currentVideoSettings.m_SubtitleOn          = loadedChannelSettings.m_SubtitleOn;
    g_settings.m_currentVideoSettings.m_SubtitleDelay       = loadedChannelSettings.m_SubtitleDelay;

    /* only change the view mode if it's different */
    if (g_settings.m_currentVideoSettings.m_ViewMode != loadedChannelSettings.m_ViewMode)
    {
      g_settings.m_currentVideoSettings.m_ViewMode = loadedChannelSettings.m_ViewMode;

      g_renderManager.SetViewMode(g_settings.m_currentVideoSettings.m_ViewMode);
      g_settings.m_currentVideoSettings.m_CustomZoomAmount = g_settings.m_fZoomAmount;
      g_settings.m_currentVideoSettings.m_CustomPixelRatio = g_settings.m_fPixelRatio;
    }

    /* only change the subtitle strea, if it's different */
    if (g_settings.m_currentVideoSettings.m_SubtitleStream != loadedChannelSettings.m_SubtitleStream)
    {
      g_settings.m_currentVideoSettings.m_SubtitleStream = loadedChannelSettings.m_SubtitleStream;

      g_application.m_pPlayer->SetSubtitle(g_settings.m_currentVideoSettings.m_SubtitleStream);
    }

    /* only change the audio stream if it's different */
    if (g_application.m_pPlayer->GetAudioStream() != g_settings.m_currentVideoSettings.m_AudioStream)
      g_application.m_pPlayer->SetAudioStream(g_settings.m_currentVideoSettings.m_AudioStream);

    g_application.m_pPlayer->SetAVDelay(g_settings.m_currentVideoSettings.m_AudioDelay);
    g_application.m_pPlayer->SetDynamicRangeCompression((long)(g_settings.m_currentVideoSettings.m_VolumeAmplification * 100));
    g_application.m_pPlayer->SetSubtitleVisible(g_settings.m_currentVideoSettings.m_SubtitleOn);
    g_application.m_pPlayer->SetSubTitleDelay(g_settings.m_currentVideoSettings.m_SubtitleDelay);
  }
}

void CPVRManager::SetPlayingGroup(CPVRChannelGroup *group)
{
  if (group && group->IsRadio())
    m_currentRadioGroup = group;
  else if (group && !group->IsRadio())
    m_currentTVGroup = group;
}

const CPVRChannelGroup *CPVRManager::GetPlayingGroup(bool bRadio /* = false */)
{
  if (bRadio && !m_currentRadioGroup)
    m_currentRadioGroup = (CPVRChannelGroup *) GetChannelGroups()->GetGroupAllRadio();
  else if (!bRadio &&!m_currentTVGroup)
    m_currentTVGroup = (CPVRChannelGroup *) GetChannelGroups()->GetGroupAllTV();

  return bRadio ? m_currentRadioGroup : m_currentTVGroup;
}

void CPVRManager::TriggerRecordingsUpdate(void)
{
  CSingleLock lock(m_critSectionTriggers);
  if (!m_bTriggerRecordingsUpdate)
  {
    m_bTriggerRecordingsUpdate = true;
    CLog::Log(LOGDEBUG, "PVRManager - %s - recordings update scheduled", __FUNCTION__);
  }
}

void CPVRManager::TriggerTimersUpdate(void)
{
  CSingleLock lock(m_critSectionTriggers);
  if (!m_bTriggerTimersUpdate)
  {
    m_bTriggerTimersUpdate = true;
    CLog::Log(LOGDEBUG, "PVRManager - %s - timers update scheduled", __FUNCTION__);
  }
}

void CPVRManager::TriggerChannelsUpdate(void)
{
  CSingleLock lock(m_critSectionTriggers);
  if (!m_bTriggerChannelsUpdate)
  {
    m_bTriggerChannelsUpdate = true;
    CLog::Log(LOGDEBUG, "PVRManager - %s - channels update scheduled", __FUNCTION__);
  }
}

void CPVRManager::TriggerChannelGroupsUpdate(void)
{
  CSingleLock lock(m_critSectionTriggers);
  if (!m_bTriggerChannelGroupsUpdate)
  {
    m_bTriggerChannelGroupsUpdate = true;
    CLog::Log(LOGDEBUG, "PVRManager - %s - channel groups update scheduled", __FUNCTION__);
  }
}

bool CPVRManager::OpenLiveStream(const CPVRChannel &tag)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  CLog::Log(LOGDEBUG,"PVRManager - %s - opening live stream on channel '%s'",
      __FUNCTION__, tag.ChannelName().c_str());

  if ((bReturn = m_addons->OpenLiveStream(tag)) != false)
  {
    delete m_currentFile;
    m_currentFile = new CFileItem(tag);

    LoadCurrentChannelSettings();
  }

  return bReturn;
}

bool CPVRManager::OpenRecordedStream(const CPVRRecording &tag)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  CLog::Log(LOGDEBUG,"PVRManager - %s - opening recorded stream '%s'",
      __FUNCTION__, tag.m_strFile.c_str());

  if ((bReturn = m_addons->OpenRecordedStream(tag)) != false)
  {
    delete m_currentFile;
    m_currentFile = new CFileItem(tag);
  }

  return bReturn;
}

void CPVRManager::CloseStream()
{
  CSingleLock lock(m_critSection);

  if (m_addons->IsReadingLiveStream())
  {
    CPVRChannel channel;
    if (m_addons->GetPlayingChannel(&channel))
    {
      /* store current time in iLastWatched */
      time_t tNow;
      CDateTime::GetCurrentDateTime().GetAsTime(tNow);
      channel.SetLastWatched(tNow, true);

      /* Store current settings inside Database */
      SaveCurrentChannelSettings();
    }
  }

  m_addons->CloseStream();
  if (m_currentFile)
  {
    delete m_currentFile;
    m_currentFile = NULL;
  }
}

bool CPVRManager::UpdateItem(CFileItem& item)
{
  /* Don't update if a recording is played */
  if (item.IsPVRRecording())
    return true;

  if (!item.IsPVRChannel())
  {
    CLog::Log(LOGERROR, "CPVRManager - %s - no channel tag provided", __FUNCTION__);
    return false;
  }

  CSingleLock lock(m_critSection);
  g_application.CurrentFileItem() = *m_currentFile;
  g_infoManager.SetCurrentItem(*m_currentFile);

  CPVRChannel* channelTag = item.GetPVRChannelInfoTag();
  const CPVREpgInfoTag* epgTagNow = channelTag->GetEPGNow();

  if (channelTag->IsRadio())
  {
    CMusicInfoTag* musictag = item.GetMusicInfoTag();
    if (musictag)
    {
      musictag->SetTitle(epgTagNow ? epgTagNow->Title() : g_localizeStrings.Get(19055));
      musictag->SetGenre(epgTagNow ? epgTagNow->Genre() : "");
      musictag->SetDuration(epgTagNow ? epgTagNow->GetDuration() : 3600);
      musictag->SetURL(channelTag->Path());
      musictag->SetArtist(channelTag->ChannelName());
      musictag->SetAlbumArtist(channelTag->ChannelName());
      musictag->SetLoaded(true);
      musictag->SetComment("");
      musictag->SetLyrics("");
    }
  }
  else
  {
    CVideoInfoTag *videotag = item.GetVideoInfoTag();
    if (videotag)
    {
      videotag->m_strTitle = epgTagNow ? epgTagNow->Title() : g_localizeStrings.Get(19055);
      videotag->m_strGenre = epgTagNow ? epgTagNow->Genre() : "";
      videotag->m_strPath = channelTag->Path();
      videotag->m_strFileNameAndPath = channelTag->Path();
      videotag->m_strPlot = epgTagNow ? epgTagNow->Plot() : "";
      videotag->m_strPlotOutline = epgTagNow ? epgTagNow->PlotOutline() : "";
      videotag->m_iEpisode = epgTagNow ? epgTagNow->EpisodeNum() : 0;
    }
  }

  CPVRChannel* tagPrev = item.GetPVRChannelInfoTag();
  if (tagPrev && tagPrev->ChannelNumber() != m_LastChannel)
  {
    m_LastChannel         = tagPrev->ChannelNumber();
    m_LastChannelChanged  = CTimeUtils::GetTimeMS();
  }
  if (CTimeUtils::GetTimeMS() - m_LastChannelChanged >= (unsigned int) g_guiSettings.GetInt("pvrplayback.channelentrytimeout") && m_LastChannel != m_PreviousChannel[m_PreviousChannelIndex])
     m_PreviousChannel[m_PreviousChannelIndex ^= 1] = m_LastChannel;

  return false;
}

bool CPVRManager::StartPlayback(const CPVRChannel *channel, bool bPreview /* = false */)
{
  bool bReturn = false;
  g_settings.m_bStartVideoWindowed = bPreview;

  if (g_application.PlayFile(CFileItem(*channel)))
  {
    CLog::Log(LOGNOTICE, "PVRManager - %s - started playback on channel '%s'",
        __FUNCTION__, channel->ChannelName().c_str());
    bReturn = true;
  }
  else
  {
    CLog::Log(LOGERROR, "PVRManager - %s - failed to start playback on channel '%s'",
        __FUNCTION__, channel ? channel->ChannelName().c_str() : "NULL");
  }

  return bReturn;
}

bool CPVRManager::PerformChannelSwitch(const CPVRChannel &channel, bool bPreview)
{
  CSingleLock lock(m_critSection);

  CLog::Log(LOGDEBUG, "PVRManager - %s - switching to channel '%s'",
      __FUNCTION__, channel.ChannelName().c_str());

  SaveCurrentChannelSettings();
  if (m_currentFile)
  {
    delete m_currentFile;
    m_currentFile = NULL;
  }

  if (channel.ClientID() < 0 || !m_addons->SwitchChannel(channel))
  {
    CLog::Log(LOGERROR, "PVRManager - %s - failed to switch to channel '%s'",
        __FUNCTION__, channel.ChannelName().c_str());
    CGUIDialogOK::ShowAndGetInput(19033,0,19136,0);
    return false;
  }

  m_currentFile = new CFileItem(channel);
  LoadCurrentChannelSettings();

  CLog::Log(LOGNOTICE, "PVRManager - %s - switched to channel '%s'",
      __FUNCTION__, channel.ChannelName().c_str());

  return true;
}

const CPVREpgInfoTag *CPVRManager::GetPlayingTag(void)
{
  const CPVREpgInfoTag *tag = NULL;

  CPVRChannel currentChannel;
  if (m_addons->GetPlayingChannel(&currentChannel))
  {
    tag = currentChannel.GetEPGNow();
    if (tag && !tag->IsActive())
    {
      CSingleLock lock(m_critSection);
      UpdateItem(*m_currentFile);
    }
  }

  return tag;
}

int CPVRManager::GetTotalTime()
{
  const CPVREpgInfoTag *tag = GetPlayingTag();

  return tag ? tag->GetDuration() * 1000 : 0;
}

int CPVRManager::GetStartTime()
{
  const CPVREpgInfoTag *tag = GetPlayingTag();

  if (tag)
  {
    /* Calculate here the position we have of the running live TV event.
     * "position in ms" = ("current local time" - "event start local time") * 1000
     */
    CDateTimeSpan time = CDateTime::GetCurrentDateTime() - tag->StartAsLocalTime();
    return time.GetDays()    * 1000 * 60 * 60 * 24
         + time.GetHours()   * 1000 * 60 * 60
         + time.GetMinutes() * 1000 * 60
         + time.GetSeconds() * 1000;
  }
  else
  {
    return 0;
  }
}

const char* CPVRManager::TranslateCharInfo(DWORD dwInfo)
{
  if      (dwInfo == PVR_NOW_RECORDING_TITLE)
  {
    UpdateRecordingToggle();
    static CStdString strReturn = m_strActiveTimerTitle;
    return strReturn.c_str();
  }
  else if (dwInfo == PVR_NOW_RECORDING_CHANNEL)
  {
    static CStdString strReturn = m_strActiveTimerChannelName;
    return strReturn.c_str();
  }
  else if (dwInfo == PVR_NOW_RECORDING_DATETIME)
  {
    static CStdString strReturn = m_strActiveTimerTime;
    return strReturn.c_str();
  }
  else if (dwInfo == PVR_NEXT_RECORDING_TITLE)
  {
    static CStdString strReturn = m_strNextRecordingTitle;
    return strReturn.c_str();
  }
  else if (dwInfo == PVR_NEXT_RECORDING_CHANNEL)
  {
    static CStdString strReturn = m_strNextRecordingChannelName;
    return strReturn.c_str();
  }
  else if (dwInfo == PVR_NEXT_RECORDING_DATETIME)
  {
    static CStdString strReturn = m_strNextRecordingTime;
    return strReturn.c_str();
  }
  else if (dwInfo == PVR_PLAYING_DURATION)       return CharInfoPlayingDuration();
  else if (dwInfo == PVR_PLAYING_TIME)           return CharInfoPlayingTime();
  else if (dwInfo == PVR_NEXT_TIMER)             return CharInfoNextTimer();
  else if (dwInfo == PVR_ACTUAL_STREAM_VIDEO_BR) return m_addons->CharInfoVideoBR();
  else if (dwInfo == PVR_ACTUAL_STREAM_AUDIO_BR) return m_addons->CharInfoAudioBR();
  else if (dwInfo == PVR_ACTUAL_STREAM_DOLBY_BR) return m_addons->CharInfoDolbyBR();
  else if (dwInfo == PVR_ACTUAL_STREAM_SIG)      return m_addons->CharInfoSignal();
  else if (dwInfo == PVR_ACTUAL_STREAM_SNR)      return m_addons->CharInfoSNR();
  else if (dwInfo == PVR_ACTUAL_STREAM_BER)      return m_addons->CharInfoBER();
  else if (dwInfo == PVR_ACTUAL_STREAM_UNC)      return m_addons->CharInfoUNC();
  else if (dwInfo == PVR_ACTUAL_STREAM_CLIENT)   return m_addons->CharInfoPlayingClientName();
  else if (dwInfo == PVR_ACTUAL_STREAM_DEVICE)   return m_addons->CharInfoFrontendName();
  else if (dwInfo == PVR_ACTUAL_STREAM_STATUS)   return m_addons->CharInfoFrontendStatus();
  else if (dwInfo == PVR_ACTUAL_STREAM_CRYPTION) return m_addons->CharInfoEncryption();
  else if (dwInfo == PVR_BACKEND_NAME)           return m_addons->CharInfoBackendName();
  else if (dwInfo == PVR_BACKEND_VERSION)        return m_addons->CharInfoBackendVersion();
  else if (dwInfo == PVR_BACKEND_HOST)           return m_addons->CharInfoBackendHost();
  else if (dwInfo == PVR_BACKEND_DISKSPACE)      return m_addons->CharInfoBackendDiskspace();
  else if (dwInfo == PVR_BACKEND_CHANNELS)       return m_addons->CharInfoBackendChannels();
  else if (dwInfo == PVR_BACKEND_TIMERS)         return m_addons->CharInfoBackendTimers();
  else if (dwInfo == PVR_BACKEND_RECORDINGS)     return m_addons->CharInfoBackendRecordings();
  else if (dwInfo == PVR_BACKEND_NUMBER)         return m_addons->CharInfoBackendNumber();
  else if (dwInfo == PVR_TOTAL_DISKSPACE)        return m_addons->CharInfoTotalDiskSpace();
  return "";
}

bool CPVRManager::TranslateBoolInfo(DWORD dwInfo)
{
  bool bReturn = false;

  if (dwInfo == PVR_IS_RECORDING)
    bReturn = m_bLoaded && m_bIsRecording;
  else if (dwInfo == PVR_HAS_TIMER)
    bReturn = m_bLoaded && m_bHasTimers;
  else if (dwInfo == PVR_IS_PLAYING_TV)
    bReturn = m_bLoaded && m_addons->IsPlayingTV();
  else if (dwInfo == PVR_IS_PLAYING_RADIO)
    bReturn = m_bLoaded && m_addons->IsPlayingRadio();
  else if (dwInfo == PVR_IS_PLAYING_RECORDING)
    bReturn = m_bLoaded && m_addons->IsPlayingRecording();
  else if (dwInfo == PVR_ACTUAL_STREAM_ENCRYPTED)
    bReturn = m_bLoaded && m_addons->IsEncrypted();

  return bReturn;
}

int CPVRManager::TranslateIntInfo(DWORD dwInfo)
{
  int iReturn = 0;

  if (dwInfo == PVR_PLAYING_PROGRESS)
    iReturn = (int) ((float) GetStartTime() / GetTotalTime() * 100);
  else if (dwInfo == PVR_ACTUAL_STREAM_SIG_PROGR)
    iReturn = m_addons->GetSignalLevel();
  else if (dwInfo == PVR_ACTUAL_STREAM_SNR_PROGR)
    iReturn = m_addons->GetSNR();

  return iReturn;
}

const char *CPVRManager::CharInfoPlayingDuration(void)
{
  m_strPlayingDuration = StringUtils::SecondsToTimeString(GetTotalTime()/1000, TIME_FORMAT_GUESS);
  return m_strPlayingDuration.c_str();
}

const char *CPVRManager::CharInfoPlayingTime(void)
{
  m_strPlayingTime = StringUtils::SecondsToTimeString(GetStartTime()/1000, TIME_FORMAT_GUESS);
  return m_strPlayingTime.c_str();
}

const char *CPVRManager::CharInfoNextTimer(void)
{
  static CStdString strReturn = m_strNextTimerInfo;
  return strReturn.c_str();
}

void CPVRManager::UpdateTimersCache(void)
{
  CSingleLock lock(m_critSection);

  /* reset values */
  m_strActiveTimerTitle         = "";
  m_strActiveTimerChannelName   = "";
  m_strActiveTimerTime          = "";
  m_strNextRecordingTitle       = "";
  m_strNextRecordingChannelName = "";
  m_strNextRecordingTime        = "";
  m_strNextTimerInfo            = "";
  m_bIsRecording = false;
  m_NowRecording.clear();
  m_NextRecording = NULL;

  /* fill values */
  m_bHasTimers = m_timers->GetNumTimers() > 0;
  if (m_bHasTimers)
  {
    m_timers->GetActiveTimers(&m_NowRecording);

    /* set the active timer info locally if we're recording right now */
    m_bIsRecording = m_timers->IsRecording();
    if (m_bIsRecording)
    {
      const CPVRTimerInfoTag *tag = m_NowRecording.at(0);
      m_strActiveTimerTitle.Format("%s",       tag->m_strTitle);
      m_strActiveTimerChannelName.Format("%s", tag->ChannelName());
      m_strActiveTimerTime.Format("%s",        tag->StartAsLocalTime().GetAsLocalizedDateTime(false, false));
    }

    /* set the next timer info locally if there is a next timer */
    m_NextRecording = m_timers->GetNextActiveTimer();
    if (m_NextRecording != NULL)
    {
      m_strNextRecordingTitle.Format("%s",       m_NextRecording->m_strTitle);
      m_strNextRecordingChannelName.Format("%s", m_NextRecording->ChannelName());
      m_strNextRecordingTime.Format("%s",        m_NextRecording->StartAsLocalTime().GetAsLocalizedDateTime(false, false));

      m_strNextTimerInfo.Format("%s %s %s %s",
          g_localizeStrings.Get(19106),
          m_NextRecording->StartAsLocalTime().GetAsLocalizedDate(true),
          g_localizeStrings.Get(19107),
          m_NextRecording->StartAsLocalTime().GetAsLocalizedTime("HH:mm", false));
    }
  }
}

void CPVRManager::UpdateRecordingsCache(void)
{
  CSingleLock lock(m_critSection);
  m_bHasRecordings = m_recordings->GetNumRecordings() > 0;
}

void CPVRManager::UpdateRecordingToggle(void)
{
  CSingleLock lock(m_critSection);

  if (m_recordingToggleStart == 0)
  {
    /* set initial values */
    m_recordingToggleStart = CTimeUtils::GetTimeMS();
    m_recordingToggleCurrent = 0;
  }
  else
  {
    if (CTimeUtils::GetTimeMS() - m_recordingToggleStart > INFO_TOGGLE_TIME)
    {
      /* switch */
      if (m_NowRecording.size() > 0)
      {
        m_recordingToggleCurrent++;
        if (m_recordingToggleCurrent > m_NowRecording.size()-1)
          m_recordingToggleCurrent = 0;

        m_recordingToggleStart = CTimeUtils::GetTimeMS();
      }
    }
  }

  if (m_NowRecording.size() > 0 && m_recordingToggleCurrent < m_NowRecording.size())
  {
    const CPVRTimerInfoTag *tag = m_NowRecording.at(m_recordingToggleCurrent);
    m_strActiveTimerTitle       = tag->m_strTitle;
    m_strActiveTimerChannelName = tag->ChannelName();
    m_strActiveTimerTime        = tag->StartAsLocalTime().GetAsLocalizedDateTime(false, false);
  }
  else
  {
    m_strActiveTimerTitle       = "";
    m_strActiveTimerChannelName = "";
    m_strActiveTimerTime        = "";
  }
}


const CStdString &CPVRManager::ConvertGenreIdToString(int iID, int iSubID)
{
  unsigned int iLabelId = 19499;
  switch (iID)
  {
    case EPG_EVENT_CONTENTMASK_MOVIEDRAMA:
      iLabelId = (iSubID <= 8) ? 19500 + iSubID : 19500;
      break;
    case EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS:
      iLabelId = (iSubID <= 4) ? 19516 + iSubID : 19516;
      break;
    case EPG_EVENT_CONTENTMASK_SHOW:
      iLabelId = (iSubID <= 3) ? 19532 + iSubID : 19532;
      break;
    case EPG_EVENT_CONTENTMASK_SPORTS:
      iLabelId = (iSubID <= 11) ? 19548 + iSubID : 19548;
      break;
    case EPG_EVENT_CONTENTMASK_CHILDRENYOUTH:
      iLabelId = (iSubID <= 5) ? 19564 + iSubID : 19564;
      break;
    case EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE:
      iLabelId = (iSubID <= 6) ? 19580 + iSubID : 19580;
      break;
    case EPG_EVENT_CONTENTMASK_ARTSCULTURE:
      iLabelId = (iSubID <= 11) ? 19596 + iSubID : 19596;
      break;
    case EPG_EVENT_CONTENTMASK_SOCIALPOLITICALECONOMICS:
      iLabelId = (iSubID <= 3) ? 19612 + iSubID : 19612;
      break;
    case EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE:
      iLabelId = (iSubID <= 7) ? 19628 + iSubID : 19628;
      break;
    case EPG_EVENT_CONTENTMASK_LEISUREHOBBIES:
      iLabelId = (iSubID <= 7) ? 19644 + iSubID : 19644;
      break;
    case EPG_EVENT_CONTENTMASK_SPECIAL:
      iLabelId = (iSubID <= 3) ? 19660 + iSubID : 19660;
      break;
    case EPG_EVENT_CONTENTMASK_USERDEFINED:
      iLabelId = (iSubID <= 3) ? 19676 + iSubID : 19676;
      break;
    default:
      break;
  }

  return g_localizeStrings.Get(iLabelId);
}

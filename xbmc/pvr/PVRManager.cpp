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
#include "epg/PVREpgInfoTag.h"
#include "recordings/PVRRecording.h"
#include "timers/PVRTimerInfoTag.h"

using namespace std;
using namespace XFILE;
using namespace MUSIC_INFO;
using namespace ADDON;

CPVRManager *CPVRManager::m_instance = NULL;

CPVRManager::CPVRManager() :
    Observer()
{
  m_bFirstStart              = true;
  m_bLoaded                  = false;
  m_bTriggerChannelsUpdate   = false;
  m_bTriggerRecordingsUpdate = false;
  m_bTriggerTimersUpdate     = false;
  m_currentFile              = NULL;
  m_addons                   = new CPVRClients();
  m_channelGroups            = new CPVRChannelGroupsContainer();
  m_epg                      = new CPVREpgContainer();
  m_recordings               = new CPVRRecordings();
  m_timers                   = new CPVRTimers();
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
  if (!m_bStop && msg.Equals("epg"))
  {
    TriggerTimersUpdate();
  }
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
  if (m_addons->IsPlayingRadio() && m_currentRadioGroup)
    channel = m_currentRadioGroup->GetByChannelNumber(iChannel);
  else if (m_addons->IsPlayingTV() && m_currentRadioGroup)
    channel = m_currentTVGroup->GetByChannelNumber(iChannel);

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
      const CPVRChannel *newChannel = bUp ? group->GetByChannelUp(&currentChannel) : group->GetByChannelDown(&currentChannel);
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
  m_hasRecordings            = false;
  m_isRecording              = false;
  m_hasTimers                = false;
  m_bTriggerChannelsUpdate   = false;
  m_bTriggerRecordingsUpdate = false;
  m_bTriggerTimersUpdate     = false;
  m_currentRadioGroup        = NULL;
  m_currentTVGroup           = NULL;
  m_PreviousChannel[0]       = -1;
  m_PreviousChannel[1]       = -1;
  m_PreviousChannelIndex     = 0;
  m_infoToggleStart          = NULL;
  m_infoToggleCurrent        = 0;
  m_recordingToggleStart     = NULL;
  m_recordingToggleCurrent   = 0;
  m_LastChannel              = 0;
}

void CPVRManager::UpdateTimers(void)
{
  CSingleLock lock(m_critSectionTriggers);
  if (!m_bTriggerRecordingsUpdate)
    return;
  lock.Leave();

  CLog::Log(LOGDEBUG, "PVRManager - %s - updating timers", __FUNCTION__);

  m_timers->Update();
  UpdateRecordingsCache();
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
  if (!m_bTriggerChannelsUpdate)
    return;
  lock.Leave();

  CLog::Log(LOGDEBUG, "PVRManager - %s - updating channel list", __FUNCTION__);

  m_channelGroups->Update();
  UpdateRecordingsCache();
  UpdateWindow(PVR_WINDOW_CHANNELS_TV);
  UpdateWindow(PVR_WINDOW_CHANNELS_RADIO);

  lock.Enter();
  m_bTriggerChannelsUpdate = false;
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

      /* get timers from the backends */
      m_timers->Load();

      /* get recordings from the backend */
      m_recordings->Load();

      /* start the EPG thread */
      m_epg->AddObserver(this);
      m_epg->Start();
    }

    /* check if there are (still) any enabled addons */
    if (DisableIfNoClients())
    {
      CLog::Log(LOGDEBUG, "PVRManager - %s - no clients could be found. aborting startup", __FUNCTION__);
      return;
    }
  }

  m_bLoaded = true;

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

void CPVRManager::UpdateRecordingsCache(void)
{
  CSingleLock lock(m_critSection);

  m_hasRecordings = m_recordings->GetNumRecordings() > 0;
  m_hasTimers = m_timers->GetNumTimers() > 0;
  m_isRecording = false;
  m_NowRecording.clear();
  m_NextRecording = NULL;

  if (m_hasTimers)
  {
    CDateTime now = CDateTime::GetCurrentDateTime();
    for (unsigned int iTimerPtr = 0; iTimerPtr < m_timers->size(); iTimerPtr++)
    {
      CPVRTimerInfoTag *timerTag = m_timers->at(iTimerPtr);
      if (timerTag->m_bIsActive)
      {
        if (timerTag->m_StartTime <= now && timerTag->m_StopTime > now)
        {
          m_NowRecording.push_back(timerTag);
          m_isRecording = true;
        }
        if (!m_NextRecording || m_NextRecording->m_StartTime > timerTag->m_StartTime)
        {
          m_NextRecording = timerTag;
        }
      }
    }
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
  bool iReturn = -1;

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

void CPVRManager::TriggerRecordingsUpdate()
{
  CSingleLock lock(m_critSectionTriggers);
  m_bTriggerRecordingsUpdate = true;
}

void CPVRManager::TriggerTimersUpdate()
{
  CSingleLock lock(m_critSectionTriggers);
  m_bTriggerTimersUpdate = true;
}

void CPVRManager::TriggerChannelsUpdate()
{
  CSingleLock lock(m_critSectionTriggers);
  m_bTriggerChannelsUpdate = true;
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
    CDateTimeSpan time = CDateTime::GetCurrentDateTime() - tag->Start();
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
  if      (dwInfo == PVR_PLAYING_DURATION)        return CharInfoPlayingDuration();
  else if (dwInfo == PVR_PLAYING_TIME)            return CharInfoPlayingTime();
  else if (dwInfo == PVR_NOW_RECORDING_TITLE)     return CharInfoNowRecordingTitle();
  else if (dwInfo == PVR_NOW_RECORDING_CHANNEL)   return CharInfoNowRecordingChannel();
  else if (dwInfo == PVR_NOW_RECORDING_DATETIME)  return CharInfoNowRecordingDateTime();
  else if (dwInfo == PVR_NEXT_RECORDING_TITLE)    return m_NextRecording ? m_NextRecording->m_strTitle : "";
  else if (dwInfo == PVR_NEXT_RECORDING_CHANNEL)  return m_NextRecording ? m_NextRecording->ChannelName() : "";
  else if (dwInfo == PVR_NEXT_RECORDING_DATETIME) return m_NextRecording ? m_NextRecording->m_StartTime.GetAsLocalizedDateTime(false, false) : "";
  else if (dwInfo == PVR_NEXT_TIMER)              return CharInfoNextTimer();
  else if (dwInfo == PVR_ACTUAL_STREAM_VIDEO_BR)  return m_addons->CharInfoVideoBR();
  else if (dwInfo == PVR_ACTUAL_STREAM_AUDIO_BR)  return m_addons->CharInfoAudioBR();
  else if (dwInfo == PVR_ACTUAL_STREAM_DOLBY_BR)  return m_addons->CharInfoDolbyBR();
  else if (dwInfo == PVR_ACTUAL_STREAM_SIG)       return m_addons->CharInfoSignal();
  else if (dwInfo == PVR_ACTUAL_STREAM_SNR)       return m_addons->CharInfoSNR();
  else if (dwInfo == PVR_ACTUAL_STREAM_BER)       return m_addons->CharInfoBER();
  else if (dwInfo == PVR_ACTUAL_STREAM_UNC)       return m_addons->CharInfoUNC();
  else if (dwInfo == PVR_ACTUAL_STREAM_CLIENT)    return m_addons->CharInfoPlayingClientName();
  else if (dwInfo == PVR_ACTUAL_STREAM_DEVICE)    return m_addons->CharInfoFrontendName();
  else if (dwInfo == PVR_ACTUAL_STREAM_STATUS)    return m_addons->CharInfoFrontendStatus();
  else if (dwInfo == PVR_ACTUAL_STREAM_CRYPTION)  return m_addons->CharInfoEncryption();
  else if (dwInfo == PVR_BACKEND_NAME)            return m_addons->CharInfoBackendName();
  else if (dwInfo == PVR_BACKEND_VERSION)         return m_addons->CharInfoBackendVersion();
  else if (dwInfo == PVR_BACKEND_HOST)            return m_addons->CharInfoBackendHost();
  else if (dwInfo == PVR_BACKEND_DISKSPACE)       return m_addons->CharInfoBackendDiskspace();
  else if (dwInfo == PVR_BACKEND_CHANNELS)        return m_addons->CharInfoBackendChannels();
  else if (dwInfo == PVR_BACKEND_TIMERS)          return m_addons->CharInfoBackendTimers();
  else if (dwInfo == PVR_BACKEND_RECORDINGS)      return m_addons->CharInfoBackendRecordings();
  else if (dwInfo == PVR_BACKEND_NUMBER)          return m_addons->CharInfoBackendNumber();
  else if (dwInfo == PVR_TOTAL_DISKSPACE)         return m_addons->CharInfoTotalDiskSpace();
  return "";
}

bool CPVRManager::TranslateBoolInfo(DWORD dwInfo)
{
  bool bReturn = false;

  if (dwInfo == PVR_IS_RECORDING)
    bReturn = m_bLoaded && m_isRecording;
  else if (dwInfo == PVR_HAS_TIMER)
    bReturn = m_bLoaded && m_hasTimers;
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
    iReturn = (float) GetStartTime() / GetTotalTime() * 100;
  else if (dwInfo == PVR_ACTUAL_STREAM_SIG_PROGR)
    iReturn = m_addons->GetSignalLevel();
  else if (dwInfo == PVR_ACTUAL_STREAM_SNR_PROGR)
    iReturn = m_addons->GetSNR();

  return iReturn;
}

const char *CPVRManager::CharInfoPlayingDuration(void)
{
  m_playingDuration = StringUtils::SecondsToTimeString(GetTotalTime()/1000, TIME_FORMAT_GUESS);
  return m_playingDuration.c_str();
}

const char *CPVRManager::CharInfoPlayingTime(void)
{
  m_playingTime = StringUtils::SecondsToTimeString(GetStartTime()/1000, TIME_FORMAT_GUESS);
  return m_playingTime.c_str();
}

const char *CPVRManager::CharInfoNowRecordingTitle(void)
{
  if (m_recordingToggleStart == 0)
  {
    m_recordingToggleStart = CTimeUtils::GetTimeMS();
    m_recordingToggleCurrent = 0;
  }
  else
  {
    if (CTimeUtils::GetTimeMS() - m_recordingToggleStart > INFO_TOGGLE_TIME)
    {
      if (m_NowRecording.size() > 0)
      {
        m_recordingToggleCurrent++;
        if (m_recordingToggleCurrent > m_NowRecording.size()-1)
          m_recordingToggleCurrent = 0;

        m_recordingToggleStart = CTimeUtils::GetTimeMS();
      }
    }
  }

  return (m_NowRecording.size() >= m_recordingToggleCurrent + 1) ?
    m_NowRecording[m_recordingToggleCurrent]->m_strTitle :
    "";
}

const char *CPVRManager::CharInfoNowRecordingChannel(void)
{
  static CStdString strReturn = "";

  if (m_NowRecording.size() > 0)
  {
    CPVRTimerInfoTag * timerTag = m_NowRecording[m_recordingToggleCurrent];
    strReturn = timerTag ? timerTag->ChannelName() : "";
  }

  return strReturn;
}

const char *CPVRManager::CharInfoNowRecordingDateTime(void)
{
  static CStdString strReturn = "";

  if (m_NowRecording.size() > 0)
  {
    CPVRTimerInfoTag *timerTag = m_NowRecording[m_recordingToggleCurrent];
    strReturn = timerTag ? timerTag->m_StartTime.GetAsLocalizedDateTime(false, false) : "";
  }

  return strReturn;
}

const char *CPVRManager::CharInfoNextTimer(void)
{
  static CStdString strReturn = "";
  CPVRTimerInfoTag next;
  if (m_timers->GetNextActiveTimer(&next))
  {
    m_nextTimer.Format("%s %s %s %s", g_localizeStrings.Get(19106),
        next.m_StartTime.GetAsLocalizedDate(true),
        g_localizeStrings.Get(19107),
        next.m_StartTime.GetAsLocalizedTime("HH:mm", false));
    strReturn = m_nextTimer;
  }

  return strReturn;
}

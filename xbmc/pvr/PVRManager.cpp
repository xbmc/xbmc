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
#include "GUIInfoManager.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "windows/GUIWindowPVR.h"
#include "utils/log.h"

#include "PVRManager.h"
#include "PVRDatabase.h"
#include "PVRGUIInfo.h"
#include "addons/PVRClients.h"
#include "channels/PVRChannelGroupsContainer.h"
#include "epg/EpgContainer.h"
#include "recordings/PVRRecordings.h"
#include "timers/PVRTimers.h"

using namespace std;
using namespace MUSIC_INFO;
using namespace PVR;
using namespace EPG;

CPVRManager::CPVRManager(void) :
    CThread("PVR manager"),
    m_channelGroups(NULL),
    m_recordings(NULL),
    m_timers(NULL),
    m_addons(NULL),
    m_guiInfo(NULL),
    m_triggerEvent(true),
    m_currentFile(NULL),
    m_database(NULL),
    m_bFirstStart(true),
    m_bLoaded(false),
    m_bIsStopping(false),
    m_loadingProgressDialog(NULL),
    m_currentRadioGroup(NULL),
    m_currentTVGroup(NULL)
{
  ResetProperties();
}

CPVRManager::~CPVRManager(void)
{
  Stop();
  Cleanup();

  CLog::Log(LOGDEBUG,"PVRManager - destroyed");
}

void CPVRManager::Notify(const Observable &obs, const CStdString& msg)
{
  // TODO process notifications from the EPG
}

CPVRManager &CPVRManager::Get(void)
{
  static CPVRManager pvrManagerInstance;
  return pvrManagerInstance;
}

void CPVRManager::Start(void)
{
  /* first stop and remove any clients */
  Stop();

  /* don't start if Settings->Video->TV->Enable isn't checked */
  if (!g_guiSettings.GetBool("pvrmanager.enabled"))
    return;

  ResetProperties();

  /* create the supervisor thread to do all background activities */
  StartUpdateThreads();
}

void CPVRManager::Stop(void)
{
  /* check whether the pvrmanager is loaded */
  CSingleLock lock(m_critSection);
  if (m_bIsStopping)
    return;

  if (!m_bLoaded)
    return;

  m_bIsStopping = true;
  m_bLoaded = false;
  lock.Leave();

  CLog::Log(LOGNOTICE, "PVRManager - stopping");

  /* stop playback if needed */
  if (IsPlaying())
  {
    CLog::Log(LOGNOTICE,"PVRManager - %s - stopping PVR playback", __FUNCTION__);
    g_application.getApplicationMessenger().MediaStop();
  }

  /* stop all update threads */
  lock.Enter();
  StopUpdateThreads();

  /* unload all data */
  g_EpgContainer.UnregisterObserver(this);

  m_recordings->Unload();
  m_timers->Unload();
  m_channelGroups->Unload();
  m_addons->Unload();
  m_bIsStopping = false;
}

bool CPVRManager::StartUpdateThreads(void)
{
  StopUpdateThreads();
  CLog::Log(LOGNOTICE, "PVRManager - starting up");

  /* create the pvrmanager thread, which will ensure that all data will be loaded */
  Create();
  SetPriority(-1);

  return true;
}

void CPVRManager::StopUpdateThreads(void)
{
  StopThread();
  g_EpgContainer.UnregisterObserver(this);
  m_guiInfo->Stop();
  m_addons->Stop();
}

void CPVRManager::Cleanup(void)
{
  CSingleLock lock(m_critSection);
  if (m_addons)        delete m_addons;        m_addons = NULL;
  if (m_guiInfo)       delete m_guiInfo;       m_guiInfo = NULL;
  if (m_timers)        delete m_timers;        m_timers = NULL;
  if (m_recordings)    delete m_recordings;    m_recordings = NULL;
  if (m_channelGroups) delete m_channelGroups; m_channelGroups = NULL;
  if (m_database)      delete m_database;      m_database = NULL;
  m_triggerEvent.Set();
}

bool CPVRManager::Load(void)
{
  /* start the add-on update thread */
  m_addons->Start();

  /* load at least one client */
  while (!g_application.m_bStop && !m_bStop && m_addons && !m_addons->HasConnectedClients())
    Sleep(50);

  bool bChannelsLoaded(false);
  if (!g_application.m_bStop && !m_bStop && !m_bLoaded && m_addons->HasConnectedClients())
  {
    CLog::Log(LOGDEBUG, "PVRManager - %s - active clients found. continue to start", __FUNCTION__);

    /* load all channels and groups */
    if (!m_bStop)
    {
      ShowProgressDialog(g_localizeStrings.Get(19236), 0);
      bChannelsLoaded = m_channelGroups->Load();
    }

    /* get timers from the backends */
    if (!m_bStop && bChannelsLoaded)
    {
      ShowProgressDialog(g_localizeStrings.Get(19237), 50);
      m_timers->Load();
    }

    /* get recordings from the backend */
    if (!m_bStop && bChannelsLoaded)
    {
      ShowProgressDialog(g_localizeStrings.Get(19238), 75);
      m_recordings->Load();
    }

    /* reset observers that are observing pvr related data in the pvr windows, or updates won't work after a reload */
    if (!m_bStop && bChannelsLoaded)
    {
      CGUIWindowPVR *pWindow = (CGUIWindowPVR *) g_windowManager.GetWindow(WINDOW_PVR);
      if (pWindow)
        pWindow->Reset();
    }

    /* start the other pvr related update threads */
    if (!m_bStop && bChannelsLoaded)
    {
      ShowProgressDialog(g_localizeStrings.Get(19239), 85);
      m_guiInfo->Start();
      g_EpgContainer.RegisterObserver(this);

      m_bLoaded = true;
    }
  }

  /* close the progess dialog */
  HideProgressDialog();

  return m_bLoaded;
}

void CPVRManager::ShowProgressDialog(const CStdString &strText, int iProgress)
{
  if (!m_loadingProgressDialog)
  {
    m_loadingProgressDialog = (CGUIDialogExtendedProgressBar *)g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS);
    m_loadingProgressDialog->Show();
    m_loadingProgressDialog->SetHeader(g_localizeStrings.Get(19235));
  }

  m_loadingProgressDialog->SetProgress(iProgress, 100);
  m_loadingProgressDialog->SetTitle(strText);
  m_loadingProgressDialog->UpdateState();
}

void CPVRManager::HideProgressDialog(void)
{
  if (m_loadingProgressDialog)
  {
    m_loadingProgressDialog->Close(true, 0, true, false);
    m_loadingProgressDialog = NULL;
  }
}

void CPVRManager::Process(void)
{
  /* load the pvr data from the db and clients if it's not already loaded */
  if (!Load())
  {
    CLog::Log(LOGERROR, "PVRManager - %s - failed to load PVR data", __FUNCTION__);
    return;
  }

  /* continue last watched channel after first startup */
  if (!m_bStop && m_bFirstStart && g_guiSettings.GetInt("pvrplayback.startlast") != START_LAST_CHANNEL_OFF)
    ContinueLastChannel();

  /* signal to window that clients are loaded */
  CGUIWindowPVR *pWindow = (CGUIWindowPVR *) g_windowManager.GetWindow(WINDOW_PVR);
  if (pWindow)
    pWindow->UnlockWindow();

  /* check whether all channel icons are cached */
  m_channelGroups->GetGroupAllRadio()->CacheIcons();
  m_channelGroups->GetGroupAllTV()->CacheIcons();

  CLog::Log(LOGDEBUG, "PVRManager - %s - entering main loop", __FUNCTION__);

  /* main loop */
  while (!g_application.m_bStop && !m_bStop)
  {
    /* execute the next pending jobs if there are any */
    if (m_addons->HasConnectedClients())
      ExecutePendingJobs();

    /* check if the (still) are any enabled addons */
    if (!m_bStop && !m_addons->HasConnectedClients())
    {
      CLog::Log(LOGNOTICE, "PVRManager - %s - no add-ons enabled anymore. restarting the pvrmanager", __FUNCTION__);
      Stop();
      Start();
      return;
    }

    if (!m_bStop)
      m_triggerEvent.WaitMSec(1000);
  }

}

bool CPVRManager::ChannelSwitch(unsigned int iChannelNumber)
{
  CSingleLock lock(m_critSection);

  const CPVRChannelGroup *playingGroup = GetPlayingGroup(m_addons->IsPlayingRadio());
  if (playingGroup == NULL)
  {
    CLog::Log(LOGERROR, "PVRManager - %s - cannot get the selected group", __FUNCTION__);
    return false;
  }

  const CPVRChannel *channel = playingGroup->GetByChannelNumber(iChannelNumber);
  if (channel == NULL)
  {
    CLog::Log(LOGERROR, "PVRManager - %s - cannot find channel %d", __FUNCTION__, iChannelNumber);
    return false;
  }

  return PerformChannelSwitch(*channel, false);
}

bool CPVRManager::ChannelUpDown(unsigned int *iNewChannelNumber, bool bPreview, bool bUp)
{
  bool bReturn = false;
  if (IsPlayingTV() || IsPlayingRadio())
  {
    CFileItem currentFile(g_application.CurrentFileItem());
    CPVRChannel *currentChannel = currentFile.GetPVRChannelInfoTag();
    const CPVRChannelGroup *group = GetPlayingGroup(currentChannel->IsRadio());
    if (group)
    {
      const CPVRChannel *newChannel = bUp ? group->GetByChannelUp(*currentChannel) : group->GetByChannelDown(*currentChannel);
      if (newChannel && PerformChannelSwitch(*newChannel, bPreview))
      {
        *iNewChannelNumber = newChannel->ChannelNumber();
        bReturn = true;
      }
    }
  }

  return bReturn;
}

bool CPVRManager::ContinueLastChannel(void)
{
  CSingleLock lock(m_critSection);
  if (!m_bFirstStart)
    return true;
  m_bFirstStart = false;
  lock.Leave();

  bool bReturn(false);
  const CPVRChannel *channel = m_channelGroups->GetLastPlayedChannel();
  if (channel != NULL)
  {
    CLog::Log(LOGNOTICE, "PVRManager - %s - continue playback on channel '%s'",
        __FUNCTION__, channel->ChannelName().c_str());
    bReturn = StartPlayback(channel, (g_guiSettings.GetInt("pvrplayback.startlast") == START_LAST_CHANNEL_MIN));
  }

  return bReturn;
}

void CPVRManager::ResetProperties(void)
{
  if (!g_application.m_bStop)
  {
    if (!m_database)      m_database      = new CPVRDatabase;
    if (!m_addons)        m_addons        = new CPVRClients;
    if (!m_channelGroups) m_channelGroups = new CPVRChannelGroupsContainer;
    if (!m_recordings)    m_recordings    = new CPVRRecordings;
    if (!m_timers)        m_timers        = new CPVRTimers;
    if (!m_guiInfo)       m_guiInfo       = new CPVRGUIInfo;
  }

  m_currentFile           = NULL;
  m_currentRadioGroup     = NULL;
  m_currentTVGroup        = NULL;
  m_PreviousChannel[0]    = -1;
  m_PreviousChannel[1]    = -1;
  m_PreviousChannelIndex  = 0;
  m_LastChannel           = 0;
  m_bIsSwitchingChannels  = false;

  for (unsigned int iJobPtr = 0; iJobPtr < m_pendingUpdates.size(); iJobPtr++)
    delete m_pendingUpdates.at(iJobPtr);
  m_pendingUpdates.clear();
}

void CPVRManager::ResetDatabase(bool bShowProgress /* = true */)
{
  CLog::Log(LOGNOTICE,"PVRManager - %s - clearing the PVR database", __FUNCTION__);

  g_EpgContainer.Stop();

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
    g_application.getApplicationMessenger().MediaStop();
  }

  if (bShowProgress)
  {
    pDlgProgress->SetPercentage(10);
    pDlgProgress->Progress();
  }

  /* stop the thread */
  if (g_guiSettings.GetBool("pvrmanager.enabled"))
    Stop();

  if (bShowProgress)
  {
    pDlgProgress->SetPercentage(20);
    pDlgProgress->Progress();
  }

  if (m_database->Open())
  {
    /* clean the EPG database */
    g_EpgContainer.Clear(true);
    if (bShowProgress)
    {
      pDlgProgress->SetPercentage(30);
      pDlgProgress->Progress();
    }

    m_database->DeleteChannelGroups();
    if (bShowProgress)
    {
      pDlgProgress->SetPercentage(50);
      pDlgProgress->Progress();
    }

    /* delete all channels */
    m_database->DeleteChannels();
    if (bShowProgress)
    {
      pDlgProgress->SetPercentage(70);
      pDlgProgress->Progress();
    }

    /* delete all channel settings */
    m_database->DeleteChannelSettings();
    if (bShowProgress)
    {
      pDlgProgress->SetPercentage(80);
      pDlgProgress->Progress();
    }

    /* delete all client information */
    m_database->DeleteClients();
    if (bShowProgress)
    {
      pDlgProgress->SetPercentage(90);
      pDlgProgress->Progress();
    }

    m_database->Close();
  }

  CLog::Log(LOGNOTICE,"PVRManager - %s - PVR database cleared", __FUNCTION__);

  g_EpgContainer.Start();

  if (g_guiSettings.GetBool("pvrmanager.enabled"))
  {
    CLog::Log(LOGNOTICE,"PVRManager - %s - restarting the PVRManager", __FUNCTION__);
    Cleanup();
    Start();
  }

  if (bShowProgress)
  {
    pDlgProgress->SetPercentage(100);
    pDlgProgress->Close();
  }
}

void CPVRManager::ResetEPG(void)
{
  CLog::Log(LOGNOTICE,"PVRManager - %s - clearing the EPG database", __FUNCTION__);

  StopUpdateThreads();
  g_EpgContainer.Stop();
  g_EpgContainer.Reset();

  if (g_guiSettings.GetBool("pvrmanager.enabled"))
  {
    m_channelGroups->GetGroupAllTV()->CreateChannelEpgs(true);
    m_channelGroups->GetGroupAllRadio()->CreateChannelEpgs(true);
    g_EpgContainer.Start();
    StartUpdateThreads();
  }
}

bool CPVRManager::IsPlaying(void) const
{
  return m_bLoaded && m_addons->IsPlaying();
}

bool CPVRManager::GetCurrentChannel(CPVRChannel *channel) const
{
  return m_addons->GetPlayingChannel(channel);
}

int CPVRManager::GetCurrentEpg(CFileItemList &results) const
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

void CPVRManager::SaveCurrentChannelSettings(void)
{
  m_addons->SaveCurrentChannelSettings();
}

void CPVRManager::LoadCurrentChannelSettings()
{
  m_addons->LoadCurrentChannelSettings();
}

void CPVRManager::SetPlayingGroup(CPVRChannelGroup *group)
{
  CSingleLock lock(m_critSection);

  if (group == NULL)
    return;

  bool bChanged(false);
  if (group->IsRadio())
  {
    bChanged = m_currentRadioGroup == NULL || *m_currentRadioGroup != *group;
    m_currentRadioGroup = group;
  }
  else
  {
    bChanged = m_currentTVGroup == NULL || *m_currentTVGroup != *group;
    m_currentTVGroup = group;
  }

  /* set this group as selected group and set channel numbers */
  if (bChanged)
    group->SetSelectedGroup();
}

CPVRChannelGroup *CPVRManager::GetPlayingGroup(bool bRadio /* = false */)
{
  CSingleTryLock tryLock(m_critSection);
  if(tryLock.IsOwner())
  {
    if (bRadio && !m_currentRadioGroup)
      SetPlayingGroup((CPVRChannelGroup *) m_channelGroups->GetGroupAllRadio());
    else if (!bRadio &&!m_currentTVGroup)
      SetPlayingGroup((CPVRChannelGroup *) m_channelGroups->GetGroupAllTV());
  }

  return bRadio ? m_currentRadioGroup : m_currentTVGroup;
}

bool CPVRManager::IsSelectedGroup(const CPVRChannelGroup &group) const
{
  CSingleLock lock(m_critSection);

  return (group.IsRadio() && m_currentRadioGroup && *m_currentRadioGroup == group) ||
      (!group.IsRadio() && m_currentTVGroup && *m_currentTVGroup == group);
}

bool CPVRRecordingsUpdateJob::DoWork(void)
{
  g_PVRRecordings->Update();
  return true;
}

bool CPVRTimersUpdateJob::DoWork(void)
{
  return g_PVRTimers->Update();
}

bool CPVRChannelsUpdateJob::DoWork(void)
{
  return g_PVRChannelGroups->Update(true);
}

bool CPVRChannelGroupsUpdateJob::DoWork(void)
{
  return g_PVRChannelGroups->Update(false);
}

bool CPVRChannelSettingsSaveJob::DoWork(void)
{
  g_PVRManager.SaveCurrentChannelSettings();
  return true;
}

bool CPVRManager::OpenLiveStream(const CPVRChannel &tag)
{
  bool bReturn(false);
  CLog::Log(LOGDEBUG,"PVRManager - %s - opening live stream on channel '%s'",
      __FUNCTION__, tag.ChannelName().c_str());

  if ((bReturn = m_addons->OpenLiveStream(tag)) != false)
  {
    CSingleLock lock(m_critSection);
    if(m_currentFile)
      delete m_currentFile;
    m_currentFile = new CFileItem(tag);
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

void CPVRManager::CloseStream(void)
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
    }
  }

  m_addons->CloseStream();
  if (m_currentFile)
  {
    delete m_currentFile;
    m_currentFile = NULL;
  }
}

void CPVRManager::UpdateCurrentFile(void)
{
  CSingleLock lock(m_critSection);
  if (m_currentFile)
    UpdateItem(*m_currentFile);
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
  if (*m_currentFile->GetPVRChannelInfoTag() == *item.GetPVRChannelInfoTag())
    return false;

  g_application.CurrentFileItem() = *m_currentFile;
  g_infoManager.SetCurrentItem(*m_currentFile);

  CPVRChannel* channelTag = item.GetPVRChannelInfoTag();
  const CEpgInfoTag* epgTagNow = channelTag->GetEPGNow();

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
    m_LastChannelChanged  = XbmcThreads::SystemClockMillis();
  }
  if (XbmcThreads::SystemClockMillis() - m_LastChannelChanged >= (unsigned int) g_guiSettings.GetInt("pvrplayback.channelentrytimeout") && m_LastChannel != m_PreviousChannel[m_PreviousChannelIndex])
     m_PreviousChannel[m_PreviousChannelIndex ^= 1] = m_LastChannel;

  return false;
}

bool CPVRManager::StartPlayback(const CPVRChannel *channel, bool bPreview /* = false */)
{
  g_settings.m_bStartVideoWindowed = bPreview;
  g_application.getApplicationMessenger().MediaPlay(CFileItem(*channel));
  CLog::Log(LOGNOTICE, "PVRManager - %s - started playback on channel '%s'",
      __FUNCTION__, channel->ChannelName().c_str());
  return true;
}

bool CPVRManager::PerformChannelSwitch(const CPVRChannel &channel, bool bPreview)
{
  bool bSwitched(false);

  CSingleLock lock(m_critSection);
  if (m_bIsSwitchingChannels)
  {
    CLog::Log(LOGDEBUG, "PVRManager - %s - can't switch to channel '%s'. waiting for the previous switch to complete",
        __FUNCTION__, channel.ChannelName().c_str());
    return bSwitched;
  }
  m_bIsSwitchingChannels = true;

  CLog::Log(LOGDEBUG, "PVRManager - %s - switching to channel '%s'",
      __FUNCTION__, channel.ChannelName().c_str());

  /* make sure that channel settings are persisted */
  if (!bPreview)
    SaveCurrentChannelSettings();

  if (m_currentFile)
  {
    delete m_currentFile;
    m_currentFile = NULL;
  }

  lock.Leave();

  if (!bPreview && (channel.ClientID() < 0 || !m_addons->SwitchChannel(channel)))
  {
    lock.Enter();
    m_bIsSwitchingChannels = false;
    lock.Leave();

    CLog::Log(LOGERROR, "PVRManager - %s - failed to switch to channel '%s'",
        __FUNCTION__, channel.ChannelName().c_str());
  }
  else
  {
    bSwitched = true;

    lock.Enter();
    m_currentFile = new CFileItem(channel);

    if (!bPreview)
      CLog::Log(LOGNOTICE, "PVRManager - %s - switched to channel '%s'",
          __FUNCTION__, channel.ChannelName().c_str());

    m_bIsSwitchingChannels = false;
  }

  if (!bSwitched)
  {
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error,
        g_localizeStrings.Get(19166),
        g_localizeStrings.Get(19035));
  }

  return bSwitched;
}

int CPVRManager::GetTotalTime(void) const
{
  CSingleLock lock(m_critSection);
  if (!m_bLoaded)
    return 0;
  lock.Leave();

  return !m_guiInfo ? 0 : m_guiInfo->GetDuration();
}

int CPVRManager::GetStartTime(void) const
{
  CSingleLock lock(m_critSection);
  if (!m_bLoaded)
    return 0;
  lock.Leave();

  return !m_guiInfo ? 0 : m_guiInfo->GetStartTime();
}

bool CPVRManager::TranslateBoolInfo(DWORD dwInfo) const
{
  CSingleLock lock(m_critSection);
  if (!m_bLoaded)
    return false;
  lock.Leave();

  return !m_guiInfo ? false : m_guiInfo->TranslateBoolInfo(dwInfo);
}

bool CPVRManager::TranslateCharInfo(DWORD dwInfo, CStdString &strValue) const
{
  CSingleLock lock(m_critSection);
  if (!m_bLoaded)
    return false;
  lock.Leave();

  return !m_guiInfo ? false : m_guiInfo->TranslateCharInfo(dwInfo, strValue);
}

int CPVRManager::TranslateIntInfo(DWORD dwInfo) const
{
  CSingleLock lock(m_critSection);
  if (!m_bLoaded)
    return 0;
  lock.Leave();

  return !m_guiInfo ? 0 : m_guiInfo->TranslateIntInfo(dwInfo);
}

bool CPVRManager::HasTimer(void) const
{
  CSingleLock lock(m_critSection);
  if (!m_bLoaded)
    return false;
  lock.Leave();

  return !m_guiInfo ? false : m_guiInfo->HasTimers();
}

bool CPVRManager::IsRecording(void) const
{
  CSingleLock lock(m_critSection);
  if (!m_bLoaded)
    return false;
  lock.Leave();

  return !m_guiInfo ? false : m_guiInfo->IsRecording();
}

void CPVRManager::ShowPlayerInfo(int iTimeout)
{
  CSingleLock lock(m_critSection);
  if (!m_bLoaded)
    return;
  lock.Leave();

  m_guiInfo->ShowPlayerInfo(iTimeout);
}

void CPVRManager::LocalizationChanged(void)
{
  CSingleLock lock(m_critSection);
  if (m_bLoaded)
  {
    m_channelGroups->GetGroupAllRadio()->CheckGroupName();
    m_channelGroups->GetGroupAllTV()->CheckGroupName();
  }
}

bool CPVRManager::IsRunning(void) const
{
  CSingleLock lock(m_critSection);
  return !m_bStop;
}

bool CPVRManager::IsInitialising(void) const
{
  CSingleLock lock(m_critSection);
  return g_guiSettings.GetBool("pvrmanager.enabled") && !m_bLoaded;
}

bool CPVRManager::IsPlayingTV(void) const
{
  CSingleLock lock(m_critSection);
  if (!m_bLoaded)
    return false;
  lock.Leave();

  return m_addons->IsPlayingTV();
}

bool CPVRManager::IsPlayingRadio(void) const
{
  CSingleLock lock(m_critSection);
  if (!m_bLoaded)
    return false;
  lock.Leave();

  return m_addons->IsPlayingRadio();
}

bool CPVRManager::IsPlayingRecording(void) const
{
  CSingleLock lock(m_critSection);
  if (!m_bLoaded)
    return false;
  lock.Leave();

  return m_addons->IsPlayingRecording();
}

bool CPVRManager::IsRunningChannelScan(void) const
{
  CSingleLock lock(m_critSection);
  if (!m_bLoaded)
    return false;
  lock.Leave();

  return m_addons->IsRunningChannelScan();
}

PVR_ADDON_CAPABILITIES CPVRManager::GetCurrentAddonCapabilities(void)
{
  PVR_ADDON_CAPABILITIES props;
  memset(&props, 0, sizeof(PVR_ADDON_CAPABILITIES));
  CSingleLock lock(m_critSection);
  if (!m_bLoaded)
    return props;
  lock.Leave();

  props = m_addons->GetCurrentAddonCapabilities();

  return props;
}

void CPVRManager::StartChannelScan(void)
{
  CSingleLock lock(m_critSection);
  if (!m_bLoaded)
    return;
  lock.Leave();

  return m_addons->StartChannelScan();
}

void CPVRManager::SearchMissingChannelIcons(void)
{
  CSingleLock lock(m_critSection);
  if (!m_bLoaded)
    return;
  lock.Leave();

  return m_channelGroups->SearchMissingChannelIcons();
}

bool CPVRManager::IsJobPending(const char *strJobName) const
{
  bool bReturn(false);
  CSingleLock lock(m_critSectionTriggers);
  if (!m_bLoaded)
    return bReturn;

  for (unsigned int iJobPtr = 0; iJobPtr < m_pendingUpdates.size(); iJobPtr++)
  {
    if (!strcmp(m_pendingUpdates.at(iJobPtr)->GetType(), "pvr-update-recordings"))
    {
      bReturn = true;
      break;
    }
  }

  return bReturn;
}

void CPVRManager::TriggerRecordingsUpdate(void)
{
  CSingleLock lock(m_critSectionTriggers);
  if (!m_bLoaded)
    return;

  if (IsJobPending("pvr-update-recordings"))
    return;

  m_pendingUpdates.push_back(new CPVRRecordingsUpdateJob());

  lock.Leave();
  m_triggerEvent.Set();
}

void CPVRManager::TriggerTimersUpdate(void)
{
  CSingleLock lock(m_critSectionTriggers);
  if (!m_bLoaded)
    return;

  if (IsJobPending("pvr-update-timers"))
    return;

  m_pendingUpdates.push_back(new CPVRTimersUpdateJob());

  lock.Leave();
  m_triggerEvent.Set();
}

void CPVRManager::TriggerChannelsUpdate(void)
{
  CSingleLock lock(m_critSectionTriggers);
  if (!m_bLoaded)
    return;

  if (IsJobPending("pvr-update-channels"))
    return;

  m_pendingUpdates.push_back(new CPVRChannelsUpdateJob());

  lock.Leave();
  m_triggerEvent.Set();
}

void CPVRManager::TriggerChannelGroupsUpdate(void)
{
  CSingleLock lock(m_critSectionTriggers);
  if (!m_bLoaded)
    return;

  if (IsJobPending("pvr-update-channelgroups"))
    return;

  m_pendingUpdates.push_back(new CPVRChannelGroupsUpdateJob());

  lock.Leave();
  m_triggerEvent.Set();
}

void CPVRManager::TriggerSaveChannelSettings(void)
{
  CSingleLock lock(m_critSectionTriggers);
  if (!m_bLoaded)
    return;

  if (IsJobPending("pvr-save-channelsettings"))
    return;

  m_pendingUpdates.push_back(new CPVRChannelSettingsSaveJob());

  lock.Leave();
  m_triggerEvent.Set();
}

void CPVRManager::ExecutePendingJobs(void)
{
  CSingleLock lock(m_critSectionTriggers);

  while (m_pendingUpdates.size() > 0)
  {
    CJob *job = m_pendingUpdates.at(0);
    m_pendingUpdates.erase(m_pendingUpdates.begin());
    lock.Leave();

    job->DoWork();
    delete job;

    lock.Enter();
  }

  m_triggerEvent.Reset();
}

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
#include "ApplicationMessenger.h"
#include "GUIInfoManager.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "windows/GUIWindowPVR.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "threads/Atomics.h"

#include "PVRManager.h"
#include "PVRDatabase.h"
#include "PVRGUIInfo.h"
#include "addons/PVRClients.h"
#include "channels/PVRChannelGroupsContainer.h"
#include "channels/PVRChannelGroupInternal.h"
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
    m_database(new CPVRDatabase),
    m_bFirstStart(true),
    m_loadingProgressDialog(NULL),
    m_managerState(ManagerStateStopped)
{
  ResetProperties();
}

CPVRManager::~CPVRManager(void)
{
  Stop();
  if (m_database->IsOpen())
    m_database->Close();
  CLog::Log(LOGDEBUG,"PVRManager - destroyed");
}

CPVRManager &CPVRManager::Get(void)
{
  static CPVRManager pvrManagerInstance;
  return pvrManagerInstance;
}

void CPVRManager::Cleanup(void)
{
  CSingleLock lock(m_critSection);

  if (m_addons)        SAFE_DELETE(m_addons);
  if (m_guiInfo)       SAFE_DELETE(m_guiInfo);
  if (m_timers)        SAFE_DELETE(m_timers);
  if (m_recordings)    SAFE_DELETE(m_recordings);
  if (m_channelGroups) SAFE_DELETE(m_channelGroups);
  m_triggerEvent.Set();

  m_currentFile           = NULL;
  m_PreviousChannel[0]    = -1;
  m_PreviousChannel[1]    = -1;
  m_PreviousChannelIndex  = 0;
  m_LastChannel           = 0;
  m_bIsSwitchingChannels  = false;

  for (unsigned int iJobPtr = 0; iJobPtr < m_pendingUpdates.size(); iJobPtr++)
    delete m_pendingUpdates.at(iJobPtr);
  m_pendingUpdates.clear();

  SetState(ManagerStateStopped);
}

void CPVRManager::ResetProperties(void)
{
  CSingleLock lock(m_critSection);
  Cleanup();

  if (!g_application.m_bStop)
  {
    m_addons        = new CPVRClients;
    m_channelGroups = new CPVRChannelGroupsContainer;
    m_recordings    = new CPVRRecordings;
    m_timers        = new CPVRTimers;
    m_guiInfo       = new CPVRGUIInfo;
  }
}

void CPVRManager::Start(void)
{
  CSingleLock lock(m_critSection);

  /* first stop and remove any clients */
  Stop();

  /* don't start if Settings->Video->TV->Enable isn't checked */
  if (!g_guiSettings.GetBool("pvrmanager.enabled"))
    return;

  ResetProperties();
  SetState(ManagerStateStarting);

  m_database->Open();

  /* create the supervisor thread to do all background activities */
  StartUpdateThreads();
}

void CPVRManager::Stop(void)
{
  /* check whether the pvrmanager is loaded */
  if (GetState() == ManagerStateStopping ||
      GetState() == ManagerStateStopped)
    return;

  SetState(ManagerStateStopping);

  /* stop the EPG updater, since it might be using the pvr add-ons */
  g_EpgContainer.Unload();

  CLog::Log(LOGNOTICE, "PVRManager - stopping");

  /* stop playback if needed */
  if (IsPlaying())
  {
    CLog::Log(LOGNOTICE,"PVRManager - %s - stopping PVR playback", __FUNCTION__);
    CApplicationMessenger::Get().MediaStop();
  }

  /* stop all update threads */
  StopUpdateThreads();

  /* executes the set wakeup command */
  SetWakeupCommand();

  /* unload all data */
  Cleanup();
}

ManagerState CPVRManager::GetState(void) const
{
  CSingleLock lock(m_managerStateMutex);
  return m_managerState;
}

void CPVRManager::SetState(ManagerState state) 
{
  CSingleLock lock(m_managerStateMutex);
  m_managerState = state;
}

void CPVRManager::Process(void)
{
  g_EpgContainer.Stop();

  /* load the pvr data from the db and clients if it's not already loaded */
  if (!Load())
  {
    CLog::Log(LOGERROR, "PVRManager - %s - failed to load PVR data", __FUNCTION__);
    return;
  }

  if (GetState() == ManagerStateStarting)
    SetState(ManagerStateStarted);
  else
    return;

  /* main loop */
  CLog::Log(LOGDEBUG, "PVRManager - %s - entering main loop", __FUNCTION__);
  g_EpgContainer.Start();

  bool bRestart(false);
  while (GetState() == ManagerStateStarted && m_addons && m_addons->HasConnectedClients() && !bRestart)
  {
    /* continue last watched channel after first startup */
    if (m_bFirstStart && g_guiSettings.GetInt("pvrplayback.startlast") != START_LAST_CHANNEL_OFF)
      ContinueLastChannel();

    /* execute the next pending jobs if there are any */
    try
    {
      ExecutePendingJobs();
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "PVRManager - %s - an error occured while trying to execute the last update job, trying to recover", __FUNCTION__);
      bRestart = true;
    }

    if (GetState() == ManagerStateStarted && !bRestart)
      m_triggerEvent.WaitMSec(1000);
  }

  if (GetState() == ManagerStateStarted)
  {
    CLog::Log(LOGNOTICE, "PVRManager - %s - no add-ons enabled anymore. restarting the pvrmanager", __FUNCTION__);
    Stop();
    Start();
    return;
  }
}

bool CPVRManager::SetWakeupCommand(void)
{
  if (!g_guiSettings.GetBool("pvrpowermanagement.enabled"))
    return false;

  const CStdString strWakeupCommand = g_guiSettings.GetString("pvrpowermanagement.setwakeupcmd", false);
  if (!strWakeupCommand.IsEmpty() && m_timers)
  {
    time_t iWakeupTime;
    const CDateTime nextEvent = m_timers->GetNextEventTime();
    nextEvent.GetAsTime(iWakeupTime);

    CStdString strExecCommand;
    strExecCommand.Format("%s %d", strWakeupCommand, iWakeupTime);

    const int iReturn = system(strExecCommand.c_str());
    if (iReturn != 0)
      CLog::Log(LOGERROR, "%s - failed to execute wakeup command '%s': %s (%d)", __FUNCTION__, strExecCommand.c_str(), strerror(iReturn), iReturn);

    return iReturn == 0;
  }

  return false;
}

bool CPVRManager::StartUpdateThreads(void)
{
  StopUpdateThreads();
  CLog::Log(LOGNOTICE, "PVRManager - starting up");

  /* create the pvrmanager thread, which will ensure that all data will be loaded */
  SetState(ManagerStateStarting);
  Create();
  SetPriority(-1);

  return true;
}

void CPVRManager::StopUpdateThreads(void)
{
  SetState(ManagerStateInterrupted);

  StopThread();
  if (m_guiInfo)
    m_guiInfo->Stop();
  if (m_addons)
    m_addons->Stop();
}

bool CPVRManager::Load(void)
{
  /* start the add-on update thread */
  m_addons->Start();

  /* load at least one client */
  while (GetState() == ManagerStateStarting && m_addons && !m_addons->HasConnectedClients())
    Sleep(50);

  if (GetState() != ManagerStateStarting || !m_addons || !m_addons->HasConnectedClients())
    return false;

  CLog::Log(LOGDEBUG, "PVRManager - %s - active clients found. continue to start", __FUNCTION__);

  CGUIWindowPVR *pWindow = (CGUIWindowPVR *) g_windowManager.GetWindow(WINDOW_PVR);
  if (pWindow)
    pWindow->Reset();

  /* load all channels and groups */
  ShowProgressDialog(g_localizeStrings.Get(19236), 0);
  if (!m_channelGroups->Load() || GetState() != ManagerStateStarting)
    return false;

  /* get timers from the backends */
  ShowProgressDialog(g_localizeStrings.Get(19237), 50);
  m_timers->Load();

  /* get recordings from the backend */
  ShowProgressDialog(g_localizeStrings.Get(19238), 75);
  m_recordings->Load();

  CSingleLock lock(m_critSection);
  if (GetState() != ManagerStateStarting)
    return false;

  /* start the other pvr related update threads */
  ShowProgressDialog(g_localizeStrings.Get(19239), 85);
  m_guiInfo->Start();

  /* close the progess dialog */
  HideProgressDialog();

  return true;
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

bool CPVRManager::ChannelSwitch(unsigned int iChannelNumber)
{
  CSingleLock lock(m_critSection);

  CPVRChannelGroupPtr playingGroup = GetPlayingGroup(m_addons->IsPlayingRadio());
  if (!playingGroup)
  {
    CLog::Log(LOGERROR, "PVRManager - %s - cannot get the selected group", __FUNCTION__);
    return false;
  }

  CFileItemPtr channel = playingGroup->GetByChannelNumber(iChannelNumber);
  if (!channel || !channel->HasPVRChannelInfoTag())
  {
    CLog::Log(LOGERROR, "PVRManager - %s - cannot find channel %d", __FUNCTION__, iChannelNumber);
    return false;
  }

  return PerformChannelSwitch(*channel->GetPVRChannelInfoTag(), false);
}

bool CPVRManager::ChannelUpDown(unsigned int *iNewChannelNumber, bool bPreview, bool bUp)
{
  bool bReturn = false;
  if (IsPlayingTV() || IsPlayingRadio())
  {
    CFileItem currentFile(g_application.CurrentFileItem());
    CPVRChannel *currentChannel = currentFile.GetPVRChannelInfoTag();
    CPVRChannelGroupPtr group = GetPlayingGroup(currentChannel->IsRadio());
    if (group)
    {
      CFileItemPtr newChannel = bUp ?
          group->GetByChannelUp(*currentChannel) :
          group->GetByChannelDown(*currentChannel);

      if (newChannel && newChannel->HasPVRChannelInfoTag() &&
          PerformChannelSwitch(*newChannel->GetPVRChannelInfoTag(), bPreview))
      {
        *iNewChannelNumber = newChannel->GetPVRChannelInfoTag()->ChannelNumber();
        bReturn = true;
      }
    }
  }

  return bReturn;
}

bool CPVRManager::ContinueLastChannel(void)
{
  {
    CSingleLock lock(m_critSection);
    if (!m_bFirstStart)
      return true;
    m_bFirstStart = false;
  }

  bool bReturn(false);
  CFileItemPtr channel = m_channelGroups->GetLastPlayedChannel();
  if (channel && channel->HasPVRChannelInfoTag())
  {
    CLog::Log(LOGNOTICE, "PVRManager - %s - continue playback on channel '%s'", __FUNCTION__, channel->GetPVRChannelInfoTag()->ChannelName().c_str());
    bReturn = StartPlayback(channel->GetPVRChannelInfoTag(), (g_guiSettings.GetInt("pvrplayback.startlast") == START_LAST_CHANNEL_MIN));
  }

  return bReturn;
}

void CPVRManager::ResetDatabase(bool bShowProgress /* = true */)
{
  CLog::Log(LOGNOTICE,"PVRManager - %s - clearing the PVR database", __FUNCTION__);

  g_EpgContainer.Stop();

  CGUIDialogProgress* pDlgProgress = NULL;
  if (bShowProgress)
  {
    pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    pDlgProgress->SetLine(0, StringUtils::EmptyString);
    pDlgProgress->SetLine(1, g_localizeStrings.Get(19186));
    pDlgProgress->SetLine(2, StringUtils::EmptyString);
    pDlgProgress->StartModal();
    pDlgProgress->Progress();
  }

  if (m_addons && m_addons->IsPlaying())
  {
    CLog::Log(LOGNOTICE,"PVRManager - %s - stopping playback", __FUNCTION__);
    CApplicationMessenger::Get().MediaStop();
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

  if (m_database && m_database->Open())
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
    m_database->Open();
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
    static_cast<CPVRChannelGroupInternal *>(m_channelGroups->GetGroupAllTV().get())->CreateChannelEpgs(true);
    static_cast<CPVRChannelGroupInternal *>(m_channelGroups->GetGroupAllRadio().get())->CreateChannelEpgs(true);
    g_EpgContainer.Start();
    StartUpdateThreads();
  }
}

bool CPVRManager::IsPlaying(void) const
{
  return IsStarted() && m_addons && m_addons->IsPlaying();
}

bool CPVRManager::GetCurrentChannel(CPVRChannel &channel) const
{
  return IsPlaying() && m_addons && m_addons->GetPlayingChannel(channel);
}

int CPVRManager::GetCurrentEpg(CFileItemList &results) const
{
  int iReturn = -1;

  CPVRChannel channel;
  if (m_addons->GetPlayingChannel(channel))
    iReturn = channel.GetEPG(results);
  else
    CLog::Log(LOGDEBUG,"PVRManager - %s - no current channel set", __FUNCTION__);

  return iReturn;
}

void CPVRManager::ResetPlayingTag(void)
{
  CSingleLock lock(m_critSection);
  if (GetState() == ManagerStateStarted && m_guiInfo)
    m_guiInfo->ResetPlayingTag();
}

int CPVRManager::GetPreviousChannel(void)
{
  //XXX this must be the craziest way to store the last channel
  int iReturn = -1;
  CPVRChannel channel;
  if (m_addons->GetPlayingChannel(channel))
  {
    int iLastChannel = channel.ChannelNumber();

    if ((m_PreviousChannel[m_PreviousChannelIndex ^ 1] == iLastChannel || iLastChannel != m_PreviousChannel[0]) &&
        iLastChannel != m_PreviousChannel[1])
      m_PreviousChannelIndex ^= 1;

    iReturn = m_PreviousChannel[m_PreviousChannelIndex ^= 1];
  }

  return iReturn;
}

bool CPVRManager::ToggleRecordingOnChannel(unsigned int iChannelId)
{
  bool bReturn = false;

  CPVRChannelPtr channel = m_channelGroups->GetChannelById(iChannelId);
  if (!channel)
    return bReturn;

  if (m_addons->HasTimerSupport(channel->ClientID()))
  {
    /* timers are supported on this channel */
    if (!channel->IsRecording())
    {
      bReturn = m_timers->InstantTimer(*channel);
      if (!bReturn)
        CGUIDialogOK::ShowAndGetInput(19033,0,19164,0);
    }
    else
    {
      /* delete active timers */
      bReturn = m_timers->DeleteTimersOnChannel(*channel, false, true);
    }
  }

  return bReturn;
}

bool CPVRManager::StartRecordingOnPlayingChannel(bool bOnOff)
{
  bool bReturn = false;

  CPVRChannel channel;
  if (!m_addons->GetPlayingChannel(channel))
    return bReturn;

  if (m_addons->HasTimerSupport(channel.ClientID()))
  {
    /* timers are supported on this channel */
    if (bOnOff && !channel.IsRecording())
    {
      bReturn = m_timers->InstantTimer(channel);
      if (!bReturn)
        CGUIDialogOK::ShowAndGetInput(19033,0,19164,0);
    }
    else if (!bOnOff && channel.IsRecording())
    {
      /* delete active timers */
      bReturn = m_timers->DeleteTimersOnChannel(channel, false, true);
    }
  }

  return bReturn;
}

bool CPVRManager::CheckParentalLock(const CPVRChannel &channel)
{
  bool bReturn(true);

  if (IsParentalLocked(channel))
  {
    // check the pin code
    if (!CheckParentalPIN())
    {
      CLog::Log(LOGERROR, "PVRManager - %s - parental lock verification failed for channel '%s': wrong PIN entered.",
          __FUNCTION__, channel.ChannelName().c_str());
      bReturn = false;
    }
    else
    {
      // reset the timer
      m_parentalTimer.StartZero();
    }
  }

  return bReturn;
}

bool CPVRManager::IsParentalLocked(const CPVRChannel &channel)
{
  bool bReturn(false);
  CPVRChannel currentChannel(NULL);

  if (// different channel
      (!GetCurrentChannel(currentChannel) || channel != currentChannel) &&
      // parental control enabled
      g_guiSettings.GetBool("pvrparental.enabled") &&
      // channel is locked
      channel.IsLocked())
  {
    float parentalDurationMs = g_guiSettings.GetInt("pvrparental.duration") * 1000.0f;
    bReturn = !m_parentalTimer.IsRunning() ||
        m_parentalTimer.GetElapsedMilliseconds() > parentalDurationMs;
  }

  return bReturn;
}

bool CPVRManager::CheckParentalPIN(const char *strTitle /* = NULL */)
{
  CStdString pinCode = g_guiSettings.GetString("pvrparental.pin");

  if (!g_guiSettings.GetBool("pvrparental.enabled") || pinCode.empty())
    return true;

  bool bValidPIN = CGUIDialogNumeric::ShowAndVerifyInput(pinCode, strTitle ? strTitle : g_localizeStrings.Get(19263).c_str(), true);
  if (!bValidPIN)
    // display message: The entered PIN number was incorrect
    CGUIDialogOK::ShowAndGetInput(19264,0,19265,0);

  return bValidPIN;
}

void CPVRManager::SaveCurrentChannelSettings(void)
{
  m_addons->SaveCurrentChannelSettings();
}

void CPVRManager::LoadCurrentChannelSettings()
{
  m_addons->LoadCurrentChannelSettings();
}

void CPVRManager::SetPlayingGroup(CPVRChannelGroupPtr group)
{
  m_channelGroups->Get(group->IsRadio())->SetSelectedGroup(group);
}

CPVRChannelGroupPtr CPVRManager::GetPlayingGroup(bool bRadio /* = false */)
{
  return m_channelGroups->GetSelectedGroup(bRadio);
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

bool CPVRManager::OpenLiveStream(const CFileItem &channel)
{
  bool bReturn(false);
  if (!channel.HasPVRChannelInfoTag())
    return bReturn;

  CLog::Log(LOGDEBUG,"PVRManager - %s - opening live stream on channel '%s'",
      __FUNCTION__, channel.GetPVRChannelInfoTag()->ChannelName().c_str());

  // check if we're allowed to play this file
  if (!CheckParentalLock(*channel.GetPVRChannelInfoTag()))
    return bReturn;

  if ((bReturn = m_addons->OpenLiveStream(*channel.GetPVRChannelInfoTag())) != false)
  {
    CSingleLock lock(m_critSection);
    if(m_currentFile)
      delete m_currentFile;
    m_currentFile = new CFileItem(channel);
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
  CPVRChannel channel;
  bool bPersistChannel(false);

  {
    CSingleLock lock(m_critSection);

    if (m_addons->IsReadingLiveStream())
    {
      CPVRChannel channel;
      if (m_addons->GetPlayingChannel(channel))
      {
        /* store current time in iLastWatched */
        time_t tNow;
        CDateTime::GetCurrentDateTime().GetAsTime(tNow);
        channel.SetLastWatched(tNow);
        bPersistChannel = true;
      }
    }

    m_addons->CloseStream();
    SAFE_DELETE(m_currentFile);
  }

  if (bPersistChannel)
    channel.Persist();
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
    return false;

  if (!item.IsPVRChannel())
  {
    CLog::Log(LOGERROR, "CPVRManager - %s - no channel tag provided", __FUNCTION__);
    return false;
  }

  CSingleLock lock(m_critSection);
  if (!m_currentFile || *m_currentFile->GetPVRChannelInfoTag() == *item.GetPVRChannelInfoTag())
    return false;

  g_application.CurrentFileItem() = *m_currentFile;
  g_infoManager.SetCurrentItem(*m_currentFile);

  CPVRChannel* channelTag = item.GetPVRChannelInfoTag();
  CEpgInfoTag epgTagNow;
  bool bHasTagNow = channelTag->GetEPGNow(epgTagNow);

  if (channelTag->IsRadio())
  {
    CMusicInfoTag* musictag = item.GetMusicInfoTag();
    if (musictag)
    {
      musictag->SetTitle(bHasTagNow ?
          epgTagNow.Title() :
          g_guiSettings.GetBool("epg.hidenoinfoavailable") ?
              StringUtils::EmptyString :
              g_localizeStrings.Get(19055)); // no information available
      if (bHasTagNow)
        musictag->SetGenre(epgTagNow.Genre());
      musictag->SetDuration(bHasTagNow ? epgTagNow.GetDuration() : 3600);
      musictag->SetURL(channelTag->Path());
      musictag->SetArtist(channelTag->ChannelName());
      musictag->SetAlbumArtist(channelTag->ChannelName());
      musictag->SetLoaded(true);
      musictag->SetComment(StringUtils::EmptyString);
      musictag->SetLyrics(StringUtils::EmptyString);
    }
  }
  else
  {
    CVideoInfoTag *videotag = item.GetVideoInfoTag();
    if (videotag)
    {
      videotag->m_strTitle = bHasTagNow ?
          epgTagNow.Title() :
          g_guiSettings.GetBool("epg.hidenoinfoavailable") ?
              StringUtils::EmptyString :
              g_localizeStrings.Get(19055); // no information available
      if (bHasTagNow)
        videotag->m_genre = epgTagNow.Genre();
      videotag->m_strPath = channelTag->Path();
      videotag->m_strFileNameAndPath = channelTag->Path();
      videotag->m_strPlot = bHasTagNow ? epgTagNow.Plot() : StringUtils::EmptyString;
      videotag->m_strPlotOutline = bHasTagNow ? epgTagNow.PlotOutline() : StringUtils::EmptyString;
      videotag->m_iEpisode = bHasTagNow ? epgTagNow.EpisodeNum() : 0;
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
  else
    m_LastChannelChanged = XbmcThreads::SystemClockMillis();

  return false;
}


bool CPVRManager::UpdateCurrentLastPlayedPosition(int lastplayedposition)
{
  // Only anything but recordings we fake success
  if (!IsPlayingRecording())
    return true;

  bool rc = false;
  CPVRRecording currentRecording;

  if (m_addons)
  {
    PVR_ERROR error;
    rc = m_addons->GetPlayingRecording(currentRecording) && m_addons->SetRecordingLastPlayedPosition(currentRecording, lastplayedposition, &error);
  }
  return rc;
}

bool CPVRManager::SetRecordingLastPlayedPosition(const CPVRRecording &recording, int lastplayedposition)
{
  bool rc = false;

  if (m_addons)
  {
    PVR_ERROR error;
    rc = m_addons->SetRecordingLastPlayedPosition(recording, lastplayedposition, &error);
  }
  return rc;
}

int CPVRManager::GetRecordingLastPlayedPosition(const CPVRRecording &recording)
{
  int rc = 0;

  if (m_addons)
  {
    rc = m_addons->GetRecordingLastPlayedPosition(recording);
  }
  return rc;
}

bool CPVRManager::StartPlayback(const CPVRChannel *channel, bool bPreview /* = false */)
{
  g_settings.m_bStartVideoWindowed = bPreview;
  CApplicationMessenger::Get().MediaPlay(CFileItem(*channel));
  CLog::Log(LOGNOTICE, "PVRManager - %s - started playback on channel '%s'",
      __FUNCTION__, channel->ChannelName().c_str());
  return true;
}

bool CPVRManager::PerformChannelSwitch(const CPVRChannel &channel, bool bPreview)
{
  bool bSwitched(false);

  if (!CheckParentalLock(channel))
    return false;

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

  SAFE_DELETE(m_currentFile);

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
  return IsStarted() && m_guiInfo ? m_guiInfo->GetDuration() : 0;
}

int CPVRManager::GetStartTime(void) const
{
  return IsStarted() && m_guiInfo ? m_guiInfo->GetStartTime() : 0;
}

bool CPVRManager::TranslateBoolInfo(DWORD dwInfo) const
{
   return IsStarted() && m_guiInfo ? m_guiInfo->TranslateBoolInfo(dwInfo) : false;
}

bool CPVRManager::TranslateCharInfo(DWORD dwInfo, CStdString &strValue) const
{
  return IsStarted() && m_guiInfo ? m_guiInfo->TranslateCharInfo(dwInfo, strValue) : false;
}

int CPVRManager::TranslateIntInfo(DWORD dwInfo) const
{
  return IsStarted() && m_guiInfo ? m_guiInfo->TranslateIntInfo(dwInfo) : 0;
}

bool CPVRManager::HasTimers(void) const
{
  return IsStarted() && m_timers ? m_timers->HasActiveTimers() : false;
}

bool CPVRManager::IsRecording(void) const
{
  return IsStarted() && m_timers ? m_timers->IsRecording() : false;
}

bool CPVRManager::IsIdle(void) const
{
  if (!IsStarted())
    return true;

  if (IsRecording() || IsPlaying()) // pvr recording or playing?
  {
    return false;
  }
  else if (m_timers) // has active timers, etc.?
  {
    const CDateTime now = CDateTime::GetUTCDateTime();
    const CDateTimeSpan idle(0, 0, g_guiSettings.GetInt("pvrpowermanagement.backendidletime"), 0);

    const CDateTime next = m_timers->GetNextEventTime();
    const CDateTimeSpan delta = next - now;

    if (delta < idle)
    {
      return false;
    }
  }

  return true;
}

void CPVRManager::ShowPlayerInfo(int iTimeout)
{
  if (IsStarted() && m_guiInfo)
    m_guiInfo->ShowPlayerInfo(iTimeout);
}

void CPVRManager::LocalizationChanged(void)
{
  CSingleLock lock(m_critSection);
  if (IsStarted())
  {
    static_cast<CPVRChannelGroupInternal *>(m_channelGroups->GetGroupAllRadio().get())->CheckGroupName();
    static_cast<CPVRChannelGroupInternal *>(m_channelGroups->GetGroupAllTV().get())->CheckGroupName();
  }
}

bool CPVRManager::IsInitialising(void) const
{
  return GetState() == ManagerStateStarting;
}

bool CPVRManager::IsStarted(void) const
{
  return GetState() == ManagerStateStarted;
}

bool CPVRManager::IsPlayingTV(void) const
{
  return IsStarted() && m_addons && m_addons->IsPlayingTV();
}

bool CPVRManager::IsPlayingRadio(void) const
{
  return IsStarted() && m_addons && m_addons->IsPlayingRadio();
}

bool CPVRManager::IsPlayingRecording(void) const
{
  return IsStarted() && m_addons && m_addons->IsPlayingRecording();
}

bool CPVRManager::IsRunningChannelScan(void) const
{
  return IsStarted() && m_addons && m_addons->IsRunningChannelScan();
}

PVR_ADDON_CAPABILITIES CPVRManager::GetCurrentAddonCapabilities(void)
{
  PVR_ADDON_CAPABILITIES props;
  memset(&props, 0, sizeof(PVR_ADDON_CAPABILITIES));

  if (IsStarted() && m_addons)
    props = m_addons->GetCurrentAddonCapabilities();

  return props;
}

void CPVRManager::StartChannelScan(void)
{
  if (IsStarted() && m_addons)
    m_addons->StartChannelScan();
}

void CPVRManager::SearchMissingChannelIcons(void)
{
  if (IsStarted() && m_channelGroups)
    m_channelGroups->SearchMissingChannelIcons();
}

bool CPVRManager::IsJobPending(const char *strJobName) const
{
  bool bReturn(false);
  CSingleLock lock(m_critSectionTriggers);
  for (unsigned int iJobPtr = 0; IsStarted() && iJobPtr < m_pendingUpdates.size(); iJobPtr++)
  {
    if (!strcmp(m_pendingUpdates.at(iJobPtr)->GetType(), strJobName))
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
  if (!IsStarted() || IsJobPending("pvr-update-recordings"))
    return;

  m_pendingUpdates.push_back(new CPVRRecordingsUpdateJob());

  lock.Leave();
  m_triggerEvent.Set();
}

void CPVRManager::TriggerTimersUpdate(void)
{
  CSingleLock lock(m_critSectionTriggers);
  if (!IsStarted() || IsJobPending("pvr-update-timers"))
    return;

  m_pendingUpdates.push_back(new CPVRTimersUpdateJob());

  lock.Leave();
  m_triggerEvent.Set();
}

void CPVRManager::TriggerChannelsUpdate(void)
{
  CSingleLock lock(m_critSectionTriggers);
  if (!IsStarted() || IsJobPending("pvr-update-channels"))
    return;

  m_pendingUpdates.push_back(new CPVRChannelsUpdateJob());

  lock.Leave();
  m_triggerEvent.Set();
}

void CPVRManager::TriggerChannelGroupsUpdate(void)
{
  CSingleLock lock(m_critSectionTriggers);
  if (!IsStarted() || IsJobPending("pvr-update-channelgroups"))
    return;

  m_pendingUpdates.push_back(new CPVRChannelGroupsUpdateJob());

  lock.Leave();
  m_triggerEvent.Set();
}

void CPVRManager::TriggerSaveChannelSettings(void)
{
  CSingleLock lock(m_critSectionTriggers);
  if (!IsStarted() || IsJobPending("pvr-save-channelsettings"))
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

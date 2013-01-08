/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
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
#include "utils/Stopwatch.h"
#include "utils/StringUtils.h"
#include "threads/Atomics.h"
#include "windows/GUIWindowPVRCommon.h"
#include "utils/JobManager.h"

#include "PVRManager.h"
#include "PVRDatabase.h"
#include "PVRGUIInfo.h"
#include "addons/PVRClients.h"
#include "channels/PVRChannel.h"
#include "channels/PVRChannelGroupsContainer.h"
#include "channels/PVRChannelGroupInternal.h"
#include "epg/EpgContainer.h"
#include "recordings/PVRRecordings.h"
#include "timers/PVRTimers.h"
#include "interfaces/AnnouncementManager.h"
#include "addons/AddonInstaller.h"

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
    m_progressHandle(NULL),
    m_managerState(ManagerStateStopped),
    m_bOpenPVRWindow(false)
{
  ResetProperties();
}

CPVRManager::~CPVRManager(void)
{
  Stop();
  CLog::Log(LOGDEBUG,"PVRManager - destroyed");
}

CPVRManager &CPVRManager::Get(void)
{
  static CPVRManager pvrManagerInstance;
  return pvrManagerInstance;
}

bool CPVRManager::IsPVRWindowActive(void) const
{
  return g_windowManager.IsWindowActive(WINDOW_PVR) ||
      g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_CHANNEL_MANAGER) ||
      g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_OSD_CHANNELS) ||
      g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_GROUP_MANAGER) ||
      g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_GUIDE_INFO) ||
      g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_OSD_CUTTER) ||
      g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_OSD_DIRECTOR) ||
      g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_OSD_GUIDE) ||
      g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_GUIDE_SEARCH) ||
      g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_RECORDING_INFO) ||
      g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_TIMER_SETTING);
}

bool CPVRManager::InstallAddonAllowed(const std::string& strAddonId) const
{
  return !IsStarted() ||
      !m_addons->IsInUse(strAddonId) ||
      (!IsPVRWindowActive() && !IsPlaying());
}

void CPVRManager::MarkAsOutdated(const std::string& strAddonId, const std::string& strReferer)
{
  if (IsStarted() && g_settings.m_bAddonAutoUpdate)
  {
    CSingleLock lock(m_critSection);
    m_outdatedAddons.insert(make_pair<string, string>(strAddonId, strReferer));
  }
}

bool CPVRManager::UpgradeOutdatedAddons(void)
{
  CSingleLock lock(m_critSection);
  if (m_outdatedAddons.empty())
    return true;

  // there's add-ons that couldn't be updated
  for (map<string, string>::iterator it = m_outdatedAddons.begin(); it != m_outdatedAddons.end(); it++)
  {
    if (!InstallAddonAllowed(it->first))
    {
      // we can't upgrade right now
      return true;
    }
  }

  // all outdated add-ons can be upgraded now
  CLog::Log(LOGINFO, "PVR - upgrading outdated add-ons");

  map<string, string> outdatedAddons = m_outdatedAddons;
  // stop threads and unload
  SetState(ManagerStateInterrupted);
  g_EpgContainer.Stop();
  m_guiInfo->Stop();
  m_addons->Stop();
  Cleanup();

  // upgrade all add-ons
  for (map<string, string>::iterator it = outdatedAddons.begin(); it != outdatedAddons.end(); it++)
  {
    CLog::Log(LOGINFO, "PVR - updating add-on '%s'", it->first.c_str());
    CAddonInstaller::Get().Install(it->first, true, it->second, false);
  }

  // reload
  CLog::Log(LOGINFO, "PVRManager - %s - restarting the PVR manager", __FUNCTION__);
  SetState(ManagerStateStarting);
  ResetProperties();

  while (!Load() && GetState() == ManagerStateStarting)
  {
    CLog::Log(LOGERROR, "PVRManager - %s - failed to load PVR data, retrying", __FUNCTION__);
    if (m_guiInfo) m_guiInfo->Stop();
    if (m_addons) m_addons->Stop();
    Cleanup();
    Sleep(1000);
  }

  if (GetState() == ManagerStateStarting)
  {
    SetState(ManagerStateStarted);
    g_EpgContainer.Start();

    CLog::Log(LOGDEBUG, "PVRManager - %s - restarted", __FUNCTION__);
    return true;
  }

  return false;
}

bool CPVRManager::WaitUntilInitialised(void)
{
  return m_initialisedEvent.Wait() &&
      IsStarted();
}

void CPVRManager::Cleanup(void)
{
  CSingleLock lock(m_critSection);

  SAFE_DELETE(m_addons);
  SAFE_DELETE(m_guiInfo);
  SAFE_DELETE(m_timers);
  SAFE_DELETE(m_recordings);
  SAFE_DELETE(m_channelGroups);
  SAFE_DELETE(m_parentalTimer);
  SAFE_DELETE(m_database);
  m_triggerEvent.Set();

  m_currentFile           = NULL;
  m_bIsSwitchingChannels  = false;
  m_outdatedAddons.clear();
  m_bOpenPVRWindow = false;

  for (unsigned int iJobPtr = 0; iJobPtr < m_pendingUpdates.size(); iJobPtr++)
    delete m_pendingUpdates.at(iJobPtr);
  m_pendingUpdates.clear();

  m_initialisedEvent.Reset();
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
    m_parentalTimer = new CStopWatch;
  }
}

class CPVRManagerStartJob : public CJob
{
public:
  CPVRManagerStartJob(bool bOpenPVRWindow = false) :
    m_bOpenPVRWindow(bOpenPVRWindow) {}
  ~CPVRManagerStartJob(void) {}

  bool DoWork(void)
  {
    g_PVRManager.Start(false, m_bOpenPVRWindow);
    return true;
  }
private:
  bool m_bOpenPVRWindow;
};

void CPVRManager::Start(bool bAsync /* = false */, bool bOpenPVRWindow /* = false */)
{
  if (bAsync)
  {
    CPVRManagerStartJob *job = new CPVRManagerStartJob(bOpenPVRWindow);
    CJobManager::GetInstance().AddJob(job, NULL);
    return;
  }

  CSingleLock lock(m_critSection);

  /* first stop and remove any clients */
  Stop();

  /* don't start if Settings->Video->TV->Enable isn't checked */
  if (!g_guiSettings.GetBool("pvrmanager.enabled"))
    return;

  ResetProperties();
  SetState(ManagerStateStarting);
  m_bOpenPVRWindow = bOpenPVRWindow;

  /* create and open database */
  if (!m_database)
    m_database = new CPVRDatabase;
  m_database->Open();

  g_EpgContainer.Start();

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
  m_initialisedEvent.Set();
  g_EpgContainer.Stop();

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

  /* close database */
  if (m_database->IsOpen())
    m_database->Close();

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
  /* load the pvr data from the db and clients if it's not already loaded */
  while (!Load() && GetState() == ManagerStateStarting)
  {
    CLog::Log(LOGERROR, "PVRManager - %s - failed to load PVR data, retrying", __FUNCTION__);
    if (m_guiInfo) m_guiInfo->Stop();
    if (m_addons) m_addons->Stop();
    Cleanup();
    Sleep(1000);
  }

  if (GetState() == ManagerStateStarting)
    SetState(ManagerStateStarted);
  else
    return;

  /* main loop */
  CLog::Log(LOGDEBUG, "PVRManager - %s - entering main loop", __FUNCTION__);
  m_initialisedEvent.Set();

  if (m_bOpenPVRWindow)
  {
    m_bOpenPVRWindow = false;
    CApplicationMessenger::Get().ExecBuiltIn("XBMC.ActivateWindowAndFocus(MyPVR, 32,0, 11,0)");
  }

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

    if (!UpgradeOutdatedAddons())
    {
      // failed to load after upgrading
      CLog::Log(LOGERROR, "PVRManager - %s - could not load pvr data after upgrading. stopping the pvrmanager", __FUNCTION__);
    }
    else if (GetState() == ManagerStateStarted && !bRestart)
      m_triggerEvent.WaitMSec(1000);
  }

  if (GetState() == ManagerStateStarted)
  {
    CLog::Log(LOGNOTICE, "PVRManager - %s - no add-ons enabled anymore. restarting the pvrmanager", __FUNCTION__);
    CApplicationMessenger::Get().ExecBuiltIn("StartPVRManager", false);
  }
  else
  {
    if (g_windowManager.GetActiveWindow() == WINDOW_PVR)
      g_windowManager.ActivateWindow(WINDOW_HOME);
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
    if (nextEvent.IsValid())
    {
      nextEvent.GetAsTime(iWakeupTime);
        
      CStdString strExecCommand;
      strExecCommand.Format("%s %d", strWakeupCommand, iWakeupTime);
        
      const int iReturn = system(strExecCommand.c_str());
      if (iReturn != 0)
        CLog::Log(LOGERROR, "%s - failed to execute wakeup command '%s': %s (%d)", __FUNCTION__, strExecCommand.c_str(), strerror(iReturn), iReturn);
        
      return iReturn == 0;
    }
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
  if (m_addons)
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
  ShowProgressDialog(g_localizeStrings.Get(19236), 0); // Loading channels from clients
  if (!m_channelGroups->Load() || GetState() != ManagerStateStarting)
    return false;

  /* get timers from the backends */
  ShowProgressDialog(g_localizeStrings.Get(19237), 50); // Loading timers from clients
  m_timers->Load();

  /* get recordings from the backend */
  ShowProgressDialog(g_localizeStrings.Get(19238), 75); // Loading recordings from clients
  m_recordings->Load();

  if (GetState() != ManagerStateStarting)
    return false;

  /* start the other pvr related update threads */
  ShowProgressDialog(g_localizeStrings.Get(19239), 85); // Starting background threads
  m_guiInfo->Start();

  /* close the progess dialog */
  HideProgressDialog();

  return true;
}

void CPVRManager::ShowProgressDialog(const CStdString &strText, int iProgress)
{
  if (!m_progressHandle)
  {
    CGUIDialogExtendedProgressBar *loadingProgressDialog = (CGUIDialogExtendedProgressBar *)g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS);
    m_progressHandle = loadingProgressDialog->GetHandle(g_localizeStrings.Get(19235)); // PVR manager is starting up
  }

  m_progressHandle->SetPercentage((float)iProgress);
  m_progressHandle->SetText(strText);
}

void CPVRManager::HideProgressDialog(void)
{
  if (m_progressHandle)
  {
    m_progressHandle->MarkFinished();
    m_progressHandle = NULL;
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

void CPVRManager::ResetDatabase(bool bResetEPGOnly /* = false */)
{
  CLog::Log(LOGNOTICE,"PVRManager - %s - clearing the PVR database", __FUNCTION__);

  g_EpgContainer.Stop();

  CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  pDlgProgress->SetLine(0, StringUtils::EmptyString);
  pDlgProgress->SetLine(1, g_localizeStrings.Get(19186)); // All data in the PVR database is being erased
  pDlgProgress->SetLine(2, StringUtils::EmptyString);
  pDlgProgress->StartModal();
  pDlgProgress->Progress();

  if (m_addons && m_addons->IsPlaying())
  {
    CLog::Log(LOGNOTICE,"PVRManager - %s - stopping playback", __FUNCTION__);
    CApplicationMessenger::Get().MediaStop();
  }

  pDlgProgress->SetPercentage(10);
  pDlgProgress->Progress();

  /* reset the EPG pointers */
  m_database->ResetEPG();

  /* stop the thread */
  Stop();

  pDlgProgress->SetPercentage(20);
  pDlgProgress->Progress();

  if (!m_database)
    m_database = new CPVRDatabase;

  if (m_database && m_database->Open())
  {
    /* clean the EPG database */
    g_EpgContainer.Reset();
    pDlgProgress->SetPercentage(30);
    pDlgProgress->Progress();

    if (!bResetEPGOnly)
    {
      m_database->DeleteChannelGroups();
      pDlgProgress->SetPercentage(50);
      pDlgProgress->Progress();

      /* delete all channels */
      m_database->DeleteChannels();
      pDlgProgress->SetPercentage(70);
      pDlgProgress->Progress();

      /* delete all channel settings */
      m_database->DeleteChannelSettings();
      pDlgProgress->SetPercentage(80);
      pDlgProgress->Progress();

      /* delete all client information */
      m_database->DeleteClients();
      pDlgProgress->SetPercentage(90);
      pDlgProgress->Progress();
    }

    m_database->Close();
  }

  CLog::Log(LOGNOTICE,"PVRManager - %s - %s database cleared", __FUNCTION__, bResetEPGOnly ? "EPG" : "PVR and EPG");

  if (g_guiSettings.GetBool("pvrmanager.enabled"))
  {
    CLog::Log(LOGNOTICE,"PVRManager - %s - restarting the PVRManager", __FUNCTION__);
    m_database->Open();
    Cleanup();
    Start();
  }

  pDlgProgress->SetPercentage(100);
  pDlgProgress->Close();
}

bool CPVRManager::IsPlaying(void) const
{
  return IsStarted() && m_addons && m_addons->IsPlaying();
}

bool CPVRManager::GetCurrentChannel(CPVRChannelPtr &channel) const
{
  return m_addons && m_addons->GetPlayingChannel(channel);
}

int CPVRManager::GetCurrentEpg(CFileItemList &results) const
{
  int iReturn = -1;

  CPVRChannelPtr channel;
  if (m_addons->GetPlayingChannel(channel))
    iReturn = channel->GetEPG(results);
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
  CPVRChannelPtr currentChannel;
  if (GetCurrentChannel(currentChannel))
  {
    CPVRChannelGroupPtr selectedGroup = GetPlayingGroup(currentChannel->IsRadio());
    CFileItemPtr channel = selectedGroup->GetLastPlayedChannel(currentChannel->ChannelID());
    if (channel && channel->HasPVRChannelInfoTag())
      return channel->GetPVRChannelInfoTag()->ChannelNumber();
  }
  return -1;
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

  CPVRChannelPtr channel;
  if (!m_addons->GetPlayingChannel(channel))
    return bReturn;

  if (m_addons->HasTimerSupport(channel->ClientID()))
  {
    /* timers are supported on this channel */
    if (bOnOff && !channel->IsRecording())
    {
      bReturn = m_timers->InstantTimer(*channel);
      if (!bReturn)
        CGUIDialogOK::ShowAndGetInput(19033,0,19164,0);
    }
    else if (!bOnOff && channel->IsRecording())
    {
      /* delete active timers */
      bReturn = m_timers->DeleteTimersOnChannel(*channel, true, true);
    }
  }

  return bReturn;
}

bool CPVRManager::CheckParentalLock(const CPVRChannel &channel)
{
  bool bReturn = !IsParentalLocked(channel) ||
      CheckParentalPIN();

  if (!bReturn)
    CLog::Log(LOGERROR, "PVRManager - %s - parental lock verification failed for channel '%s': wrong PIN entered.", __FUNCTION__, channel.ChannelName().c_str());

  return bReturn;
}

bool CPVRManager::IsParentalLocked(const CPVRChannel &channel)
{
  bool bReturn(false);
  CSingleLock lock(m_managerStateMutex);
  if (!IsStarted())
    return bReturn;
  CPVRChannelPtr currentChannel(new CPVRChannel(false));

  if (// different channel
      (!GetCurrentChannel(currentChannel) || channel != *currentChannel) &&
      // parental control enabled
      g_guiSettings.GetBool("pvrparental.enabled") &&
      // channel is locked
      channel.IsLocked())
  {
    float parentalDurationMs = g_guiSettings.GetInt("pvrparental.duration") * 1000.0f;
    bReturn = m_parentalTimer &&
        (!m_parentalTimer->IsRunning() ||
          m_parentalTimer->GetElapsedMilliseconds() > parentalDurationMs);
  }

  return bReturn;
}

bool CPVRManager::CheckParentalPIN(const char *strTitle /* = NULL */)
{
  CStdString pinCode = g_guiSettings.GetString("pvrparental.pin");

  if (!g_guiSettings.GetBool("pvrparental.enabled") || pinCode.empty())
    return true;

  // Locked channel. Enter PIN:
  bool bValidPIN = CGUIDialogNumeric::ShowAndVerifyInput(pinCode, strTitle ? strTitle : g_localizeStrings.Get(19263).c_str(), true);
  if (!bValidPIN)
    // display message: The entered PIN number was incorrect
    CGUIDialogOK::ShowAndGetInput(19264,0,19265,0);
  else if (m_parentalTimer)
  {
    // reset the timer
    m_parentalTimer->StartZero();
  }

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
  if (m_channelGroups && group)
    m_channelGroups->Get(group->IsRadio())->SetSelectedGroup(group);
}

CPVRChannelGroupPtr CPVRManager::GetPlayingGroup(bool bRadio /* = false */)
{
  if (m_channelGroups)
    return m_channelGroups->GetSelectedGroup(bRadio);

  return CPVRChannelGroupPtr();
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
  if (IsParentalLocked(*channel.GetPVRChannelInfoTag()))
    return bReturn;

  CPVRChannelPtr playingChannel;
  bool bPersistChannel(false);
  if ((bReturn = m_addons->OpenStream(*channel.GetPVRChannelInfoTag(), false)) != false)
  {
    CSingleLock lock(m_critSection);
    if(m_currentFile)
      delete m_currentFile;
    m_currentFile = new CFileItem(channel);

    if (m_addons->GetPlayingChannel(playingChannel))
    {
      /* store current time in iLastWatched */
      time_t tNow;
      CDateTime::GetCurrentDateTime().GetAsTime(tNow);
      playingChannel->SetLastWatched(tNow);
      bPersistChannel = true;
    }
  }

  if (bPersistChannel)
    playingChannel->Persist();

  return bReturn;
}

bool CPVRManager::OpenRecordedStream(const CPVRRecording &tag)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  CLog::Log(LOGDEBUG,"PVRManager - %s - opening recorded stream '%s'",
      __FUNCTION__, tag.m_strFile.c_str());

  if ((bReturn = m_addons->OpenStream(tag)) != false)
  {
    delete m_currentFile;
    m_currentFile = new CFileItem(tag);
  }

  return bReturn;
}

void CPVRManager::CloseStream(void)
{
  CPVRChannelPtr channel;
  bool bPersistChannel(false);

  {
    CSingleLock lock(m_critSection);

    if (m_addons->GetPlayingChannel(channel))
    {
      /* store current time in iLastWatched */
      time_t tNow;
      CDateTime::GetCurrentDateTime().GetAsTime(tNow);
      channel->SetLastWatched(tNow);
      bPersistChannel = true;
    }

    m_addons->CloseStream();
    SAFE_DELETE(m_currentFile);
  }

  if (bPersistChannel)
    channel->Persist();
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

  return false;
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
  // check parental lock state
  if (IsParentalLocked(channel))
    return false;

  // invalid channel
  if (channel.ClientID() < 0)
    return false;

  // check whether we're waiting for a previous switch to complete
  {
    CSingleLock lock(m_critSection);
    if (m_bIsSwitchingChannels)
    {
      CLog::Log(LOGDEBUG, "PVRManager - %s - can't switch to channel '%s'. waiting for the previous switch to complete",
          __FUNCTION__, channel.ChannelName().c_str());
      return false;
    }

    // no need to do anything except switching m_currentFile
    if (bPreview)
    {
      delete m_currentFile;
      m_currentFile = new CFileItem(channel);
      return true;
    }

    m_bIsSwitchingChannels = true;
  }

  CLog::Log(LOGDEBUG, "PVRManager - %s - switching to channel '%s'", __FUNCTION__, channel.ChannelName().c_str());

  // store current time in iLastWatched
  CPVRChannelPtr currentChannel;
  if (m_addons->GetPlayingChannel(currentChannel))
  {
    time_t tNow;
    CDateTime::GetCurrentDateTime().GetAsTime(tNow);
    currentChannel->SetLastWatched(tNow);
  }

  // store channel settings
  SaveCurrentChannelSettings();

  // will be deleted by CPVRChannelSwitchJob::DoWork()
  CFileItem* previousFile = m_currentFile;
  m_currentFile = NULL;

  bool bSwitched(false);

  // switch channel
  if (!m_addons->SwitchChannel(channel))
  {
    // switch failed
    CSingleLock lock(m_critSection);
    m_bIsSwitchingChannels = false;

    CLog::Log(LOGERROR, "PVRManager - %s - failed to switch to channel '%s'", __FUNCTION__, channel.ChannelName().c_str());

    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error,
        g_localizeStrings.Get(19166), // PVR information
        g_localizeStrings.Get(19035)); // This channel cannot be played. Check the log for details.
  }
  else
  {
    // switch successful
    bSwitched = true;

    CSingleLock lock(m_critSection);
    m_currentFile = new CFileItem(channel);
    m_bIsSwitchingChannels = false;

    CLog::Log(LOGNOTICE, "PVRManager - %s - switched to channel '%s'", __FUNCTION__, channel.ChannelName().c_str());
  }

  // announce OnStop and OnPlay. yes, this ain't pretty
  {
    CSingleLock lock(m_critSectionTriggers);
    m_pendingUpdates.push_back(new CPVRChannelSwitchJob(previousFile, m_currentFile));
  }
  m_triggerEvent.Set();

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

bool CPVRChannelSwitchJob::DoWork(void)
{
  // announce OnStop and delete m_previous when done
  if (m_previous)
  {
    CVariant data(CVariant::VariantTypeObject);
    data["end"] = true;
    ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::Player, "xbmc", "OnStop", CFileItemPtr(m_previous), data);
  }

  // announce OnPlay if the switch was successful
  if (m_next)
  {
    CVariant param;
    param["player"]["speed"] = 1;
    param["player"]["playerid"] = g_playlistPlayer.GetCurrentPlaylist();
    ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::Player, "xbmc", "OnPlay", CFileItemPtr(new CFileItem(*m_next)), param);
  }

  return true;
}

/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "Application.h"
#include "ApplicationMessenger.h"
#include "GUIInfoManager.h"
#include "Util.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/MediaSettings.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "windows/GUIWindowPVRBase.h"
#include "utils/log.h"
#include "utils/Stopwatch.h"
#include "utils/StringUtils.h"
#include "utils/JobManager.h"
#include "interfaces/AnnouncementManager.h"
#include "video/VideoDatabase.h"
#include "network/Network.h"

#include "PVRManager.h"
#include "PVRDatabase.h"
#include "PVRGUIInfo.h"
#include "PVRActionListener.h"
#include "addons/PVRClients.h"
#include "channels/PVRChannel.h"
#include "channels/PVRChannelGroupsContainer.h"
#include "channels/PVRChannelGroupInternal.h"
#include "epg/EpgContainer.h"
#include "recordings/PVRRecordings.h"
#include "timers/PVRTimers.h"
#include "addons/AddonInstaller.h"
#include "dialogs/GUIDialogPVRChannelManager.h"
#include "dialogs/GUIDialogPVRGroupManager.h"

#include <assert.h>

using namespace MUSIC_INFO;
using namespace PVR;
using namespace EPG;
using namespace ANNOUNCEMENT;

int CPVRManager::m_pvrWindowIds[10] = {
    WINDOW_TV_CHANNELS,
    WINDOW_TV_GUIDE,
    WINDOW_TV_RECORDINGS,
    WINDOW_TV_SEARCH,
    WINDOW_TV_TIMERS,
    WINDOW_RADIO_CHANNELS,
    WINDOW_RADIO_GUIDE,
    WINDOW_RADIO_RECORDINGS,
    WINDOW_RADIO_SEARCH,
    WINDOW_RADIO_TIMERS
};

CPVRManager::CPVRManager(void) :
    CThread("PVRManager"),
    m_channelGroups(NULL),
    m_recordings(NULL),
    m_timers(NULL),
    m_addons(NULL),
    m_guiInfo(NULL),
    m_triggerEvent(true),
    m_currentFile(NULL),
    m_database(NULL),
    m_bFirstStart(true),
    m_bEpgsCreated(false),
    m_progressHandle(NULL),
    m_managerState(ManagerStateStopped),
    m_openWindowId(0)
{
  CAnnouncementManager::Get().AddAnnouncer(this);
  ResetProperties();
}

CPVRManager::~CPVRManager(void)
{
  CAnnouncementManager::Get().RemoveAnnouncer(this);
  Stop();
  CLog::Log(LOGDEBUG,"PVRManager - destroyed");
}

void CPVRManager::Announce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (!IsStarted() || (flag & (System)) == 0)
   return;

  if (strcmp(message, "OnWake") == 0)
  {
    /* start job to search for missing channel icons */
    TriggerSearchMissingChannelIcons();

    /* continue last watched channel */
    ContinueLastChannel();

    /* trigger PVR data updates */
    TriggerChannelGroupsUpdate();
    TriggerChannelsUpdate();
    TriggerRecordingsUpdate();
    TriggerEpgsCreate();
    TriggerTimersUpdate();
  }
}

CPVRManager &CPVRManager::Get(void)
{
  static CPVRManager pvrManagerInstance;
  return pvrManagerInstance;
}

bool CPVRManager::RestartManagerOnAddonDisabled(void) const
{
  ManagerState ms(GetState());
  if (ms == ManagerStateStarting)
    return false;
  else if (ms == ManagerStateStarted)
    return m_addons->RestartManagerOnAddonDisabled();
  return true;
}

void CPVRManager::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == "pvrmanager.enabled")
  {
    CApplicationMessenger::Get().SetPVRManagerState(((CSettingBool*)setting)->GetValue());
  }
  else if (settingId == "pvrparental.enabled")
  {
    if (((CSettingBool*)setting)->GetValue() && CSettings::Get().GetString("pvrparental.pin").empty())
    {
      std::string newPassword = "";
      // password set... save it
      if (CGUIDialogNumeric::ShowAndVerifyNewPassword(newPassword))
        CSettings::Get().SetString("pvrparental.pin", newPassword);
      // password not set... disable parental
      else
        ((CSettingBool*)setting)->SetValue(false);
    }
  }
}

void CPVRManager::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == "pvrmenu.searchicons")
  {
    if (IsStarted())
      TriggerSearchMissingChannelIcons();
  }
  else if (settingId == "pvrmanager.resetdb")
  {
    if (CheckParentalPIN(g_localizeStrings.Get(19262)) &&
        CGUIDialogYesNo::ShowAndGetInput(19098, 19186))
    {
      CDateTime::ResetTimezoneBias();
      ResetDatabase(false);
    }
  }
  else if (settingId == "epg.resetepg")
  {
    if (CGUIDialogYesNo::ShowAndGetInput(19098, 19188))
    {
      CDateTime::ResetTimezoneBias();
      ResetDatabase(true);
    }
  }
  else if (settingId == "pvrmanager.channelscan")
  {
    if (IsStarted())
      StartChannelScan();
  }
  else if (settingId == "pvrmanager.channelmanager")
  {
    if (IsStarted())
    {
      CGUIDialogPVRChannelManager *dialog = (CGUIDialogPVRChannelManager *)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_CHANNEL_MANAGER);
      if (dialog)
        dialog->DoModal();
    }
  }
  else if (settingId == "pvrmanager.groupmanager")
  {
    if (IsStarted())
    {
      CGUIDialogPVRGroupManager *dialog = (CGUIDialogPVRGroupManager *)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GROUP_MANAGER);
      if (dialog)
        dialog->DoModal();
    }
  }
  else if (settingId == "pvrclient.menuhook")
  {
    if (IsStarted())
      m_addons->ProcessMenuHooks(-1, PVR_MENUHOOK_SETTING, NULL);
  }
}

bool CPVRManager::IsPVRWindowActive(void) const
{
  return g_windowManager.IsWindowActive(WINDOW_TV_CHANNELS) ||
      g_windowManager.IsWindowActive(WINDOW_TV_GUIDE) ||
      g_windowManager.IsWindowActive(WINDOW_TV_RECORDINGS) ||
      g_windowManager.IsWindowActive(WINDOW_TV_TIMERS) ||
      g_windowManager.IsWindowActive(WINDOW_TV_SEARCH) ||
      g_windowManager.IsWindowActive(WINDOW_RADIO_CHANNELS) ||
      g_windowManager.IsWindowActive(WINDOW_RADIO_GUIDE) ||
      g_windowManager.IsWindowActive(WINDOW_RADIO_RECORDINGS) ||
      g_windowManager.IsWindowActive(WINDOW_RADIO_TIMERS) ||
      g_windowManager.IsWindowActive(WINDOW_RADIO_SEARCH) ||
      g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_CHANNEL_MANAGER) ||
      g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_OSD_CHANNELS) ||
      g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_GROUP_MANAGER) ||
      g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_GUIDE_INFO) ||
      g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_OSD_GUIDE) ||
      g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_GUIDE_SEARCH) ||
      g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_RECORDING_INFO) ||
      g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_TIMER_SETTING);
}

bool CPVRManager::IsPVRWindow(int windowId)
{
  return (windowId == WINDOW_TV_CHANNELS ||
          windowId == WINDOW_TV_GUIDE ||
          windowId == WINDOW_TV_RECORDINGS ||
          windowId == WINDOW_TV_SEARCH ||
          windowId == WINDOW_TV_TIMERS ||
          windowId == WINDOW_RADIO_CHANNELS ||
          windowId == WINDOW_RADIO_GUIDE ||
          windowId == WINDOW_RADIO_RECORDINGS ||
          windowId == WINDOW_RADIO_SEARCH ||
          windowId == WINDOW_RADIO_TIMERS);
}

bool CPVRManager::InstallAddonAllowed(const std::string& strAddonId) const
{
  return !IsStarted() ||
      !m_addons->IsInUse(strAddonId) ||
      (!IsPVRWindowActive() && !IsPlaying());
}

void CPVRManager::MarkAsOutdated(const std::string& strAddonId, const std::string& strReferer)
{
  if (IsStarted() && CSettings::Get().GetInt("general.addonupdates") == AUTO_UPDATES_ON)
  {
    CSingleLock lock(m_critSection);
    m_outdatedAddons.insert(make_pair(strAddonId, strReferer));
  }
}

bool CPVRManager::UpgradeOutdatedAddons(void)
{
  CSingleLock lock(m_critSection);
  if (m_outdatedAddons.empty())
    return true;

  // there's add-ons that couldn't be updated
  for (std::map<std::string, std::string>::iterator it = m_outdatedAddons.begin(); it != m_outdatedAddons.end(); ++it)
  {
    if (!InstallAddonAllowed(it->first))
    {
      // we can't upgrade right now
      return true;
    }
  }

  // all outdated add-ons can be upgraded now
  CLog::Log(LOGINFO, "PVR - upgrading outdated add-ons");

  std::map<std::string, std::string> outdatedAddons = m_outdatedAddons;
  // stop threads and unload
  SetState(ManagerStateInterrupted);
  g_EpgContainer.Stop();
  m_guiInfo->Stop();
  m_addons->Stop();
  Cleanup();

  // upgrade all add-ons
  for (std::map<std::string, std::string>::iterator it = outdatedAddons.begin(); it != outdatedAddons.end(); ++it)
  {
    CLog::Log(LOGINFO, "PVR - updating add-on '%s'", it->first.c_str());
    CAddonInstaller::Get().Install(it->first, true, it->second, false);
  }

  // reload
  CLog::Log(LOGINFO, "PVRManager - %s - restarting the PVR manager", __FUNCTION__);
  SetState(ManagerStateStarting);
  ResetProperties();

  while (!Load() && IsInitialising())
  {
    CLog::Log(LOGERROR, "PVRManager - %s - failed to load PVR data, retrying", __FUNCTION__);
    if (m_guiInfo) m_guiInfo->Stop();
    if (m_addons) m_addons->Stop();
    Cleanup();
    Sleep(1000);
  }

  if (IsInitialising())
  {
    SetState(ManagerStateStarted);
    g_EpgContainer.Start();

    CLog::Log(LOGDEBUG, "PVRManager - %s - restarted", __FUNCTION__);
    return true;
  }

  return false;
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
  m_openWindowId = 0;
  m_bEpgsCreated = false;

  for (unsigned int iJobPtr = 0; iJobPtr < m_pendingUpdates.size(); iJobPtr++)
    delete m_pendingUpdates.at(iJobPtr);
  m_pendingUpdates.clear();

  /* unregister application action listener */
  g_application.UnregisterActionListener(&CPVRActionListener::Get());

  HideProgressDialog();

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
  CPVRManagerStartJob(int openWindowId = 0) :
    m_openWindowId(openWindowId) {}
  ~CPVRManagerStartJob(void) {}

  bool DoWork(void)
  {
    g_PVRManager.Start(false, m_openWindowId);
    return true;
  }
private:
  int m_openWindowId;
};

void CPVRManager::Start(bool bAsync /* = false */, int openWindowId /* = 0 */)
{
  if (bAsync)
  {
    CPVRManagerStartJob *job = new CPVRManagerStartJob(openWindowId);
    CJobManager::GetInstance().AddJob(job, NULL);
    return;
  }

  CSingleLock lock(m_critSection);

  /* first stop and remove any clients */
  Stop();

  /* don't start if Settings->Video->TV->Enable isn't checked */
  if (!CSettings::Get().GetBool("pvrmanager.enabled"))
    return;

  ResetProperties();
  SetState(ManagerStateStarting);
  m_openWindowId = openWindowId;

  /* create and open database */
  if (!m_database)
    m_database = new CPVRDatabase;
  m_database->Open();

  /* register application action listener */
  g_application.RegisterActionListener(&CPVRActionListener::Get());

  /* create the supervisor thread to do all background activities */
  StartUpdateThreads();
}

void CPVRManager::Stop(void)
{
  /* check whether the pvrmanager is loaded */
  if (IsStopping() || IsStopped())
    return;

  SetState(ManagerStateStopping);

  /* stop the EPG updater, since it might be using the pvr add-ons */
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
  {
    CSingleLock lock(m_managerStateMutex);
    m_managerState = state;
    SetChanged();
  }

  NotifyObservers(ObservableMessageManagerStateChanged);
}

void CPVRManager::Process(void)
{
  g_EpgContainer.Stop();

  /* load the pvr data from the db and clients if it's not already loaded */
  while (!Load() && IsInitialising())
  {
    CLog::Log(LOGERROR, "PVRManager - %s - failed to load PVR data, retrying", __FUNCTION__);
    if (m_guiInfo) m_guiInfo->Stop();
    if (m_addons) m_addons->Stop();
    Cleanup();
    Sleep(1000);
  }

  if (!IsInitialising())
    return;

  SetState(ManagerStateStarted);

  /* main loop */
  CLog::Log(LOGDEBUG, "PVRManager - %s - entering main loop", __FUNCTION__);
  g_EpgContainer.Start();

  /* activate startup window */
  if (m_openWindowId > 0)
  {
    g_windowManager.ActivateWindow(m_openWindowId);
    m_openWindowId = 0;

    // let everyone know that the user interface is now ready for usage
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UI_READY);
    g_windowManager.SendThreadMessage(msg);
  }

  bool bRestart(false);
  while (IsStarted() && m_addons && m_addons->HasConnectedClients() && !bRestart)
  {
    /* first startup */
    if (m_bFirstStart)
    {
      {
        CSingleLock lock(m_critSection);
        m_bFirstStart = false;
      }

      /* start job to search for missing channel icons */
      TriggerSearchMissingChannelIcons();

      /* try to continue last watched channel otherwise set group to last played group */
      if (!ContinueLastChannel())
        SetPlayingGroup(m_channelGroups->GetLastPlayedGroup());
    }
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
    else if (IsStarted() && !bRestart)
      m_triggerEvent.WaitMSec(1000);
  }

  if (IsStarted())
  {
    CLog::Log(LOGNOTICE, "PVRManager - %s - no add-ons enabled anymore. restarting the pvrmanager", __FUNCTION__);
    CApplicationMessenger::Get().SetPVRManagerState(true);
  }
  else
  {
    if (IsPVRWindow(g_windowManager.GetActiveWindow()))
      g_windowManager.ActivateWindow(WINDOW_HOME);
  }
}

bool CPVRManager::SetWakeupCommand(void)
{
  if (!CSettings::Get().GetBool("pvrpowermanagement.enabled"))
    return false;

  const std::string strWakeupCommand = CSettings::Get().GetString("pvrpowermanagement.setwakeupcmd");
  if (!strWakeupCommand.empty() && m_timers)
  {
    time_t iWakeupTime;
    const CDateTime nextEvent = m_timers->GetNextEventTime();
    if (nextEvent.IsValid())
    {
      nextEvent.GetAsTime(iWakeupTime);

      std::string strExecCommand = StringUtils::Format("%s %ld", strWakeupCommand.c_str(), iWakeupTime);

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
  while (IsInitialising() && m_addons && !m_addons->HasConnectedClients())
    Sleep(50);

  if (!IsInitialising() || !m_addons || !m_addons->HasConnectedClients())
    return false;

  CLog::Log(LOGDEBUG, "PVRManager - %s - active clients found. continue to start", __FUNCTION__);

  /* reset observer for pvr windows */
  for (std::size_t i = 0; i != ARRAY_SIZE(m_pvrWindowIds); i++)
  {
    CGUIWindowPVRBase *pWindow = (CGUIWindowPVRBase *) g_windowManager.GetWindow(m_pvrWindowIds[i]);
    if (pWindow)
      pWindow->ResetObservers();
  }

  /* load all channels and groups */
  ShowProgressDialog(g_localizeStrings.Get(19236), 0); // Loading channels from clients
  if (!m_channelGroups->Load() || !IsInitialising())
    return false;

  /* get timers from the backends */
  ShowProgressDialog(g_localizeStrings.Get(19237), 50); // Loading timers from clients
  m_timers->Load();

  /* get recordings from the backend */
  ShowProgressDialog(g_localizeStrings.Get(19238), 75); // Loading recordings from clients
  m_recordings->Load();

  if (!IsInitialising())
    return false;

  /* start the other pvr related update threads */
  ShowProgressDialog(g_localizeStrings.Get(19239), 85); // Starting background threads
  m_guiInfo->Start();

  /* close the progess dialog */
  HideProgressDialog();

  return true;
}

void CPVRManager::ShowProgressDialog(const std::string &strText, int iProgress)
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

  return PerformChannelSwitch(channel->GetPVRChannelInfoTag(), false);
}

bool CPVRManager::ChannelUpDown(unsigned int *iNewChannelNumber, bool bPreview, bool bUp)
{
  bool bReturn = false;
  if (IsPlayingTV() || IsPlayingRadio())
  {
    CFileItem currentFile(g_application.CurrentFileItem());
    CPVRChannelPtr currentChannel(currentFile.GetPVRChannelInfoTag());
    if (currentChannel)
    {
      CPVRChannelGroupPtr group = GetPlayingGroup(currentChannel->IsRadio());
      if (group)
      {
        CFileItemPtr newChannel = bUp ?
            group->GetByChannelUp(currentChannel) :
            group->GetByChannelDown(currentChannel);

        if (newChannel && newChannel->HasPVRChannelInfoTag() &&
            PerformChannelSwitch(newChannel->GetPVRChannelInfoTag(), bPreview))
        {
          *iNewChannelNumber = newChannel->GetPVRChannelInfoTag()->ChannelNumber();
          bReturn = true;
        }
      }
    }
  }

  return bReturn;
}

bool CPVRManager::ContinueLastChannel(void)
{
  if (CSettings::Get().GetInt("pvrplayback.startlast") == CONTINUE_LAST_CHANNEL_OFF)
    return false;

  CFileItemPtr channel = m_channelGroups->GetLastPlayedChannel();
  if (channel && channel->HasPVRChannelInfoTag())
  {
    CLog::Log(LOGNOTICE, "PVRManager - %s - continue playback on channel '%s'", __FUNCTION__, channel->GetPVRChannelInfoTag()->ChannelName().c_str());
    SetPlayingGroup(m_channelGroups->GetLastPlayedGroup(channel->GetPVRChannelInfoTag()->ChannelID()));
    StartPlayback(channel->GetPVRChannelInfoTag(), (CSettings::Get().GetInt("pvrplayback.startlast") == CONTINUE_LAST_CHANNEL_IN_BACKGROUND));
    return true;
  }

  CLog::Log(LOGDEBUG, "PVRManager - %s - no last played channel to continue playback found", __FUNCTION__);

  return false;
}

void CPVRManager::ResetDatabase(bool bResetEPGOnly /* = false */)
{
  CLog::Log(LOGNOTICE,"PVRManager - %s - clearing the PVR database", __FUNCTION__);

  g_EpgContainer.Stop();

  CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  pDlgProgress->SetLine(0, "");
  pDlgProgress->SetLine(1, g_localizeStrings.Get(19186)); // All data in the PVR database is being erased
  pDlgProgress->SetLine(2, "");
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
  if (m_database)
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

      /* delete all channel and recording settings */
      CVideoDatabase videoDatabase;

      if (videoDatabase.Open())
      {
        videoDatabase.EraseVideoSettings("pvr://channels/");
        videoDatabase.EraseVideoSettings("pvr://recordings/");
        videoDatabase.Close();
      }

      pDlgProgress->SetPercentage(80);
      pDlgProgress->Progress();

      /* delete all client information */
      pDlgProgress->SetPercentage(90);
      pDlgProgress->Progress();
    }

    m_database->Close();
  }

  CLog::Log(LOGNOTICE,"PVRManager - %s - %s database cleared", __FUNCTION__, bResetEPGOnly ? "EPG" : "PVR and EPG");

  if (CSettings::Get().GetBool("pvrmanager.enabled"))
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

CPVRChannelPtr CPVRManager::GetCurrentChannel(void) const
{
  return m_addons ? m_addons->GetPlayingChannel() : CPVRChannelPtr();
}

int CPVRManager::GetCurrentEpg(CFileItemList &results) const
{
  int iReturn = -1;

  CPVRChannelPtr channel(m_addons->GetPlayingChannel());
  if (channel)
    iReturn = channel->GetEPG(results);
  else
    CLog::Log(LOGDEBUG,"PVRManager - %s - no current channel set", __FUNCTION__);

  return iReturn;
}

void CPVRManager::ResetPlayingTag(void)
{
  CSingleLock lock(m_critSection);
  if (IsStarted() && m_guiInfo)
    m_guiInfo->ResetPlayingTag();
}

bool CPVRManager::ToggleRecordingOnChannel(unsigned int iChannelId)
{
  bool bReturn = false;

  CPVRChannelPtr channel(m_channelGroups->GetChannelById(iChannelId));
  if (!channel)
    return bReturn;

  if (m_addons->HasTimerSupport(channel->ClientID()))
  {
    /* timers are supported on this channel */
    if (!channel->IsRecording())
    {
      bReturn = m_timers->InstantTimer(channel);
      if (!bReturn)
        CGUIDialogOK::ShowAndGetInput(19033, 19164);
    }
    else
    {
      /* delete active timers */
      bReturn = m_timers->DeleteTimersOnChannel(channel, true, true);
    }
  }

  return bReturn;
}

bool CPVRManager::StartRecordingOnPlayingChannel(bool bOnOff)
{
  bool bReturn = false;

  CPVRChannelPtr channel(m_addons->GetPlayingChannel());
  if (!channel)
    return bReturn;

  if (m_addons->HasTimerSupport(channel->ClientID()))
  {
    /* timers are supported on this channel */
    if (bOnOff && !channel->IsRecording())
    {
      bReturn = m_timers->InstantTimer(channel);
      if (!bReturn)
        CGUIDialogOK::ShowAndGetInput(19033, 19164);
    }
    else if (!bOnOff && channel->IsRecording())
    {
      /* delete active timers */
      bReturn = m_timers->DeleteTimersOnChannel(channel, true, true);
    }
  }

  return bReturn;
}

bool CPVRManager::CheckParentalLock(const CPVRChannelPtr &channel)
{
  bool bReturn = !IsParentalLocked(channel) ||
      CheckParentalPIN();

  if (!bReturn)
    CLog::Log(LOGERROR, "PVRManager - %s - parental lock verification failed for channel '%s': wrong PIN entered.", __FUNCTION__, channel->ChannelName().c_str());

  return bReturn;
}

bool CPVRManager::IsParentalLocked(const CPVRChannelPtr &channel)
{
  bool bReturn(false);
  if (!IsStarted())
    return bReturn;
  CPVRChannelPtr currentChannel(GetCurrentChannel());

  if (// different channel
      (!currentChannel || channel != currentChannel) &&
      // parental control enabled
      CSettings::Get().GetBool("pvrparental.enabled") &&
      // channel is locked
      channel && channel->IsLocked())
  {
    float parentalDurationMs = CSettings::Get().GetInt("pvrparental.duration") * 1000.0f;
    bReturn = m_parentalTimer &&
        (!m_parentalTimer->IsRunning() ||
          m_parentalTimer->GetElapsedMilliseconds() > parentalDurationMs);
  }

  return bReturn;
}

bool CPVRManager::CheckParentalPIN(const std::string& strTitle /* = "" */)
{
  std::string pinCode = CSettings::Get().GetString("pvrparental.pin");

  if (!CSettings::Get().GetBool("pvrparental.enabled") || pinCode.empty())
    return true;

  // Locked channel. Enter PIN:
  bool bValidPIN = CGUIDialogNumeric::ShowAndVerifyInput(pinCode, !strTitle.empty() ? strTitle : g_localizeStrings.Get(19263), true);
  if (!bValidPIN)
    // display message: The entered PIN number was incorrect
    CGUIDialogOK::ShowAndGetInput(19264, 19265);
  else if (m_parentalTimer)
  {
    // reset the timer
    m_parentalTimer->StartZero();
  }

  return bValidPIN;
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

bool CPVREpgsCreateJob::DoWork(void)
{
  return g_PVRManager.CreateChannelEpgs();
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

bool CPVRManager::OpenLiveStream(const CFileItem &channel)
{
  bool bReturn(false);
  if (!channel.HasPVRChannelInfoTag())
    return bReturn;

  CLog::Log(LOGDEBUG,"PVRManager - %s - opening live stream on channel '%s'",
      __FUNCTION__, channel.GetPVRChannelInfoTag()->ChannelName().c_str());

  // check if we're allowed to play this file
  if (IsParentalLocked(channel.GetPVRChannelInfoTag()))
    return bReturn;

  if ((bReturn = m_addons->OpenStream(channel.GetPVRChannelInfoTag(), false)) != false)
  {
    CSingleLock lock(m_critSection);
    if(m_currentFile)
      delete m_currentFile;
    m_currentFile = new CFileItem(channel);

    // set channel as selected item
    if (channel.HasPVRChannelInfoTag())
      CGUIWindowPVRBase::SetSelectedItemPath(channel.GetPVRChannelInfoTag()->IsRadio(), channel.GetPVRChannelInfoTag()->Path());

    CPVRChannelPtr playingChannel(m_addons->GetPlayingChannel());
    if (playingChannel)
      UpdateLastWatched(playingChannel);
  }

  return bReturn;
}

bool CPVRManager::OpenRecordedStream(const CPVRRecordingPtr &tag)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  if ((bReturn = m_addons->OpenStream(tag)) != false)
  {
    delete m_currentFile;
    m_currentFile = new CFileItem(tag);
  }

  return bReturn;
}

void CPVRManager::CloseStream(void)
{
  CSingleLock lock(m_critSection);

  CPVRChannelPtr channel(m_addons->GetPlayingChannel());
  if (channel)
  {
    UpdateLastWatched(channel);

    // store channel settings
    g_application.SaveFileState();
  }

  m_addons->CloseStream();
  SAFE_DELETE(m_currentFile);
}

bool CPVRManager::PlayMedia(const CFileItem& item)
{
  if (!g_PVRManager.IsStarted())
  {
    CLog::Log(LOGERROR, "CApplication - %s PVR manager not started to play file '%s'", __FUNCTION__, item.GetPath().c_str());
    return false;
  }

  CFileItem pvrItem(item);
  if (URIUtils::IsPVRChannel(item.GetPath()) && !item.HasPVRChannelInfoTag())
    pvrItem = *g_PVRChannelGroups->GetByPath(item.GetPath());
  else if (URIUtils::IsPVRRecording(item.GetPath()) && !item.HasPVRRecordingInfoTag())
    pvrItem = *g_PVRRecordings->GetByPath(item.GetPath());

  if (!pvrItem.HasPVRChannelInfoTag() && !pvrItem.HasPVRRecordingInfoTag())
    return false;

  // check parental lock if we want to play a channel
  if (pvrItem.IsPVRChannel() && !g_PVRManager.CheckParentalLock(pvrItem.GetPVRChannelInfoTag()))
    return false;

  if (!g_application.IsCurrentThread())
  {
    CApplicationMessenger::Get().MediaPlay(pvrItem);
    return true;
  }

  return g_application.PlayFile(pvrItem, false) == PLAYBACK_OK;
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

  CPVRChannelPtr channelTag(item.GetPVRChannelInfoTag());
  CEpgInfoTagPtr epgTagNow(channelTag->GetEPGNow());

  if (channelTag->IsRadio())
  {
    CMusicInfoTag* musictag = item.GetMusicInfoTag();
    if (musictag)
    {
      musictag->SetTitle(epgTagNow ?
          epgTagNow->Title() :
          CSettings::Get().GetBool("epg.hidenoinfoavailable") ?
              "" :
              g_localizeStrings.Get(19055)); // no information available
      if (epgTagNow)
        musictag->SetGenre(epgTagNow->Genre());
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
      videotag->m_strTitle = epgTagNow ?
          epgTagNow->Title() :
          CSettings::Get().GetBool("epg.hidenoinfoavailable") ?
              "" :
              g_localizeStrings.Get(19055); // no information available
      if (epgTagNow)
        videotag->m_genre = epgTagNow->Genre();
      videotag->m_strPath = channelTag->Path();
      videotag->m_strFileNameAndPath = channelTag->Path();
      videotag->m_strPlot = epgTagNow ? epgTagNow->Plot() : "";
      videotag->m_strPlotOutline = epgTagNow ? epgTagNow->PlotOutline() : "";
      videotag->m_iEpisode = epgTagNow ? epgTagNow->EpisodeNumber() : 0;
    }
  }

  return false;
}

bool CPVRManager::StartPlayback(const CPVRChannelPtr &channel, bool bMinimised /* = false */)
{
  CMediaSettings::Get().SetVideoStartWindowed(bMinimised);
  CApplicationMessenger::Get().MediaPlay(CFileItem(channel));
  CLog::Log(LOGNOTICE, "PVRManager - %s - started playback on channel '%s'",
      __FUNCTION__, channel->ChannelName().c_str());
  return true;
}

bool CPVRManager::StartPlayback(PlaybackType type /* = PlaybackTypeAny */)
{
  bool bIsRadio(false);
  bool bReturn(false);
  bool bIsPlaying(false);
  CFileItemPtr channel;

  // check if the desired PlaybackType is already playing,
  // and if not, try to grab the last played channel of this type
  switch (type)
  {
    case PlaybackTypeRadio:
      if (IsPlayingRadio())
        bIsPlaying = true;
      else
        channel = m_channelGroups->GetGroupAllRadio()->GetLastPlayedChannel();
      bIsRadio = true;
      break;

    case PlaybackTypeTv:
      if (IsPlayingTV())
        bIsPlaying = true;
      else
        channel = m_channelGroups->GetGroupAllTV()->GetLastPlayedChannel();
      break;

    default:
      if (IsPlaying())
        bIsPlaying = true;
      else
        channel = m_channelGroups->GetLastPlayedChannel();
  }

  // we're already playing? Then nothing to do
  if (bIsPlaying)
    return true;

  // if we have a last played channel, start playback
  if (channel && channel->HasPVRChannelInfoTag())
  {
    bReturn = StartPlayback(channel->GetPVRChannelInfoTag(), false);
  }
  else
  {
    // if we don't, find the active channel group of the demanded type and play it's first channel
    CPVRChannelGroupPtr channelGroup = GetPlayingGroup(bIsRadio);
    if (channelGroup)
    {
      // try to start playback of first channel in this group
      std::vector<PVRChannelGroupMember> groupMembers(channelGroup->GetMembers());
      if (!groupMembers.empty())
        bReturn = StartPlayback((*groupMembers.begin()).channel, false);
    }
  }

  if (!bReturn)
  {
    CLog::Log(LOGNOTICE, "PVRManager - %s - could not determine %s channel to start playback with. No last played channel found, and first channel of active group could also not be determined.", __FUNCTION__, bIsRadio ? "radio": "tv");

    std::string msg = StringUtils::Format(g_localizeStrings.Get(19035).c_str(), g_localizeStrings.Get(bIsRadio ? 19021 : 19020).c_str()); // RADIO/TV could not be played. Check the log for details.
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error,
            g_localizeStrings.Get(19166), // PVR information
            msg);
  }

  return bReturn;
}


bool CPVRManager::PerformChannelSwitch(const CPVRChannelPtr &channel, bool bPreview)
{
  assert(channel.get());

  // check parental lock state
  if (IsParentalLocked(channel))
    return false;

  // invalid channel
  if (channel->ClientID() < 0)
    return false;

  // check whether we're waiting for a previous switch to complete
  {
    CSingleLock lock(m_critSection);
    if (m_bIsSwitchingChannels)
    {
      CLog::Log(LOGDEBUG, "PVRManager - %s - can't switch to channel '%s'. waiting for the previous switch to complete",
          __FUNCTION__, channel->ChannelName().c_str());
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

  CLog::Log(LOGDEBUG, "PVRManager - %s - switching to channel '%s'", __FUNCTION__, channel->ChannelName().c_str());

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

    CLog::Log(LOGERROR, "PVRManager - %s - failed to switch to channel '%s'", __FUNCTION__, channel->ChannelName().c_str());

    std::string msg = StringUtils::Format(g_localizeStrings.Get(19035).c_str(), channel->ChannelName().c_str()); // CHANNELNAME could not be played. Check the log for details.
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error,
        g_localizeStrings.Get(19166), // PVR information
        msg);
  }
  else
  {
    // switch successful
    bSwitched = true;

    // save previous and load new channel's settings (view mode is updated in
    // the player)
    g_application.SaveFileState();
    g_application.LoadVideoSettings(channel);

    // set channel as selected item
    CGUIWindowPVRBase::SetSelectedItemPath(channel->IsRadio(), channel->Path());

    UpdateLastWatched(channel);

    CSingleLock lock(m_critSection);
    m_currentFile = new CFileItem(channel);
    m_bIsSwitchingChannels = false;

    CLog::Log(LOGNOTICE, "PVRManager - %s - switched to channel '%s'", __FUNCTION__, channel->ChannelName().c_str());
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

bool CPVRManager::TranslateCharInfo(DWORD dwInfo, std::string &strValue) const
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
  bool bReturn(true);
  if (IsStarted())
  {
    if (IsRecording() || IsPlaying()) // pvr recording or playing?
      bReturn = false;
    else
      bReturn = !IsNextEventWithinBackendIdleTime();
  }
  return bReturn;
}

bool CPVRManager::CanSystemPowerdown(bool bAskUser /*= true*/) const
{
  bool bReturn(true);
  if (IsStarted())
  {
    CPVRTimerInfoTagPtr cause;
    if (!AllLocalBackendsIdle(cause))
    {
      if (bAskUser)
      {
        std::string text;

        if (cause)
        {
          if (cause->IsRecording())
          {
            text = StringUtils::Format(g_localizeStrings.Get(19691).c_str(), // "PVR is currently recording...."
                                       cause->Title().c_str(),
                                       cause->ChannelName().c_str());
          }
          else
          {
            // Next event is due to a local recording.

            const CDateTime now(CDateTime::GetUTCDateTime());
            const CDateTime start(cause->StartAsUTC());
            const CDateTimeSpan prestart(0, 0, cause->MarginStart(), 0);
            const CDateTimeSpan prewakeup(0, 0, CSettings::Get().GetInt("pvrpowermanagement.prewakeup"), 0);

            CDateTimeSpan diff(start - now);
            diff -= prestart - prewakeup;
            int mins = diff.GetSecondsTotal() / 60;

            std::string dueStr;
            if (mins > 1)
            {
              // "%d minutes"
              dueStr = StringUtils::Format(g_localizeStrings.Get(19694).c_str(), mins);
            }
            else
            {
              // "about a minute"
              dueStr = g_localizeStrings.Get(19695);
            }

            text = StringUtils::Format(g_localizeStrings.Get(19692).c_str(), // "PVR will start recording...."
                                       cause->Title().c_str(),
                                       cause->ChannelName().c_str(),
                                       dueStr.c_str());
          }
        }
        else
        {
          // Next event is due to automatic daily wakeup of PVR.
          const CDateTime now(CDateTime::GetUTCDateTime());

          CDateTime dailywakeuptime;
          dailywakeuptime.SetFromDBTime(CSettings::Get().GetString("pvrpowermanagement.dailywakeuptime"));
          dailywakeuptime = dailywakeuptime.GetAsUTCDateTime();

          const CDateTimeSpan diff(dailywakeuptime - now);
          int mins = diff.GetSecondsTotal() / 60;

          std::string dueStr;
          if (mins > 1)
          {
            // "%d minutes"
            dueStr = StringUtils::Format(g_localizeStrings.Get(19694).c_str(), mins);
          }
          else
          {
            // "about a minute"
            dueStr = g_localizeStrings.Get(19695);
          }

          text = StringUtils::Format(g_localizeStrings.Get(19693).c_str(), // "Daily wakeup is due in...."
                                     dueStr.c_str());
        }

        // Inform user about PVR being busy. Ask if user wants to powerdown anyway.
        bool bCanceled = false;
        bReturn = CGUIDialogYesNo::ShowAndGetInput(19685, // "Confirm shutdown"
                                                   text,
                                                   bCanceled,
                                                   222,   // "Cancel"
                                                   19696, // "Shutdown anyway"
                                                   10000);
      }
      else
        bReturn = false; // do not powerdown (busy, but no user interaction requested).
    }
  }
  return bReturn;
}

bool CPVRManager::AllLocalBackendsIdle(CPVRTimerInfoTagPtr& causingEvent) const
{
  if (m_timers)
  {
    // active recording on local backend?
    std::vector<CFileItemPtr> recordings = m_timers->GetActiveRecordings();
    for (std::vector<CFileItemPtr>::const_iterator timerIt = recordings.begin(); timerIt != recordings.end(); ++timerIt)
    {
      if (EventOccursOnLocalBackend(*timerIt))
      {
        causingEvent = (*timerIt)->GetPVRTimerInfoTag();
        return false;
      }
    }

    // soon recording on local backend?
    if (IsNextEventWithinBackendIdleTime())
    {
      CFileItemPtr item = m_timers->GetNextActiveTimer();
      if (item.get() == NULL)
      {
        // Next event is due to automatic daily wakeup of PVR!
        causingEvent.reset();
        return false;
      }

      if (EventOccursOnLocalBackend(item))
      {
        causingEvent = item->GetPVRTimerInfoTag();
        return false;
      }
    }
  }
  return true;
}

bool CPVRManager::EventOccursOnLocalBackend(const CFileItemPtr& item) const
{
  if (item && item->HasPVRTimerInfoTag())
  {
    CPVRTimerInfoTagPtr tag(item->GetPVRTimerInfoTag());
    std::string hostname(m_addons->GetBackendHostnameByClientId(tag->m_iClientId));
    if (!hostname.empty() && g_application.getNetwork().IsLocalHost(hostname))
      return true;
  }
  return false;
}

bool CPVRManager::IsNextEventWithinBackendIdleTime(void) const
{
  // timers going off soon?
  const CDateTime now(CDateTime::GetUTCDateTime());
  const CDateTimeSpan idle(
    0, 0, CSettings::Get().GetInt("pvrpowermanagement.backendidletime"), 0);
  const CDateTime next(m_timers->GetNextEventTime());
  const CDateTimeSpan delta(next - now);

  return (delta <= idle);
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

bool CPVRManager::EpgsCreated(void) const
{
  CSingleLock lock(m_critSection);
  return m_bEpgsCreated;
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

void CPVRManager::QueueJob(CJob *job)
{
  CSingleLock lock(m_critSectionTriggers);
  if (!IsStarted() || IsJobPending(job->GetType()))
  {
    delete job;
    return;
  }

  m_pendingUpdates.push_back(job);

  lock.Leave();
  m_triggerEvent.Set();
}

void CPVRManager::TriggerEpgsCreate(void)
{
  QueueJob(new CPVREpgsCreateJob());
}

void CPVRManager::TriggerRecordingsUpdate(void)
{
  QueueJob(new CPVRRecordingsUpdateJob());
}

void CPVRManager::TriggerTimersUpdate(void)
{
  QueueJob(new CPVRTimersUpdateJob());
}

void CPVRManager::TriggerChannelsUpdate(void)
{
  QueueJob(new CPVRChannelsUpdateJob());
}

void CPVRManager::TriggerChannelGroupsUpdate(void)
{
  QueueJob(new CPVRChannelGroupsUpdateJob());
}

void CPVRManager::TriggerSearchMissingChannelIcons(void)
{
  CJobManager::GetInstance().AddJob(new CPVRSearchMissingChannelIconsJob(), NULL);
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
    ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::Player, "xbmc", "OnStop", CFileItemPtr(m_previous), data);
  }

  // announce OnPlay if the switch was successful
  if (m_next)
  {
    CVariant param;
    param["player"]["speed"] = 1;
    param["player"]["playerid"] = g_playlistPlayer.GetCurrentPlaylist();
    ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::Player, "xbmc", "OnPlay", CFileItemPtr(new CFileItem(*m_next)), param);
  }

  return true;
}

bool CPVRSearchMissingChannelIconsJob::DoWork(void)
{
  g_PVRManager.SearchMissingChannelIcons();
  return true;
}

bool CPVRManager::CreateChannelEpgs(void)
{
  if (EpgsCreated())
    return true;

  CSingleLock lock(m_critSection);
  m_bEpgsCreated = m_channelGroups->CreateChannelEpgs();
  return m_bEpgsCreated;
}

std::string CPVRManager::GetPlayingTVGroupName()
{
  return IsStarted() && m_guiInfo ? m_guiInfo->GetPlayingTVGroup() : "";
}

void CPVRManager::UpdateLastWatched(const CPVRChannelPtr &channel)
{
  assert(channel.get());

  time_t tNow;
  CDateTime::GetCurrentDateTime().GetAsTime(tNow);

  channel->SetLastWatched(tNow);

  // update last watched timestamp for group
  CPVRChannelGroupPtr group(GetPlayingGroup(channel->IsRadio()));
  group->SetLastWatched(tNow);

  /* update last played group */
  m_channelGroups->SetLastPlayedGroup(group);
}

/*
 *      Copyright (C) 2012-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PVRManager.h"

#include <cassert>
#include <utility>

#include "Application.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "epg/EpgContainer.h"
#include "GUIInfoManager.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "music/tags/MusicInfoTag.h"
#include "network/Network.h"
#include "PlayListPlayer.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupInternal.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/PVRActionListener.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRGUIInfo.h"
#include "pvr/PVRJobs.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/recordings/PVRRecordingsPath.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/windows/GUIWindowPVRBase.h"
#include "settings/lib/Setting.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "threads/SystemClock.h"
#include "Util.h"
#include "utils/JobManager.h"
#include "utils/log.h"
#include "utils/Stopwatch.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "video/VideoDatabase.h"
#include "ServiceBroker.h"

using namespace MUSIC_INFO;
using namespace PVR;
using namespace EPG;
using namespace ANNOUNCEMENT;
using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;

CPVRManagerJobQueue::CPVRManagerJobQueue()
: m_triggerEvent(false),
m_bStopped(true)
{
}

void CPVRManagerJobQueue::Start()
{
  CSingleLock lock(m_critSection);
  m_bStopped = false;
  m_triggerEvent.Set();
}

void CPVRManagerJobQueue::Stop()
{
  CSingleLock lock(m_critSection);
  m_bStopped = true;
  m_triggerEvent.Reset();
}

void CPVRManagerJobQueue::Clear()
{
  CSingleLock lock(m_critSection);
  for (CJob *updateJob : m_pendingUpdates)
    delete updateJob;

  m_pendingUpdates.clear();
  m_triggerEvent.Set();
}

void CPVRManagerJobQueue::AppendJob(CJob * job)
{
  CSingleLock lock(m_critSection);

  // check for another pending job of given type...
  for (CJob *updateJob : m_pendingUpdates)
  {
    if (!strcmp(updateJob->GetType(), job->GetType()))
    {
      delete job;
      return;
    }
  }

  m_pendingUpdates.push_back(job);
  m_triggerEvent.Set();
}

void CPVRManagerJobQueue::ExecutePendingJobs()
{
  std::vector<CJob *> pendingUpdates;

  {
    CSingleLock lock(m_critSection);

    if (m_bStopped)
      return;

    pendingUpdates = std::move(m_pendingUpdates);
    m_triggerEvent.Reset();
  }

  CJob *job = nullptr;
  while (!pendingUpdates.empty())
  {
    job = pendingUpdates.front();
    pendingUpdates.erase(pendingUpdates.begin());

    job->DoWork();
    delete job;
  }
}

bool CPVRManagerJobQueue::WaitForJobs(unsigned int milliSeconds)
{
  return m_triggerEvent.WaitMSec(milliSeconds);
}

CPVRManager::CPVRManager(void) :
    CThread("PVRManager"),
    m_addons(new CPVRClients),
    m_bFirstStart(true),
    m_bIsSwitchingChannels(false),
    m_bEpgsCreated(false),
    m_progressBar(nullptr),
    m_progressHandle(nullptr),
    m_managerState(ManagerStateStopped),
    m_isChannelPreview(false),
    m_bSettingPowerManagementEnabled(CServiceBroker::GetSettings().GetBool(CSettings::SETTING_PVRPOWERMANAGEMENT_ENABLED)),
    m_strSettingWakeupCommand(CServiceBroker::GetSettings().GetString(CSettings::SETTING_PVRPOWERMANAGEMENT_SETWAKEUPCMD))
{
  CAnnouncementManager::GetInstance().AddAnnouncer(this);
}

CPVRManager::~CPVRManager(void)
{
  CServiceBroker::GetSettings().UnregisterCallback(this);
  CAnnouncementManager::GetInstance().RemoveAnnouncer(this);
  CLog::Log(LOGDEBUG,"PVRManager - destroyed");
}

void CPVRManager::Announce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (!IsStarted())
    return;

  if ((flag & (ANNOUNCEMENT::GUI)))
  {
    if (strcmp(message, "OnScreensaverActivated") == 0)
      CServiceBroker::GetPVRManager().Clients()->OnPowerSavingActivated();
    else if (strcmp(message, "OnScreensaverDeactivated") == 0)
      CServiceBroker::GetPVRManager().Clients()->OnPowerSavingDeactivated();
  }
}

void CPVRManager::SaveLastPlayedChannel() const
{
  const CPVRChannelPtr playingChannel(GetCurrentChannel());
  if (playingChannel)
    playingChannel->SetWasPlayingOnLastQuit(true);
}

CPVRDatabasePtr CPVRManager::GetTVDatabase(void) const
{
  CSingleLock lock(m_critSection);
  if (!m_database || !m_database->IsOpen())
    CLog::Log(LOGERROR, "PVRManager - %s - failed to open the database", __FUNCTION__);

  return m_database;
}

CPVRChannelGroupsContainerPtr CPVRManager::ChannelGroups(void) const
{
  CSingleLock lock(m_critSection);
  return m_channelGroups;
}

CPVRRecordingsPtr CPVRManager::Recordings(void) const
{
  CSingleLock lock(m_critSection);
  return m_recordings;
}

CPVRTimersPtr CPVRManager::Timers(void) const
{
  CSingleLock lock(m_critSection);
  return m_timers;
}

CPVRClientsPtr CPVRManager::Clients(void) const
{
  // note: m_addons is const (only set/reset in ctor/dtor). no need for a lock here.
  return m_addons;
}

void CPVRManager::OnSettingChanged(const CSetting *setting)
{
  if (setting == nullptr)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_EPG_DAYSTODISPLAY)
  {
    m_addons->SetEPGTimeFrame(static_cast<const CSettingInt*>(setting)->GetValue());
  }
  else if (settingId == CSettings::SETTING_PVRPOWERMANAGEMENT_ENABLED)
  {
    CSingleLock lock(m_critSection);
    m_bSettingPowerManagementEnabled = static_cast<const CSettingBool*>(setting)->GetValue();
  }
  else if (settingId == CSettings::SETTING_PVRPOWERMANAGEMENT_SETWAKEUPCMD)
  {
    CSingleLock lock(m_critSection);
    m_strSettingWakeupCommand = static_cast<const CSettingString*>(setting)->GetValue();
  }
}

void CPVRManager::Clear(void)
{
  g_application.UnregisterActionListener(&CPVRActionListener::GetInstance());

  m_pendingUpdates.Clear();

  CSingleLock lock(m_critSection);

  m_guiInfo.reset();
  m_timers.reset();
  m_recordings.reset();
  m_channelGroups.reset();
  m_parentalTimer.reset();
  m_database.reset();

  m_currentFile.reset();
  m_bIsSwitchingChannels  = false;
  m_bEpgsCreated = false;

  HideProgressDialog();
}

void CPVRManager::ResetProperties(void)
{
  CSingleLock lock(m_critSection);
  Clear();

  m_database.reset(new CPVRDatabase);
  m_channelGroups.reset(new CPVRChannelGroupsContainer);
  m_recordings.reset(new CPVRRecordings);
  m_timers.reset(new CPVRTimers);
  m_guiInfo.reset(new CPVRGUIInfo);
  m_parentalTimer.reset(new CStopWatch);
}

void CPVRManager::Init()
{
  std::set<std::string> settingSet;
  settingSet.insert(CSettings::SETTING_EPG_DAYSTODISPLAY);
  settingSet.insert(CSettings::SETTING_PVRPOWERMANAGEMENT_ENABLED);
  settingSet.insert(CSettings::SETTING_PVRPOWERMANAGEMENT_SETWAKEUPCMD);
  CServiceBroker::GetSettings().RegisterCallback(this, settingSet);

  // Create and init action listener
  CPVRActionListener::GetInstance().Init();

  // Note: we're holding the progress bar dialog instance pointer in a member because it is needed by pvr core
  //       components. The latter might run in a different thread than the gui and g_windowManager.GetWindow()
  //       locks the global graphics mutex, which easily can lead to deadlocks.
  m_progressBar = g_windowManager.GetWindow<CGUIDialogExtendedProgressBar>();

  if (!m_progressBar)
    CLog::Log(LOGERROR, "CPVRManager - %s - unable to get WINDOW_DIALOG_EXT_PROGRESS!", __FUNCTION__);

  // initial check for enabled addons
  // if at least one pvr addon is enabled, PVRManager start up
  CJobManager::GetInstance().AddJob(new CPVRStartupJob(), nullptr);
}

void CPVRManager::Reinit()
{
  // initial check for enabled addons
  // if at least one pvr addon is enabled, PVRManager start up
  CJobManager::GetInstance().AddJob(new CPVRStartupJob(), nullptr);
}

void CPVRManager::Start()
{
  CSingleLock initLock(m_startStopMutex);

  // Note: Stop() must not be called while holding pvr manager's mutex. Stop() calls
  // StopThread() which can deadlock if the worker thread tries to acquire pvr manager's
  // lock while StopThread() is waiting for the worker to exit. Thus, we introduce another
  // lock here (m_startStopMutex), which only gets hold while starting/restarting pvr manager.
  Stop();

  CSingleLock lock(m_critSection);

  if (!m_addons->HasCreatedClients())
    return;

  ResetProperties();
  SetState(ManagerStateStarting);

  m_pendingUpdates.Start();

  m_database->Open();

  /* create the pvrmanager thread, which will ensure that all data will be loaded */
  Create();
  SetPriority(-1);
}

void CPVRManager::Stop(void)
{
  CSingleLock initLock(m_startStopMutex);

  /* check whether the pvrmanager is loaded */
  if (IsStopped())
    return;

  /* stop playback if needed */
  if (IsPlaying())
  {
    CLog::Log(LOGNOTICE,"PVRManager - %s - stopping PVR playback", __FUNCTION__);
    CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_STOP);
  }

  SetState(ManagerStateStopping);

  m_pendingUpdates.Stop();

  /* stop the EPG updater, since it might be using the pvr add-ons */
  g_EpgContainer.Stop();

  CLog::Log(LOGNOTICE, "PVRManager - stopping");

  /* stop all update threads */
  SetState(ManagerStateInterrupted);

  StopThread();

  if (m_guiInfo)
    m_guiInfo->Stop();

  /* close database */
  const CPVRDatabasePtr database(GetTVDatabase());
  if (database && database->IsOpen())
    database->Close();

  SetState(ManagerStateStopped);
}

void CPVRManager::Unload()
{
  // stop pvr manager thread and clear all pvr data
  Stop();
  Clear();

  // stop epg container thread and clear all epg data
  g_EpgContainer.Stop();
  g_EpgContainer.Clear();
}

void CPVRManager::Deinit()
{
  SaveLastPlayedChannel();
  SetWakeupCommand();
  Unload();

  // release addons
  m_addons.reset();

  // deinit action listener
  CPVRActionListener::GetInstance().Deinit();
}

CPVRManager::ManagerState CPVRManager::GetState(void) const
{
  CSingleLock lock(m_managerStateMutex);
  return m_managerState;
}

void CPVRManager::SetState(CPVRManager::ManagerState state)
{
  ObservableMessage observableMsg(ObservableMessageNone);

  {
    CSingleLock lock(m_managerStateMutex);
    if (m_managerState == state)
      return;

    m_managerState = state;

    PVREvent event;
    switch (state)
    {
      case ManagerStateError:
        event = ManagerError;
        break;
      case ManagerStateStopped:
        event = ManagerStopped;
        observableMsg = ObservableMessageManagerStopped;
        break;
      case ManagerStateStarting:
        event = ManagerStarting;
        break;
      case ManagerStateStopping:
        event = ManagerStopped;
        break;
      case ManagerStateInterrupted:
        event = ManagerInterrupted;
        break;
      case ManagerStateStarted:
        event = ManagerStarted;
        break;
      default:
        return;
    }
    m_events.Publish(event);
  }

  if (observableMsg != ObservableMessageNone)
  {
    SetChanged();
    NotifyObservers(observableMsg);
  }
}

void CPVRManager::PublishEvent(PVREvent event)
{
  m_events.Publish(event);
}

void CPVRManager::Process(void)
{
  /* register application action listener */
  {
    CSingleExit exit(m_critSection);
    g_application.RegisterActionListener(&CPVRActionListener::GetInstance());
  }

  g_EpgContainer.Stop();

  /* load the pvr data from the db and clients if it's not already loaded */
  XbmcThreads::EndTime progressTimeout(30000); // 30 secs
  while (!Load(!progressTimeout.IsTimePast()) && IsInitialising())
  {
    CLog::Log(LOGERROR, "PVRManager - %s - failed to load PVR data, retrying", __FUNCTION__);
    Sleep(1000);
  }

  if (!IsInitialising())
    return;

  SetState(ManagerStateStarted);

  /* start epg container */
  g_EpgContainer.Start(true);

  /* main loop */
  CLog::Log(LOGDEBUG, "PVRManager - %s - entering main loop", __FUNCTION__);

  bool bRestart(false);
  while (IsStarted() && m_addons->HasCreatedClients() && !bRestart)
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

      /* try to continue last watched channel */
      TriggerContinueLastChannel();
    }
    /* execute the next pending jobs if there are any */
    try
    {
      m_pendingUpdates.ExecutePendingJobs();
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "PVRManager - %s - an error occured while trying to execute the last update job, trying to recover", __FUNCTION__);
      bRestart = true;
    }

    if (IsStarted() && !bRestart)
      m_pendingUpdates.WaitForJobs(1000);
  }

  if (IsStarted())
  {
    CLog::Log(LOGNOTICE, "PVRManager - %s - no add-ons enabled anymore. restarting the pvrmanager", __FUNCTION__);
    CApplicationMessenger::GetInstance().PostMsg(TMSG_SETPVRMANAGERSTATE, 1);
  }
}

bool CPVRManager::SetWakeupCommand(void)
{
  bool bSettingPowerManagementEnabled;
  std::string strSettingWakeupCommand;

  {
    CSingleLock lock(m_critSection);
    bSettingPowerManagementEnabled = m_bSettingPowerManagementEnabled;
    strSettingWakeupCommand = m_strSettingWakeupCommand;
  }

  if (!bSettingPowerManagementEnabled)
    return false;

  if (!strSettingWakeupCommand.empty() && m_timers)
  {
    time_t iWakeupTime;
    const CDateTime nextEvent = m_timers->GetNextEventTime();
    if (nextEvent.IsValid())
    {
      nextEvent.GetAsTime(iWakeupTime);

      std::string strExecCommand = StringUtils::Format("%s %ld", strSettingWakeupCommand.c_str(), iWakeupTime);

      const int iReturn = system(strExecCommand.c_str());
      if (iReturn != 0)
        CLog::Log(LOGERROR, "%s - failed to execute wakeup command '%s': %s (%d)", __FUNCTION__, strExecCommand.c_str(), strerror(iReturn), iReturn);

      return iReturn == 0;
    }
  }

  return false;
}

void CPVRManager::OnSleep()
{
  SaveLastPlayedChannel();
  SetWakeupCommand();

  CServiceBroker::GetPVRManager().Clients()->OnSystemSleep();
}

void CPVRManager::OnWake()
{
  CServiceBroker::GetPVRManager().Clients()->OnSystemWake();

  /* start job to search for missing channel icons */
  TriggerSearchMissingChannelIcons();

  /* continue last watched channel */
  TriggerContinueLastChannel();

  /* trigger PVR data updates */
  TriggerChannelGroupsUpdate();
  TriggerChannelsUpdate();
  TriggerRecordingsUpdate();
  TriggerEpgsCreate();
  TriggerTimersUpdate();
}

bool CPVRManager::Load(bool bShowProgress)
{
  if (!bShowProgress)
    HideProgressDialog();

  /* load at least one client */
  while (IsInitialising() && m_addons && !m_addons->HasCreatedClients())
    Sleep(50);

  if (!IsInitialising() || !m_addons->HasCreatedClients())
    return false;

  CLog::Log(LOGDEBUG, "PVRManager - %s - active clients found. continue to start", __FUNCTION__);

  /* load all channels and groups */
  if (bShowProgress)
    ShowProgressDialog(g_localizeStrings.Get(19236), 0); // Loading channels from clients
  if (!m_channelGroups->Load() || !IsInitialising())
    return false;

  SetChanged();
  NotifyObservers(ObservableMessageChannelGroupsLoaded);

  /* get timers from the backends */
  if (bShowProgress)
    ShowProgressDialog(g_localizeStrings.Get(19237), 50); // Loading timers from clients
  m_timers->Load();

  /* get recordings from the backend */
  if (bShowProgress)
    ShowProgressDialog(g_localizeStrings.Get(19238), 75); // Loading recordings from clients
  m_recordings->Load();

  if (!IsInitialising())
    return false;

  /* start the other pvr related update threads */
  if (bShowProgress)
    ShowProgressDialog(g_localizeStrings.Get(19239), 85); // Starting background threads
  m_guiInfo->Start();

  /* close the progress dialog */
  if (bShowProgress)
    HideProgressDialog();

  return true;
}

void CPVRManager::ShowProgressDialog(const std::string &strText, int iProgress)
{
  if (!m_progressHandle && m_progressBar)
    m_progressHandle = m_progressBar->GetHandle(g_localizeStrings.Get(19235)); // PVR manager is starting up

  if (m_progressHandle)
  {
    m_progressHandle->SetPercentage(static_cast<float>(iProgress));
    m_progressHandle->SetText(strText);
  }
}

void CPVRManager::HideProgressDialog(void)
{
  if (m_progressHandle)
  {
    m_progressHandle->MarkFinished();
    m_progressHandle = NULL;
  }
}

CGUIDialogProgressBarHandle* CPVRManager::ShowProgressDialog(const std::string &strTitle) const
{
  if (m_progressBar)
    return m_progressBar->GetHandle(strTitle);

  return nullptr;
}

bool CPVRManager::ChannelSwitchById(unsigned int iChannelId)
{
  CSingleLock lock(m_critSection);

  CPVRChannelPtr channel = m_channelGroups->GetChannelById(iChannelId);
  if (channel)
  {
    SetPlayingGroup(channel);
    return PerformChannelSwitch(channel, false);
  }

  CLog::Log(LOGERROR, "PVRManager - %s - cannot find channel with id %d", __FUNCTION__, iChannelId);
  return false;
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

void CPVRManager::TriggerContinueLastChannel(void)
{
  if (IsStarted())
    CJobManager::GetInstance().AddJob(new CPVRContinueLastChannelJob(), nullptr);
}

bool CPVRManager::IsPlaying(void) const
{
  return IsStarted() && m_addons->IsPlaying();
}

bool CPVRManager::IsPlayingChannel(const CPVRChannelPtr &channel) const
{
  bool bReturn(false);

  if (channel && IsStarted())
  {
    CPVRChannelPtr current(GetCurrentChannel());
    if (current && *current == *channel)
      bReturn = true;
  }

  return bReturn;
}

bool CPVRManager::IsPlayingRecording(const CPVRRecordingPtr &recording) const
{
  bool bReturn(false);

  if (recording && IsStarted())
  {
    CPVRRecordingPtr current(GetCurrentRecording());
    if (current && *current == *recording)
      bReturn = true;
  }

  return bReturn;
}

CPVRChannelPtr CPVRManager::GetCurrentChannel(void) const
{
  return m_addons->GetPlayingChannel();
}

CPVRRecordingPtr CPVRManager::GetCurrentRecording(void) const
{
  return m_addons->GetPlayingRecording();
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

void CPVRManager::StartRecordingOnPlayingChannel(bool bOnOff)
{
  // can be called from VideoPlayer thread. SetRecordingOnChannel can open a dialog. Thus, execute async.
  CJobManager::GetInstance().AddJob(new CPVRSetRecordingOnChannelJob(m_addons->GetPlayingChannel(), bOnOff), NULL);
}

void CPVRManager::RestartParentalTimer()
{
  if (m_parentalTimer)
    m_parentalTimer->StartZero();
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
      CServiceBroker::GetSettings().GetBool(CSettings::SETTING_PVRPARENTAL_ENABLED) &&
      // channel is locked
      channel && channel->IsLocked())
  {
    float parentalDurationMs = CServiceBroker::GetSettings().GetInt(CSettings::SETTING_PVRPARENTAL_DURATION) * 1000.0f;
    bReturn = m_parentalTimer &&
        (!m_parentalTimer->IsRunning() ||
          m_parentalTimer->GetElapsedMilliseconds() > parentalDurationMs);
  }

  return bReturn;
}

void CPVRManager::SetPlayingGroup(const CPVRChannelGroupPtr &group)
{
  if (m_channelGroups && group)
    m_channelGroups->Get(group->IsRadio())->SetSelectedGroup(group);
}

void CPVRManager::SetPlayingGroup(const CPVRChannelPtr &channel)
{
  CPVRChannelGroupPtr group = m_channelGroups->GetSelectedGroup(channel->IsRadio());
  if (!group || !group->IsGroupMember(channel))
  {
    // The channel we'll switch to is not part of the current selected group.
    // Set the first group as the selected group where the channel is a member.
    CPVRChannelGroups *channelGroups = m_channelGroups->Get(channel->IsRadio());
    std::vector<CPVRChannelGroupPtr> groups = channelGroups->GetGroupsByChannel(channel, true);
    if (!groups.empty())
      channelGroups->SetSelectedGroup(groups.front());
  }
}

CPVRChannelGroupPtr CPVRManager::GetPlayingGroup(bool bRadio /* = false */)
{
  if (m_channelGroups)
    return m_channelGroups->GetSelectedGroup(bRadio);

  return CPVRChannelGroupPtr();
}

bool CPVRManager::OpenLiveStream(const CFileItem &fileItem)
{
  bool bReturn(false);
  if (!fileItem.HasPVRChannelInfoTag())
    return bReturn;

  CLog::Log(LOGDEBUG,"PVRManager - %s - opening live stream on channel '%s'",
      __FUNCTION__, fileItem.GetPVRChannelInfoTag()->ChannelName().c_str());

  // check if we're allowed to play this file
  if (IsParentalLocked(fileItem.GetPVRChannelInfoTag()))
    return bReturn;

  if ((bReturn = m_addons->OpenStream(fileItem.GetPVRChannelInfoTag(), false)) != false)
  {
    CSingleLock lock(m_critSection);
    m_currentFile.reset(new CFileItem(fileItem));
  }

  if (bReturn)
  {
    const CPVRChannelPtr channel(m_addons->GetPlayingChannel());
    if (channel)
    {
      SetPlayingGroup(channel);
      UpdateLastWatched(channel);
      // set channel as selected item
      CGUIWindowPVRBase::SetSelectedItemPath(channel->IsRadio(), channel->Path());
    }
  }

  return bReturn;
}

bool CPVRManager::OpenRecordedStream(const CPVRRecordingPtr &tag)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  if ((bReturn = m_addons->OpenStream(tag)) != false)
  {
    m_currentFile.reset(new CFileItem(tag));
  }

  return bReturn;
}

void CPVRManager::CloseStream(void)
{
  CPVRChannelPtr channel(m_addons->GetPlayingChannel());
  if (channel)
  {
    UpdateLastWatched(channel);

    // store channel settings
    g_application.SaveFileState();
  }

  m_addons->CloseStream();

  CSingleLock lock(m_critSection);
  m_isChannelPreview = false;
  m_currentFile.reset();
}

void CPVRManager::UpdateCurrentChannel(void)
{
  CSingleLock lock(m_critSection);

  CPVRChannelPtr playingChannel(GetCurrentChannel());
  if (m_currentFile &&
      playingChannel &&
      !IsPlayingChannel(m_currentFile->GetPVRChannelInfoTag()))
  {
    m_currentFile.reset(new CFileItem(playingChannel));
    UpdateItem(*m_currentFile);
    m_isChannelPreview = false;
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
    return false;

  if (!item.IsPVRChannel())
  {
    CLog::Log(LOGERROR, "CPVRManager - %s - no channel tag provided", __FUNCTION__);
    return false;
  }

  CSingleLock lock(m_critSection);
  if (!m_currentFile || !m_currentFile->GetPVRChannelInfoTag() || !item.GetPVRChannelInfoTag() ||
      *m_currentFile->GetPVRChannelInfoTag() == *item.GetPVRChannelInfoTag())
    return false;

  g_application.SetCurrentFileItem(*m_currentFile);
  g_infoManager.SetCurrentItem(m_currentFile);

  CPVRChannelPtr channelTag(item.GetPVRChannelInfoTag());
  CEpgInfoTagPtr epgTagNow(channelTag->GetEPGNow());

  if (channelTag->IsRadio())
  {
    CMusicInfoTag* musictag = item.GetMusicInfoTag();
    if (musictag)
    {
      musictag->SetTitle(epgTagNow ?
          epgTagNow->Title() :
          CServiceBroker::GetSettings().GetBool(CSettings::SETTING_EPG_HIDENOINFOAVAILABLE) ?
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
          CServiceBroker::GetSettings().GetBool(CSettings::SETTING_EPG_HIDENOINFOAVAILABLE) ?
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
  CFileItemPtr previousFile;
  {
    CSingleLock lock(m_critSection);
    if (m_bIsSwitchingChannels)
    {
      CLog::Log(LOGDEBUG, "PVRManager - %s - can't switch to channel '%s'. waiting for the previous switch to complete",
          __FUNCTION__, channel->ChannelName().c_str());
      return false;
    }

    if (bPreview)
    {
      if (!g_infoManager.GetShowInfo() &&
          CServiceBroker::GetSettings().GetInt(CSettings::SETTING_PVRPLAYBACK_CHANNELENTRYTIMEOUT) == 0)
      {
        // no need to do anything
        return true;
      }

      m_currentFile.reset(new CFileItem(channel));

      if (IsPlayingChannel(channel))
        m_isChannelPreview = false;
      else
        m_isChannelPreview = true;

      return true;
    }

    m_bIsSwitchingChannels = true;

    CLog::Log(LOGDEBUG, "PVRManager - %s - switching to channel '%s'", __FUNCTION__, channel->ChannelName().c_str());

    previousFile = std::move(m_currentFile);
  }

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
    m_currentFile.reset(new CFileItem(channel));
    m_bIsSwitchingChannels = false;

    CLog::Log(LOGNOTICE, "PVRManager - %s - switched to channel '%s'", __FUNCTION__, channel->ChannelName().c_str());
  }

  // announce OnStop
  if (previousFile)
  {
    CVariant data(CVariant::VariantTypeObject);
    data["end"] = true;
    ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::Player, "xbmc", "OnStop", previousFile, data);
  }

  // announce OnPlay
  if (m_currentFile)
  {
    CVariant param;
    param["player"]["speed"] = 1;
    param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
    ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::Player, "xbmc", "OnPlay", m_currentFile, param);
  }

  return bSwitched;
}

void CPVRManager::SetChannelPreview(bool preview)
{
  m_isChannelPreview = preview;
}

bool CPVRManager::IsChannelPreview() const
{
  return m_isChannelPreview;
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

bool CPVRManager::IsRecording(void) const
{
  return IsStarted() && m_timers ? m_timers->IsRecording() : false;
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

            CDateTimeSpan diff(start - now);
            diff -= prestart;
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
          dailywakeuptime.SetFromDBTime(CServiceBroker::GetSettings().GetString(CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME));
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
        bReturn = HELPERS::DialogResponse::YES == 
          HELPERS::ShowYesNoDialogText(CVariant{19685}, // "Confirm shutdown"
                                       CVariant{text},
                                       CVariant{222}, // "Shutdown anyway",
                                       CVariant{19696}, // "Cancel"
                                       10000); // timeout value before closing
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
    0, 0, CServiceBroker::GetSettings().GetInt(CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME), 0);
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
  return IsStarted() && m_addons->IsPlayingTV();
}

bool CPVRManager::IsPlayingRadio(void) const
{
  return IsStarted() && m_addons->IsPlayingRadio();
}

bool CPVRManager::IsPlayingRecording(void) const
{
  return IsStarted() && m_addons->IsPlayingRecording();
}

void CPVRManager::SearchMissingChannelIcons(void)
{
  if (IsStarted() && m_channelGroups)
    m_channelGroups->SearchMissingChannelIcons();
}

void CPVRManager::TriggerEpgsCreate(void)
{
  m_pendingUpdates.AppendJob(new CPVREpgsCreateJob());
}

void CPVRManager::TriggerRecordingsUpdate(void)
{
  m_pendingUpdates.AppendJob(new CPVRRecordingsUpdateJob());
}

void CPVRManager::TriggerTimersUpdate(void)
{
  m_pendingUpdates.AppendJob(new CPVRTimersUpdateJob());
}

void CPVRManager::TriggerChannelsUpdate(void)
{
  m_pendingUpdates.AppendJob(new CPVRChannelsUpdateJob());
}

void CPVRManager::TriggerChannelGroupsUpdate(void)
{
  m_pendingUpdates.AppendJob(new CPVRChannelGroupsUpdateJob());
}

void CPVRManager::TriggerSearchMissingChannelIcons(void)
{
  if (IsStarted())
    CJobManager::GetInstance().AddJob(new CPVRSearchMissingChannelIconsJob(), NULL);
}

void CPVRManager::ConnectionStateChange(CPVRClient *client, std::string connectString, PVR_CONNECTION_STATE state, std::string message)
{
  if (IsStarted())
    CJobManager::GetInstance().AddJob(new CPVRClientConnectionJob(client, connectString, state, message), NULL);
}

bool CPVRManager::CreateChannelEpgs(void)
{
  if (EpgsCreated())
    return true;

  bool bEpgsCreated = m_channelGroups->CreateChannelEpgs();

  CSingleLock lock(m_critSection);
  m_bEpgsCreated = bEpgsCreated;
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

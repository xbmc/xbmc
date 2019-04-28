/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRManager.h"

#include <utility>

#include "ServiceBroker.h"
#include "addons/PVRClient.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/Settings.h"
#include "threads/SystemClock.h"
#include "threads/Timer.h"
#include "utils/JobManager.h"
#include "utils/Stopwatch.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include "pvr/PVRDatabase.h"
#include "pvr/PVRGUIActions.h"
#include "pvr/PVRGUIInfo.h"
#include "pvr/PVRJobs.h"
#include "pvr/PVRGUIProgressHandler.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupInternal.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/recordings/PVRRecordingsPath.h"
#include "pvr/timers/PVRTimers.h"

using namespace PVR;
using namespace KODI::MESSAGING;

CPVRManagerJobQueue::CPVRManagerJobQueue()
: m_triggerEvent(false)
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
    m_channelGroups(new CPVRChannelGroupsContainer),
    m_recordings(new CPVRRecordings),
    m_timers(new CPVRTimers),
    m_addons(new CPVRClients),
    m_guiInfo(new CPVRGUIInfo),
    m_guiActions(new CPVRGUIActions),
    m_database(new CPVRDatabase),
    m_parentalTimer(new CStopWatch),
    m_settings({
      CSettings::SETTING_PVRPLAYBACK_DELAYMARKLASTWATCHED,
      CSettings::SETTING_PVRPOWERMANAGEMENT_ENABLED,
      CSettings::SETTING_PVRPOWERMANAGEMENT_SETWAKEUPCMD,
      CSettings::SETTING_PVRPARENTAL_ENABLED,
      CSettings::SETTING_PVRPARENTAL_DURATION
    })
{
  CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this);
  m_actionListener.Init(*this);

  CLog::LogFC(LOGDEBUG, LOGPVR, "PVR Manager instance created");
}

CPVRManager::~CPVRManager(void)
{
  m_actionListener.Deinit(*this);
  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);

  CLog::LogFC(LOGDEBUG, LOGPVR, "PVR Manager instance destroyed");
}

void CPVRManager::Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (!IsStarted())
    return;

  if ((flag & (ANNOUNCEMENT::GUI)))
  {
    if (strcmp(message, "OnScreensaverActivated") == 0)
      m_addons->OnPowerSavingActivated();
    else if (strcmp(message, "OnScreensaverDeactivated") == 0)
      m_addons->OnPowerSavingDeactivated();
  }
}

CPVRDatabasePtr CPVRManager::GetTVDatabase(void) const
{
  CSingleLock lock(m_critSection);
  if (!m_database || !m_database->IsOpen())
    CLog::LogF(LOGERROR, "Failed to open the PVR database");

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

CPVRClientPtr CPVRManager::GetClient(const CFileItem &item) const
{
  int iClientID = PVR_INVALID_CLIENT_ID;

  if (item.HasPVRChannelInfoTag())
    iClientID = item.GetPVRChannelInfoTag()->ClientID();
  else if (item.HasPVRRecordingInfoTag())
    iClientID = item.GetPVRRecordingInfoTag()->m_iClientId;
  else if (item.HasPVRTimerInfoTag())
    iClientID = item.GetPVRTimerInfoTag()->m_iClientId;
  else if (item.HasEPGInfoTag())
    iClientID = item.GetEPGInfoTag()->ClientID();
  else if (URIUtils::IsPVRChannel(item.GetPath()))
  {
    const std::shared_ptr<CFileItem> channelItem = m_channelGroups->GetByPath(item.GetPath());
    if (channelItem)
      iClientID = channelItem->GetPVRChannelInfoTag()->ClientID();
  }
  else if (URIUtils::IsPVRRecording(item.GetPath()))
  {
    const std::shared_ptr<CFileItem> recordingItem = m_recordings->GetByPath(item.GetPath());
    if (recordingItem)
      iClientID = recordingItem->GetPVRRecordingInfoTag()->ClientID();
  }
  return GetClient(iClientID);
}

CPVRClientPtr CPVRManager::GetClient(int iClientId) const
{
  CPVRClientPtr client;
  if (iClientId != PVR_INVALID_CLIENT_ID)
    m_addons->GetCreatedClient(iClientId, client);

  return client;
}

CPVRGUIActionsPtr CPVRManager::GUIActions(void) const
{
  // note: m_guiActions is const (only set/reset in ctor/dtor). no need for a lock here.
  return m_guiActions;
}

CPVREpgContainer& CPVRManager::EpgContainer()
{
  // note: m_epgContainer is const (only set/reset in ctor/dtor). no need for a lock here.
  return m_epgContainer;
}

void CPVRManager::Clear(void)
{
  m_pendingUpdates.Clear();
  m_epgContainer.Clear();

  CSingleLock lock(m_critSection);

  m_guiInfo.reset();
  m_timers.reset();
  m_recordings.reset();
  m_channelGroups.reset();
  m_parentalTimer.reset();
  m_database.reset();

  m_bEpgsCreated = false;
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
  // initial check for enabled addons
  // if at least one pvr addon is enabled, PVRManager start up
  CJobManager::GetInstance().AddJob(new CPVRStartupJob(), nullptr);
}

void CPVRManager::Start()
{
  CSingleLock initLock(m_startStopMutex);

  // Prevent concurrent starts
  if (IsInitialising())
    return;

  // Note: Stop() must not be called while holding pvr manager's mutex. Stop() calls
  // StopThread() which can deadlock if the worker thread tries to acquire pvr manager's
  // lock while StopThread() is waiting for the worker to exit. Thus, we introduce another
  // lock here (m_startStopMutex), which only gets hold while starting/restarting pvr manager.
  Stop();

  CSingleLock lock(m_critSection);

  if (!m_addons->HasCreatedClients())
    return;

  CLog::Log(LOGNOTICE, "PVR Manager: Starting");
  SetState(ManagerStateStarting);

  /* create the pvrmanager thread, which will ensure that all data will be loaded */
  Create();
  SetPriority(-1);
}

void CPVRManager::Stop(void)
{
  CSingleLock initLock(m_startStopMutex);

  // Prevent concurrent stops
  if (IsStopped())
    return;

  /* stop playback if needed */
  if (IsPlaying())
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "Stopping PVR playback");
    CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_STOP);
  }

  CLog::Log(LOGNOTICE, "PVR Manager: Stopping");
  SetState(ManagerStateStopping);

  m_addons->Stop();
  m_pendingUpdates.Stop();
  m_epgContainer.Stop();
  m_guiInfo->Stop();

  StopThread();

  SetState(ManagerStateInterrupted);

  CSingleLock lock(m_critSection);

  UnloadComponents();
  m_database->Close();

  ResetProperties();

  CLog::Log(LOGNOTICE, "PVR Manager: Stopped");
  SetState(ManagerStateStopped);
}

void CPVRManager::Unload()
{
  // stop pvr manager thread and clear all pvr data
  Stop();
  Clear();
}

void CPVRManager::Deinit()
{
  SetWakeupCommand();
  Unload();

  // release addons
  m_addons.reset();
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
        event = PVREvent::ManagerError;
        break;
      case ManagerStateStopped:
        event = PVREvent::ManagerStopped;
        observableMsg = ObservableMessageManagerStopped;
        break;
      case ManagerStateStarting:
        event = PVREvent::ManagerStarting;
        break;
      case ManagerStateStopping:
        event = PVREvent::ManagerStopped;
        break;
      case ManagerStateInterrupted:
        event = PVREvent::ManagerInterrupted;
        break;
      case ManagerStateStarted:
        event = PVREvent::ManagerStarted;
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
  m_addons->Continue();
  m_database->Open();

  /* load the pvr data from the db and clients if it's not already loaded */
  XbmcThreads::EndTime progressTimeout(30000); // 30 secs
  CPVRGUIProgressHandler* progressHandler = new CPVRGUIProgressHandler(g_localizeStrings.Get(19235)); // PVR manager is starting up
  while (!LoadComponents(progressHandler) && IsInitialising())
  {
    CLog::Log(LOGWARNING, "PVR Manager failed to load data, retrying");
    Sleep(1000);

    if (progressHandler && progressTimeout.IsTimePast())
    {
      progressHandler->DestroyProgress();
      progressHandler = nullptr; // no delete, instance is deleting itself
    }
  }

  if (progressHandler)
  {
    progressHandler->DestroyProgress();
    progressHandler = nullptr; // no delete, instance is deleting itself
  }

  if (!IsInitialising())
  {
    CLog::Log(LOGNOTICE, "PVR Manager: Start aborted");
    return;
  }

  m_guiInfo->Start();
  m_epgContainer.Start(true);
  m_pendingUpdates.Start();

  SetState(ManagerStateStarted);
  CLog::Log(LOGNOTICE, "PVR Manager: Started");

  /* main loop */
  CLog::LogFC(LOGDEBUG, LOGPVR, "PVR Manager entering main loop");

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

      /* try to play channel on startup */
      TriggerPlayChannelOnStartup();
    }
    /* execute the next pending jobs if there are any */
    try
    {
      m_pendingUpdates.ExecutePendingJobs();
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "An error occured while trying to execute the last PVR update job, trying to recover");
      bRestart = true;
    }

    if (IsStarted() && !bRestart)
      m_pendingUpdates.WaitForJobs(1000);
  }

  CLog::LogFC(LOGDEBUG, LOGPVR, "PVR Manager leaving main loop");
}

bool CPVRManager::SetWakeupCommand(void)
{
#if !defined(TARGET_DARWIN_EMBEDDED) && !defined(TARGET_WINDOWS_STORE)
  if (!m_settings.GetBoolValue(CSettings::SETTING_PVRPOWERMANAGEMENT_ENABLED))
    return false;

  const std::string strWakeupCommand(m_settings.GetStringValue(CSettings::SETTING_PVRPOWERMANAGEMENT_SETWAKEUPCMD));
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
        CLog::LogF(LOGERROR, "PVR Manager failed to execute wakeup command '%s': %s (%d)", strExecCommand.c_str(), strerror(iReturn), iReturn);

      return iReturn == 0;
    }
  }
#endif
  return false;
}

void CPVRManager::OnSleep()
{
  SetWakeupCommand();

  m_addons->OnSystemSleep();
}

void CPVRManager::OnWake()
{
  m_addons->OnSystemWake();

  /* start job to search for missing channel icons */
  TriggerSearchMissingChannelIcons();

  /* trigger PVR data updates */
  TriggerChannelGroupsUpdate();
  TriggerChannelsUpdate();
  TriggerRecordingsUpdate();
  TriggerEpgsCreate();
  TriggerTimersUpdate();
}

bool CPVRManager::LoadComponents(CPVRGUIProgressHandler* progressHandler)
{
  /* load at least one client */
  while (IsInitialising() && m_addons && !m_addons->HasCreatedClients())
    Sleep(50);

  if (!IsInitialising() || !m_addons->HasCreatedClients())
    return false;

  CLog::LogFC(LOGDEBUG, LOGPVR, "PVR Manager found active clients. Continuing startup");

  /* load all channels and groups */
  if (progressHandler)
    progressHandler->UpdateProgress(g_localizeStrings.Get(19236), 0); // Loading channels from clients

  if (!m_channelGroups->Load() || !IsInitialising())
    return false;

  SetChanged();
  NotifyObservers(ObservableMessageChannelGroupsLoaded);

  /* get timers from the backends */
  if (progressHandler)
    progressHandler->UpdateProgress(g_localizeStrings.Get(19237), 50); // Loading timers from clients

  m_timers->Load();

  /* get recordings from the backend */
  if (progressHandler)
    progressHandler->UpdateProgress(g_localizeStrings.Get(19238), 75); // Loading recordings from clients

  m_recordings->Load();

  if (!IsInitialising())
    return false;

  /* start the other pvr related update threads */
  if (progressHandler)
    progressHandler->UpdateProgress(g_localizeStrings.Get(19239), 85); // Starting background threads

  return true;
}

void CPVRManager::UnloadComponents()
{
  m_recordings->Unload();
  m_timers->Unload();
  m_channelGroups->Unload();
}

void CPVRManager::TriggerPlayChannelOnStartup(void)
{
  if (IsStarted())
    CJobManager::GetInstance().AddJob(new CPVRPlayChannelOnStartupJob(), nullptr);
}

bool CPVRManager::IsPlaying(void) const
{
  return IsStarted() && (m_playingChannel || m_playingRecording || m_playingEpgTag);
}

bool CPVRManager::IsPlayingChannel(const CPVRChannelPtr &channel) const
{
  bool bReturn(false);

  if (channel && IsStarted())
  {
    CPVRChannelPtr current(GetPlayingChannel());
    if (current && *current == *channel)
      bReturn = true;
  }

  return bReturn;
}

bool CPVRManager::IsPlayingEncryptedChannel(void) const
{
  return IsStarted() && m_playingChannel && m_playingChannel->IsEncrypted();
}

bool CPVRManager::IsPlayingRecording(const CPVRRecordingPtr &recording) const
{
  bool bReturn(false);

  if (recording && IsStarted())
  {
    CPVRRecordingPtr current(GetPlayingRecording());
    if (current && *current == *recording)
      bReturn = true;
  }

  return bReturn;
}

bool CPVRManager::IsPlayingEpgTag(const CPVREpgInfoTagPtr &epgTag) const
{
  bool bReturn(false);

  if (epgTag && IsStarted())
  {
    CPVREpgInfoTagPtr current(GetPlayingEpgTag());
    if (current && *current == *epgTag)
      bReturn = true;
  }

  return bReturn;
}

bool CPVRManager::MatchPlayingChannel(int iClientID, int iUniqueChannelID) const
{
  if (m_playingChannel)
    return m_playingChannel->ClientID() == iClientID && m_playingChannel->UniqueID() == iUniqueChannelID;

  return false;
}

CPVRChannelPtr CPVRManager::GetPlayingChannel(void) const
{
  return m_playingChannel;
}

CPVRRecordingPtr CPVRManager::GetPlayingRecording(void) const
{
  return m_playingRecording;
}

CPVREpgInfoTagPtr CPVRManager::GetPlayingEpgTag(void) const
{
  return m_playingEpgTag;
}

std::string CPVRManager::GetPlayingClientName(void) const
{
  return m_strPlayingClientName;
}

int CPVRManager::GetPlayingClientID(void) const
{
  return m_playingClientId;
}

bool CPVRManager::IsRecordingOnPlayingChannel(void) const
{
  const CPVRChannelPtr currentChannel = GetPlayingChannel();
  return currentChannel && CServiceBroker::GetPVRManager().Timers()->IsRecordingOnChannel(*currentChannel);
}

bool CPVRManager::CanRecordOnPlayingChannel(void) const
{
  const CPVRChannelPtr currentChannel = GetPlayingChannel();
  return currentChannel && currentChannel->CanRecord();
}

void CPVRManager::RestartParentalTimer()
{
  if (m_parentalTimer)
    m_parentalTimer->StartZero();
}

bool CPVRManager::IsParentalLocked(const std::shared_ptr<CPVREpgInfoTag>& epgTag) const
{
  return m_channelGroups &&
         epgTag &&
         IsCurrentlyParentalLocked(m_channelGroups->GetByUniqueID(epgTag->UniqueChannelID(), epgTag->ClientID()),
                                   epgTag->IsParentalLocked());
}

bool CPVRManager::IsParentalLocked(const std::shared_ptr<CPVRChannel>& channel) const
{
  return channel &&
         IsCurrentlyParentalLocked(channel, channel->IsLocked());
}

bool CPVRManager::IsCurrentlyParentalLocked(const std::shared_ptr<CPVRChannel>& channel, bool bGenerallyLocked) const
{
  bool bReturn = false;

  if (!channel || !bGenerallyLocked)
    return bReturn;

  const std::shared_ptr<CPVRChannel> currentChannel = GetPlayingChannel();

  if (// if channel in question is currently playing it must be currently unlocked.
      (!currentChannel || channel != currentChannel) &&
      // parental control enabled
      m_settings.GetBoolValue(CSettings::SETTING_PVRPARENTAL_ENABLED))
  {
    float parentalDurationMs = m_settings.GetIntValue(CSettings::SETTING_PVRPARENTAL_DURATION) * 1000.0f;
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

CPVRChannelGroupPtr CPVRManager::GetPlayingGroup(bool bRadio /* = false */) const
{
  if (m_channelGroups)
    return m_channelGroups->GetSelectedGroup(bRadio);

  return CPVRChannelGroupPtr();
}

class CPVRManager::CLastWatchedUpdateTimer : public CTimer, private ITimerCallback
{
public:
  explicit CLastWatchedUpdateTimer(CPVRManager& pvrMgr,
                                   const std::shared_ptr<CPVRChannel>& channel,
                                   const CDateTime& time)
  : CTimer(this),
    m_pvrMgr(pvrMgr),
    m_channel(channel),
    m_time(time)
  {
  }

  // ITimerCallback implementation
  void OnTimeout() override
  {
    m_pvrMgr.UpdateLastWatched(m_channel, m_time);
  }

private:
  CLastWatchedUpdateTimer() = delete;

  CPVRManager& m_pvrMgr;
  const std::shared_ptr<CPVRChannel> m_channel;
  const CDateTime m_time;
};

void CPVRManager::OnPlaybackStarted(const CFileItemPtr item)
{
  m_playingChannel.reset();
  m_playingRecording.reset();
  m_playingEpgTag.reset();
  m_playingClientId = -1;
  m_strPlayingClientName.clear();

  if (item->HasPVRChannelInfoTag())
  {
    const CPVRChannelPtr channel(item->GetPVRChannelInfoTag());

    m_playingChannel = channel;
    m_playingClientId = m_playingChannel->ClientID();

    SetPlayingGroup(channel);

    int iLastWatchedDelay = m_settings.GetIntValue(CSettings::SETTING_PVRPLAYBACK_DELAYMARKLASTWATCHED) * 1000;
    if (iLastWatchedDelay > 0)
    {
      // Insert new / replace existing last watched update timer
      if (m_lastWatchedUpdateTimer)
        m_lastWatchedUpdateTimer->Stop(true);

      m_lastWatchedUpdateTimer.reset(new CLastWatchedUpdateTimer(*this, channel, CDateTime::GetUTCDateTime()));
      m_lastWatchedUpdateTimer->Start(iLastWatchedDelay);
    }
    else
    {
      // Store last watched timestamp immediately
      UpdateLastWatched(channel, CDateTime::GetUTCDateTime());
    }
  }
  else if (item->HasPVRRecordingInfoTag())
  {
    m_playingRecording = item->GetPVRRecordingInfoTag();
    m_playingClientId = m_playingRecording->m_iClientId;
  }
  else if (item->HasEPGInfoTag())
  {
    m_playingEpgTag = item->GetEPGInfoTag();
    m_playingClientId = m_playingEpgTag->ClientID();
  }

  if (m_playingClientId != -1)
  {
    const CPVRClientPtr client = GetClient(m_playingClientId);
    if (client)
      m_strPlayingClientName = client->GetFriendlyName();
  }

  m_guiActions->OnPlaybackStarted(item);
  m_epgContainer.OnPlaybackStarted(item);
}

void CPVRManager::OnPlaybackStopped(const CFileItemPtr item)
{
  // Playback ended due to user interaction

  if (item->HasPVRChannelInfoTag() && item->GetPVRChannelInfoTag() == m_playingChannel)
  {
    bool bUpdateLastWatched = true;

    if (m_lastWatchedUpdateTimer)
    {
      if (m_lastWatchedUpdateTimer->IsRunning())
      {
        // If last watched timer is still running, cancel it. Channel was not watched long enough to store the value.
        m_lastWatchedUpdateTimer->Stop(true);
        bUpdateLastWatched = false;
      }
      m_lastWatchedUpdateTimer.reset();
    }

    if (bUpdateLastWatched)
    {
      // If last watched timer is not running (any more), channel was watched long enough to store the value.
      UpdateLastWatched(m_playingChannel, CDateTime::GetUTCDateTime());
    }

    SetChanged();
    NotifyObservers(ObservableMessageChannelPlaybackStopped);

    m_playingChannel.reset();
    m_playingClientId = -1;
    m_strPlayingClientName.clear();
  }
  else if (item->HasPVRRecordingInfoTag() && item->GetPVRRecordingInfoTag() == m_playingRecording)
  {
    m_playingRecording.reset();
    m_playingClientId = -1;
    m_strPlayingClientName.clear();
  }
  else if (item->HasEPGInfoTag() && item->GetEPGInfoTag() == m_playingEpgTag)
  {
    m_playingEpgTag.reset();
    m_playingClientId = -1;
    m_strPlayingClientName.clear();
  }

  m_guiActions->OnPlaybackStopped(item);
  m_epgContainer.OnPlaybackStopped(item);
}

void CPVRManager::OnPlaybackEnded(const CFileItemPtr item)
{
  // Playback ended, but not due to user interaction
  OnPlaybackStopped(item);
}

bool CPVRManager::IsRecording(void) const
{
  return IsStarted() && m_timers ? m_timers->IsRecording() : false;
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
  return IsStarted() && m_playingChannel && !m_playingChannel->IsRadio();
}

bool CPVRManager::IsPlayingRadio(void) const
{
  return IsStarted() && m_playingChannel && m_playingChannel->IsRadio();
}

bool CPVRManager::IsPlayingRecording(void) const
{
  return IsStarted() && m_playingRecording;
}

bool CPVRManager::IsPlayingEpgTag(void) const
{
  return IsStarted() && m_playingEpgTag;
}

void CPVRManager::SearchMissingChannelIcons(void)
{
  if (IsStarted() && m_channelGroups)
    m_channelGroups->SearchMissingChannelIcons();
}

bool CPVRManager::FillStreamFileItem(CFileItem &fileItem)
{
  const CPVRClientPtr client = GetClient(fileItem);
  if (client)
  {
    if (fileItem.IsPVRChannel())
      return client->FillChannelStreamFileItem(fileItem) == PVR_ERROR_NO_ERROR;
    else if (fileItem.IsPVRRecording())
      return client->FillRecordingStreamFileItem(fileItem) == PVR_ERROR_NO_ERROR;
    else if (fileItem.IsEPG())
      return client->FillEpgTagStreamFileItem(fileItem) == PVR_ERROR_NO_ERROR;
  }
  return false;
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
  // Note: No check for started pvr manager here. This method is intended to get called even before the mgr is started.
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

void CPVRManager::UpdateLastWatched(const CPVRChannelPtr &channel, const CDateTime& time)
{
  time_t iTime;
  time.GetAsTime(iTime);

  channel->SetLastWatched(iTime);

  // update last watched timestamp for group
  CPVRChannelGroupPtr group(GetPlayingGroup(channel->IsRadio()));
  group->SetLastWatched(iTime);

  /* update last played group */
  m_channelGroups->SetLastPlayedGroup(group);
}

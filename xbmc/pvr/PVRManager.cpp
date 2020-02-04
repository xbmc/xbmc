/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRManager.h"

#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroupInternal.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/guilib/PVRGUIActions.h"
#include "pvr/guilib/PVRGUIChannelIconUpdater.h"
#include "pvr/guilib/PVRGUIProgressHandler.h"
#include "pvr/guilib/guiinfo/PVRGUIInfo.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "settings/Settings.h"
#include "utils/JobManager.h"
#include "utils/Stopwatch.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace PVR;
using namespace KODI::MESSAGING;

namespace PVR
{
template<typename F>
class CPVRLambdaJob : public CJob
{
public:
  CPVRLambdaJob() = delete;
  CPVRLambdaJob(const std::string& type, F&& f) : m_type(type), m_f(std::forward<F>(f)) {}

  bool DoWork() override
  {
    m_f();
    return true;
  }

  const char* GetType() const override
  {
    return m_type.c_str();
  }

private:
  std::string m_type;
  F m_f;
};

class CPVRManagerJobQueue
{
public:
  CPVRManagerJobQueue() : m_triggerEvent(false) {}

  void Start();
  void Stop();
  void Clear();

  template<typename F>
  void Append(const std::string& type, F&& f)
  {
    AppendJob(new CPVRLambdaJob<F>(type, std::forward<F>(f)));
  }

  void ExecutePendingJobs();

  bool WaitForJobs(unsigned int milliSeconds)
  {
    return m_triggerEvent.WaitMSec(milliSeconds);
  }


private:
  void AppendJob(CJob* job);

  CCriticalSection m_critSection;
  CEvent m_triggerEvent;
  std::vector<CJob*> m_pendingUpdates;
  bool m_bStopped = true;
};
} // namespace PVR

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
  for (CJob* updateJob : m_pendingUpdates)
    delete updateJob;

  m_pendingUpdates.clear();
  m_triggerEvent.Set();
}

void CPVRManagerJobQueue::AppendJob(CJob* job)
{
  CSingleLock lock(m_critSection);

  // check for another pending job of given type...
  for (CJob* updateJob : m_pendingUpdates)
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
  std::vector<CJob*> pendingUpdates;

  {
    CSingleLock lock(m_critSection);

    if (m_bStopped)
      return;

    pendingUpdates = std::move(m_pendingUpdates);
    m_triggerEvent.Reset();
  }

  CJob* job = nullptr;
  while (!pendingUpdates.empty())
  {
    job = pendingUpdates.front();
    pendingUpdates.erase(pendingUpdates.begin());

    job->DoWork();
    delete job;
  }
}

CPVRManager::CPVRManager() :
    CThread("PVRManager"),
    m_channelGroups(new CPVRChannelGroupsContainer),
    m_recordings(new CPVRRecordings),
    m_timers(new CPVRTimers),
    m_addons(new CPVRClients),
    m_guiInfo(new CPVRGUIInfo),
    m_guiActions(new CPVRGUIActions),
    m_pendingUpdates(new CPVRManagerJobQueue),
    m_database(new CPVRDatabase),
    m_parentalTimer(new CStopWatch),
    m_playbackState(new CPVRPlaybackState),
    m_settings({
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

CPVRManager::~CPVRManager()
{
  m_actionListener.Deinit(*this);
  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);

  CLog::LogFC(LOGDEBUG, LOGPVR, "PVR Manager instance destroyed");
}

void CPVRManager::Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char* sender, const char* message, const CVariant& data)
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

std::shared_ptr<CPVRDatabase> CPVRManager::GetTVDatabase() const
{
  CSingleLock lock(m_critSection);
  if (!m_database || !m_database->IsOpen())
    CLog::LogF(LOGERROR, "Failed to open the PVR database");

  return m_database;
}

std::shared_ptr<CPVRChannelGroupsContainer> CPVRManager::ChannelGroups() const
{
  CSingleLock lock(m_critSection);
  return m_channelGroups;
}

std::shared_ptr<CPVRRecordings> CPVRManager::Recordings() const
{
  CSingleLock lock(m_critSection);
  return m_recordings;
}

std::shared_ptr<CPVRTimers> CPVRManager::Timers() const
{
  CSingleLock lock(m_critSection);
  return m_timers;
}

std::shared_ptr<CPVRClients> CPVRManager::Clients() const
{
  // note: m_addons is const (only set/reset in ctor/dtor). no need for a lock here.
  return m_addons;
}

std::shared_ptr<CPVRClient> CPVRManager::GetClient(const CFileItem& item) const
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
    const std::shared_ptr<CPVRChannel> channel = m_channelGroups->GetByPath(item.GetPath());
    if (channel)
      iClientID = channel->ClientID();
  }
  else if (URIUtils::IsPVRRecording(item.GetPath()))
  {
    const std::shared_ptr<CPVRRecording> recording = m_recordings->GetByPath(item.GetPath());
    if (recording)
      iClientID = recording->ClientID();
  }
  return GetClient(iClientID);
}

std::shared_ptr<CPVRClient> CPVRManager::GetClient(int iClientId) const
{
  std::shared_ptr<CPVRClient> client;
  if (iClientId != PVR_INVALID_CLIENT_ID)
    m_addons->GetCreatedClient(iClientId, client);

  return client;
}

std::shared_ptr<CPVRGUIActions> CPVRManager::GUIActions() const
{
  // note: m_guiActions is const (only set/reset in ctor/dtor). no need for a lock here.
  return m_guiActions;
}

std::shared_ptr<CPVRPlaybackState> CPVRManager::PlaybackState() const
{
  CSingleLock lock(m_critSection);
  return m_playbackState;
}

CPVREpgContainer& CPVRManager::EpgContainer()
{
  // note: m_epgContainer is const (only set/reset in ctor/dtor). no need for a lock here.
  return m_epgContainer;
}

void CPVRManager::Clear()
{
  m_pendingUpdates->Clear();
  m_epgContainer.Clear();

  CSingleLock lock(m_critSection);

  m_guiInfo.reset();
  m_timers.reset();
  m_recordings.reset();
  m_channelGroups.reset();
  m_parentalTimer.reset();
  m_playbackState.reset();
  m_database.reset();

  m_bEpgsCreated = false;
}

void CPVRManager::ResetProperties()
{
  CSingleLock lock(m_critSection);
  Clear();

  m_database.reset(new CPVRDatabase);
  m_channelGroups.reset(new CPVRChannelGroupsContainer);
  m_recordings.reset(new CPVRRecordings);
  m_timers.reset(new CPVRTimers);
  m_guiInfo.reset(new CPVRGUIInfo);
  m_parentalTimer.reset(new CStopWatch);
  m_playbackState.reset(new CPVRPlaybackState);
}

void CPVRManager::Init()
{
  // initial check for enabled addons
  // if at least one pvr addon is enabled, PVRManager start up
  CJobManager::GetInstance().Submit([this] {
    Clients()->Start();
    return true;
  });
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

  if (!m_addons->HasCreatedClients())
    return;

  CLog::Log(LOGNOTICE, "PVR Manager: Starting");
  SetState(ManagerStateStarting);

  /* create the pvrmanager thread, which will ensure that all data will be loaded */
  Create();
  SetPriority(-1);
}

void CPVRManager::Stop()
{
  CSingleLock initLock(m_startStopMutex);

  // Prevent concurrent stops
  if (IsStopped())
    return;

  /* stop playback if needed */
  if (m_playbackState->IsPlaying())
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "Stopping PVR playback");
    CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_STOP);
  }

  CLog::Log(LOGNOTICE, "PVR Manager: Stopping");
  SetState(ManagerStateStopping);

  m_addons->Stop();
  m_pendingUpdates->Stop();
  m_epgContainer.Stop();
  m_guiInfo->Stop();

  StopThread();

  SetState(ManagerStateInterrupted);

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

CPVRManager::ManagerState CPVRManager::GetState() const
{
  CSingleLock lock(m_managerStateMutex);
  return m_managerState;
}

void CPVRManager::SetState(CPVRManager::ManagerState state)
{
  {
    CSingleLock lock(m_managerStateMutex);
    if (m_managerState == state)
      return;

    m_managerState = state;
  }

  PVREvent event;
  switch (state)
  {
    case ManagerStateError:
      event = PVREvent::ManagerError;
      break;
    case ManagerStateStopped:
      event = PVREvent::ManagerStopped;
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

  PublishEvent(event);
}

void CPVRManager::PublishEvent(PVREvent event)
{
  m_events.Publish(event);
}

void CPVRManager::Process()
{
  m_addons->Continue();
  m_database->Open();

  /* load the pvr data from the db and clients if it's not already loaded */
  XbmcThreads::EndTime progressTimeout(30000); // 30 secs
  CPVRGUIProgressHandler* progressHandler = new CPVRGUIProgressHandler(g_localizeStrings.Get(19235)); // PVR manager is starting up
  while (!LoadComponents(progressHandler) && IsInitialising())
  {
    CLog::Log(LOGWARNING, "PVR Manager failed to load data, retrying");
    CThread::Sleep(1000);

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
  m_pendingUpdates->Start();

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
      m_pendingUpdates->ExecutePendingJobs();
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "An error occured while trying to execute the last PVR update job, trying to recover");
      bRestart = true;
    }

    if (IsStarted() && !bRestart)
      m_pendingUpdates->WaitForJobs(1000);
  }

  CLog::LogFC(LOGDEBUG, LOGPVR, "PVR Manager leaving main loop");
}

bool CPVRManager::SetWakeupCommand()
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
  PublishEvent(PVREvent::SystemSleep);

  SetWakeupCommand();

  m_addons->OnSystemSleep();
}

void CPVRManager::OnWake()
{
  m_addons->OnSystemWake();

  PublishEvent(PVREvent::SystemWake);

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
    CThread::Sleep(50);

  if (!IsInitialising() || !m_addons->HasCreatedClients())
    return false;

  CLog::LogFC(LOGDEBUG, LOGPVR, "PVR Manager found active clients. Continuing startup");

  /* load all channels and groups */
  if (progressHandler)
    progressHandler->UpdateProgress(g_localizeStrings.Get(19236), 0); // Loading channels from clients

  if (!m_channelGroups->Load() || !IsInitialising())
    return false;

  PublishEvent(PVREvent::ChannelGroupsLoaded);

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

void CPVRManager::TriggerPlayChannelOnStartup()
{
  if (IsStarted())
  {
    CJobManager::GetInstance().Submit([this] {
      return GUIActions()->PlayChannelOnStartup();
    });
  }
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

  const std::shared_ptr<CPVRChannel> currentChannel = m_playbackState->GetPlayingChannel();

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

void CPVRManager::OnPlaybackStarted(const CFileItemPtr item)
{
  m_playbackState->OnPlaybackStarted(item);
  m_guiActions->OnPlaybackStarted(item);
  m_epgContainer.OnPlaybackStarted();
}

void CPVRManager::OnPlaybackStopped(const CFileItemPtr item)
{
  // Playback ended due to user interaction
  if (m_playbackState->OnPlaybackStopped(item))
    PublishEvent(PVREvent::ChannelPlaybackStopped);

  m_guiActions->OnPlaybackStopped(item);
  m_epgContainer.OnPlaybackStopped();
}

void CPVRManager::OnPlaybackEnded(const CFileItemPtr item)
{
  // Playback ended, but not due to user interaction
  OnPlaybackStopped(item);
}

void CPVRManager::LocalizationChanged()
{
  CSingleLock lock(m_critSection);
  if (IsStarted())
  {
    static_cast<CPVRChannelGroupInternal *>(m_channelGroups->GetGroupAllRadio().get())->CheckGroupName();
    static_cast<CPVRChannelGroupInternal *>(m_channelGroups->GetGroupAllTV().get())->CheckGroupName();
  }
}

bool CPVRManager::EpgsCreated() const
{
  CSingleLock lock(m_critSection);
  return m_bEpgsCreated;
}

void CPVRManager::TriggerEpgsCreate()
{
  m_pendingUpdates->Append("pvr-create-epgs", [this]() {
    return CreateChannelEpgs();
  });
}

void CPVRManager::TriggerRecordingsUpdate()
{
  m_pendingUpdates->Append("pvr-update-recordings", [this]() {
    return Recordings()->Update();
  });
}

void CPVRManager::TriggerTimersUpdate()
{
  m_pendingUpdates->Append("pvr-update-timers", [this]() {
    return Timers()->Update();
  });
}

void CPVRManager::TriggerChannelsUpdate()
{
  m_pendingUpdates->Append("pvr-update-channels", [this]() {
    return ChannelGroups()->Update(true);
  });
}

void CPVRManager::TriggerChannelGroupsUpdate()
{
  m_pendingUpdates->Append("pvr-update-channelgroups", [this]() {
    return ChannelGroups()->Update(false);
  });
}

void CPVRManager::TriggerSearchMissingChannelIcons()
{
  if (IsStarted())
  {
    CJobManager::GetInstance().Submit([this] {
      CPVRGUIChannelIconUpdater updater({ChannelGroups()->GetGroupAllTV(), ChannelGroups()->GetGroupAllRadio()}, true);
      updater.SearchAndUpdateMissingChannelIcons();
      return true;
    });
  }
}

void CPVRManager::TriggerSearchMissingChannelIcons(const std::shared_ptr<CPVRChannelGroup>& group)
{
  if (IsStarted())
  {
    CJobManager::GetInstance().Submit([group] {
      CPVRGUIChannelIconUpdater updater({group}, false);
      updater.SearchAndUpdateMissingChannelIcons();
      return true;
    });
  }
}

void CPVRManager::ConnectionStateChange(CPVRClient* client,
                                        const std::string& connectString,
                                        PVR_CONNECTION_STATE state,
                                        const std::string& message)
{
  CJobManager::GetInstance().Submit([this, client, connectString, state, message] {
    Clients()->ConnectionStateChange(client, connectString, state, message);
    return true;
  });
}

bool CPVRManager::CreateChannelEpgs()
{
  if (EpgsCreated())
    return true;

  bool bEpgsCreated = m_channelGroups->CreateChannelEpgs();

  CSingleLock lock(m_critSection);
  m_bEpgsCreated = bEpgsCreated;
  return m_bEpgsCreated;
}

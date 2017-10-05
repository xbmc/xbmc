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

#include <utility>

#include "Application.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/LocalizeStrings.h"
#include "input/Key.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "network/Network.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "threads/SystemClock.h"
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
#include "pvr/windows/GUIWindowPVRBase.h"

using namespace PVR;
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
    m_guiActions(new CPVRGUIActions),
    m_bFirstStart(true),
    m_bEpgsCreated(false),
    m_managerState(ManagerStateStopped),
    m_settings({
      CSettings::SETTING_PVRPOWERMANAGEMENT_ENABLED,
      CSettings::SETTING_PVRPOWERMANAGEMENT_SETWAKEUPCMD,
      CSettings::SETTING_PVRPARENTAL_ENABLED,
      CSettings::SETTING_PVRPARENTAL_DURATION,
      CSettings::SETTING_EPG_HIDENOINFOAVAILABLE,
      CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME,
      CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME
    })
{
  CAnnouncementManager::GetInstance().AddAnnouncer(this);
}

CPVRManager::~CPVRManager(void)
{
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
  m_epgContainer.Stop();

  CLog::Log(LOGNOTICE, "PVRManager - stopping");

  /* stop all update threads */
  SetState(ManagerStateInterrupted);

  StopThread();

  if (m_guiInfo)
    m_guiInfo->Stop();

  /* close database */
  const CPVRDatabasePtr database(GetTVDatabase());
  if (database)
    database->Close();

  SetState(ManagerStateStopped);
}

void CPVRManager::Unload()
{
  // stop pvr manager thread and clear all pvr data
  Stop();
  Clear();

  // stop epg container thread and clear all epg data
  m_epgContainer.Stop();
  m_epgContainer.Clear();
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
  m_epgContainer.Stop();

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
  m_epgContainer.Start(true);

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
        CLog::Log(LOGERROR, "%s - failed to execute wakeup command '%s': %s (%d)", __FUNCTION__, strExecCommand.c_str(), strerror(iReturn), iReturn);

      return iReturn == 0;
    }
  }

  return false;
}

void CPVRManager::OnSleep()
{
  SetWakeupCommand();

  CServiceBroker::GetPVRManager().Clients()->OnSystemSleep();
}

void CPVRManager::OnWake()
{
  CServiceBroker::GetPVRManager().Clients()->OnSystemWake();

  /* start job to search for missing channel icons */
  TriggerSearchMissingChannelIcons();

  /* try to play channel on startup */
  TriggerPlayChannelOnStartup();

  /* trigger PVR data updates */
  TriggerChannelGroupsUpdate();
  TriggerChannelsUpdate();
  TriggerRecordingsUpdate();
  TriggerEpgsCreate();
  TriggerTimersUpdate();
}

bool CPVRManager::Load(bool bShowProgress)
{
  /* load at least one client */
  while (IsInitialising() && m_addons && !m_addons->HasCreatedClients())
    Sleep(50);

  if (!IsInitialising() || !m_addons->HasCreatedClients())
    return false;

  CLog::Log(LOGDEBUG, "PVRManager - %s - active clients found. continue to start", __FUNCTION__);

  CPVRGUIProgressHandler* progressHandler = nullptr;

  /* load all channels and groups */
  if (bShowProgress)
  {
    progressHandler = new CPVRGUIProgressHandler(g_localizeStrings.Get(19235)); // PVR manager is starting up
    progressHandler->UpdateProgress(g_localizeStrings.Get(19236), 0); // Loading channels from clients
  }

  if (!m_channelGroups->Load() || !IsInitialising())
  {
    if (progressHandler)
      progressHandler->DestroyProgress();

    return false;
  }

  SetChanged();
  NotifyObservers(ObservableMessageChannelGroupsLoaded);

  /* get timers from the backends */
  if (bShowProgress)
    progressHandler->UpdateProgress(g_localizeStrings.Get(19237), 50); // Loading timers from clients

  m_timers->Load();

  /* get recordings from the backend */
  if (bShowProgress)
    progressHandler->UpdateProgress(g_localizeStrings.Get(19238), 75); // Loading recordings from clients

  m_recordings->Load();

  if (!IsInitialising())
  {
    if (progressHandler)
      progressHandler->DestroyProgress();

    return false;
  }

  /* start the other pvr related update threads */
  if (bShowProgress)
  {
    progressHandler->UpdateProgress(g_localizeStrings.Get(19239), 85); // Starting background threads
    progressHandler->DestroyProgress();
  }

  m_guiInfo->Start();

  return true;
}

void CPVRManager::TriggerPlayChannelOnStartup(void)
{
  if (IsStarted())
    CJobManager::GetInstance().AddJob(new CPVRPlayChannelOnStartupJob(), nullptr);
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

bool CPVRManager::IsPlayingEpgTag(const CPVREpgInfoTagPtr &epgTag) const
{
  bool bReturn(false);

  if (epgTag && IsStarted())
  {
    CPVREpgInfoTagPtr current(GetCurrentEpgTag());
    if (current && *current == *epgTag)
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

CPVREpgInfoTagPtr CPVRManager::GetCurrentEpgTag(void) const
{
  return m_addons->GetPlayingEpgTag();
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
      m_settings.GetBoolValue(CSettings::SETTING_PVRPARENTAL_ENABLED) &&
      // channel is locked
      channel && channel->IsLocked())
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
  const CPVRChannelPtr channel = fileItem.GetPVRChannelInfoTag();
  if (!IsParentalLocked(channel))
    bReturn = m_addons->OpenStream(channel);

  return bReturn;
}

bool CPVRManager::OpenRecordedStream(const CPVRRecordingPtr &recording)
{
  return m_addons->OpenStream(recording);
}

void CPVRManager::CloseStream(void)
{
  m_addons->CloseStream();
}

void CPVRManager::OnPlaybackStarted(const CFileItemPtr item)
{
  m_addons->ClearPlayingChannel();
  m_addons->ClearPlayingRecording();
  m_addons->ClearPlayingEpgTag();

  if (item->HasPVRChannelInfoTag())
  {
    const CPVRChannelPtr channel(item->GetPVRChannelInfoTag());

    m_addons->SetPlayingChannel(channel);

    m_guiActions->GetChannelNavigator().SetPlayingChannel(channel);
    SetPlayingGroup(channel);
    UpdateLastWatched(channel);

    // set channel as selected item
    CGUIWindowPVRBase::SetSelectedItemPath(channel->IsRadio(), channel->Path());
  }
  else if (item->HasPVRRecordingInfoTag())
  {
    m_addons->SetPlayingRecording(item->GetPVRRecordingInfoTag());
  }
  else if (item->HasEPGInfoTag())
  {
    m_addons->SetPlayingEpgTag(item->GetEPGInfoTag());
  }
}

void CPVRManager::OnPlaybackStopped(const CFileItemPtr item)
{
  // Playback ended due to user interaction

  if (item->HasPVRChannelInfoTag())
  {
    UpdateLastWatched(item->GetPVRChannelInfoTag());

    // store channel settings
    g_application.SaveFileState();

    m_addons->ClearPlayingChannel();
    m_guiActions->GetChannelNavigator().ClearPlayingChannel();
  }
  else if (item->HasPVRRecordingInfoTag())
  {
    m_addons->ClearPlayingRecording();
  }
  else if (item->HasEPGInfoTag())
  {
    m_addons->ClearPlayingEpgTag();
  }
}

void CPVRManager::OnPlaybackEnded(const CFileItemPtr item)
{
  // Playback ended, but not due to user interaction
  OnPlaybackStopped(item);
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

bool CPVRManager::GetVideoLabel(const CFileItem &item, int iLabel, std::string &strValue) const
{
  return IsStarted() && m_guiInfo ? m_guiInfo->GetVideoLabel(item, iLabel, strValue) : false;
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
          dailywakeuptime.SetFromDBTime(m_settings.GetStringValue(CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME));
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
  const CDateTimeSpan idle(0, 0, m_settings.GetIntValue(CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME), 0);
  const CDateTime next(m_timers->GetNextEventTime());
  const CDateTimeSpan delta(next - now);

  return (delta <= idle);
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

bool CPVRManager::FillStreamFileItem(CFileItem &fileItem)
{
  if (fileItem.IsPVRChannel())
    return m_addons->FillChannelStreamFileItem(fileItem);
  else if (fileItem.IsPVRRecording())
    return m_addons->FillRecordingStreamFileItem(fileItem);
  else if (fileItem.IsEPG())
    return m_addons->FillEpgTagStreamFileItem(fileItem);
  else
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

std::string CPVRManager::GetPlayingTVGroupName()
{
  return IsStarted() && m_guiInfo ? m_guiInfo->GetPlayingTVGroup() : "";
}

void CPVRManager::UpdateLastWatched(const CPVRChannelPtr &channel)
{
  time_t tNow;
  CDateTime::GetCurrentDateTime().GetAsTime(tNow);

  channel->SetLastWatched(tNow);

  // update last watched timestamp for group
  CPVRChannelGroupPtr group(GetPlayingGroup(channel->IsRadio()));
  group->SetLastWatched(tNow);

  /* update last played group */
  m_channelGroups->SetLastPlayedGroup(group);
}

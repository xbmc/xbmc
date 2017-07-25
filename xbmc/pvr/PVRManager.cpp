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
#include "pvr/PVRDatabase.h"
#include "pvr/PVRGUIActions.h"
#include "pvr/PVRGUIInfo.h"
#include "pvr/PVRJobs.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/recordings/PVRRecordingsPath.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/windows/GUIWindowPVRBase.h"
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
    m_bIsSwitchingChannels(false),
    m_bEpgsCreated(false),
    m_progressBar(nullptr),
    m_progressHandle(nullptr),
    m_managerState(ManagerStateStopped),
    m_isChannelPreview(false),
    m_settings({
      CSettings::SETTING_PVRPOWERMANAGEMENT_ENABLED,
      CSettings::SETTING_PVRPOWERMANAGEMENT_SETWAKEUPCMD,
      CSettings::SETTING_PVRPARENTAL_ENABLED,
      CSettings::SETTING_PVRPARENTAL_DURATION,
      CSettings::SETTING_EPG_HIDENOINFOAVAILABLE,
      CSettings::SETTING_PVRPLAYBACK_CHANNELENTRYTIMEOUT,
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
  // Note: we're holding the progress bar dialog instance pointer in a member because it is needed by pvr core
  //       components. The latter might run in a different thread than the gui and g_windowManager.GetWindow()
  //       locks the global graphics mutex, which easily can lead to deadlocks.
  m_progressBar = g_windowManager.GetWindow<CGUIDialogExtendedProgressBar>(WINDOW_DIALOG_EXT_PROGRESS);

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
  m_epgContainer.Stop();

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

  if (!m_isChannelPreview)
    g_application.SetCurrentFileItem(*m_currentFile);
  
  g_infoManager.SetCurrentItem(m_currentFile);

  CPVRChannelPtr channelTag(item.GetPVRChannelInfoTag());
  CPVREpgInfoTagPtr epgTagNow(channelTag->GetEPGNow());

  if (channelTag->IsRadio())
  {
    CMusicInfoTag* musictag = item.GetMusicInfoTag();
    if (musictag)
    {
      musictag->SetTitle(epgTagNow
                         ? epgTagNow->Title()
                         : m_settings.GetBoolValue(CSettings::SETTING_EPG_HIDENOINFOAVAILABLE)
                            ? ""
                            : g_localizeStrings.Get(19055)); // no information available
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
      videotag->m_strTitle = epgTagNow
        ? epgTagNow->Title()
        : m_settings.GetBoolValue(CSettings::SETTING_EPG_HIDENOINFOAVAILABLE)
          ? ""
          : g_localizeStrings.Get(19055); // no information available
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

void CPVRManager::ChannelPreviewUpDown(bool up)
{
  CSingleLock lock(m_critSection);
  CPVRChannelPtr currentChannel(m_currentFile->GetPVRChannelInfoTag());
  if (currentChannel)
  {
    CPVRChannelGroupPtr group = GetPlayingGroup(currentChannel->IsRadio());
    if (group)
    {
      CFileItemPtr newChannel = up ?
      group->GetByChannelUp(currentChannel) :
      group->GetByChannelDown(currentChannel);

      ChannelPreview(newChannel);
    }
  }
}

void CPVRManager::ChannelPreview(const CFileItemPtr item)
{
  CSingleLock lock(m_critSection);

  if (!g_infoManager.GetShowInfo() && m_settings.GetIntValue(CSettings::SETTING_PVRPLAYBACK_CHANNELENTRYTIMEOUT) == 0)
  {
    // if no info shown and no channel switch delay, just show info for current channel.
    CServiceBroker::GetPVRManager().ShowPlayerInfo(CServiceBroker::GetSettings().GetInt(CSettings::SETTING_PVRMENU_DISPLAYCHANNELINFO));
    return;
  }

  m_currentFile.reset(new CFileItem(*item));

  CPVRChannelPtr channel(item->GetPVRChannelInfoTag());
  if (!channel)
    return;

  m_isChannelPreview = !IsPlayingChannel(channel);
  g_infoManager.SetCurrentItem(m_currentFile);
  CServiceBroker::GetPVRManager().ShowPlayerInfo(CServiceBroker::GetSettings().GetInt(CSettings::SETTING_PVRMENU_DISPLAYCHANNELINFO));

  if (m_isChannelPreview)
  {
    int timeout = CServiceBroker::GetSettings().GetInt(CSettings::SETTING_PVRPLAYBACK_CHANNELENTRYTIMEOUT);
    if (timeout > 0)
    {
      if (m_channelEntryJobId >= 0)
        CJobManager::GetInstance().CancelJob(m_channelEntryJobId);
      CPVRChannelEntryTimeoutJob *job = new CPVRChannelEntryTimeoutJob(timeout);
      m_channelEntryJobId = CJobManager::GetInstance().AddJob(job, dynamic_cast<IJobCallback*>(job));
    }
  }
}

void CPVRManager::ChannelPreviewSelect()
{
  CSingleLock lock(m_critSection);

  m_channelEntryJobId = -1;

  if (m_isChannelPreview)
    m_guiActions->SwitchToChannel(m_currentFile, false);
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

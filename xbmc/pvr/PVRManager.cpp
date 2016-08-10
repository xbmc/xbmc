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
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "epg/EpgContainer.h"
#include "GUIInfoManager.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "music/tags/MusicInfoTag.h"
#include "network/Network.h"
#include "PlayListPlayer.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupInternal.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/dialogs/GUIDialogPVRChannelManager.h"
#include "pvr/dialogs/GUIDialogPVRGroupManager.h"
#include "pvr/PVRActionListener.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRGUIInfo.h"
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
#include "utils/Variant.h"
#include "video/VideoDatabase.h"
#include "ServiceBroker.h"

using namespace MUSIC_INFO;
using namespace PVR;
using namespace EPG;
using namespace ANNOUNCEMENT;
using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;

const int CPVRManager::m_pvrWindowIds[12] = {
    WINDOW_TV_CHANNELS,
    WINDOW_TV_GUIDE,
    WINDOW_TV_RECORDINGS,
    WINDOW_TV_SEARCH,
    WINDOW_TV_TIMERS,
    WINDOW_TV_TIMER_RULES,
    WINDOW_RADIO_CHANNELS,
    WINDOW_RADIO_GUIDE,
    WINDOW_RADIO_RECORDINGS,
    WINDOW_RADIO_SEARCH,
    WINDOW_RADIO_TIMERS,
    WINDOW_RADIO_TIMER_RULES
};

CPVRManager::CPVRManager(void) :
    CThread("PVRManager"),
    m_triggerEvent(true),
    m_currentFile(NULL),
    m_database(NULL),
    m_bFirstStart(true),
    m_bIsSwitchingChannels(false),
    m_bEpgsCreated(false),
    m_progressHandle(NULL),
    m_managerState(ManagerStateStopped),
    m_isChannelPreview(false)
{
  CAnnouncementManager::GetInstance().AddAnnouncer(this);
  m_addons.reset(new CPVRClients);
}

CPVRManager::~CPVRManager(void)
{
  CSettings::GetInstance().UnregisterCallback(this);
  CAnnouncementManager::GetInstance().RemoveAnnouncer(this);
  Stop();
  m_addons.reset();
  CLog::Log(LOGDEBUG,"PVRManager - destroyed");
}

void CPVRManager::Announce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (!IsStarted())
    return;

  if ((flag & (ANNOUNCEMENT::System)))
  {
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

  if ((flag & (ANNOUNCEMENT::GUI)))
  {
    if (strcmp(message, "OnScreensaverActivated") == 0)
      g_PVRClients->OnPowerSavingActivated();
    else if (strcmp(message, "OnScreensaverDeactivated") == 0)
      g_PVRClients->OnPowerSavingDeactivated();
  }
}

CPVRManager &CPVRManager::GetInstance()
{
  return CServiceBroker::GetPVRManager();
}

void CPVRManager::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_PVRPARENTAL_ENABLED)
  {
    if (((CSettingBool*)setting)->GetValue() && CSettings::GetInstance().GetString(CSettings::SETTING_PVRPARENTAL_PIN).empty())
    {
      std::string newPassword = "";
      // password set... save it
      if (CGUIDialogNumeric::ShowAndVerifyNewPassword(newPassword))
        CSettings::GetInstance().SetString(CSettings::SETTING_PVRPARENTAL_PIN, newPassword);
      // password not set... disable parental
      else
        ((CSettingBool*)setting)->SetValue(false);
    }
  }
  else if(settingId == CSettings::SETTING_EPG_DAYSTODISPLAY)
  {
    m_addons->SetEPGTimeFrame(static_cast<const CSettingInt*>(setting)->GetValue());
  }
}

void CPVRManager::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_PVRMENU_SEARCHICONS)
  {
    if (IsStarted())
      TriggerSearchMissingChannelIcons();
  }
  else if (settingId == CSettings::SETTING_PVRMANAGER_RESETDB)
  {
    if (CheckParentalPIN(g_localizeStrings.Get(19262)) &&
      HELPERS::ShowYesNoDialogText(CVariant{19098}, CVariant{19186}) == DialogResponse::YES)
    {
      CDateTime::ResetTimezoneBias();
      ResetDatabase(false);
    }
  }
  else if (settingId == CSettings::SETTING_EPG_RESETEPG)
  {
    if (HELPERS::ShowYesNoDialogText(CVariant{19098}, CVariant{19188}) == DialogResponse::YES)
    {
      CDateTime::ResetTimezoneBias();
      ResetDatabase(true);
    }
  }
  else if (settingId == CSettings::SETTING_PVRMANAGER_CHANNELSCAN)
  {
    if (IsStarted())
      StartChannelScan();
  }
  else if (settingId == CSettings::SETTING_PVRMANAGER_CHANNELMANAGER)
  {
    if (IsStarted())
    {
      CGUIDialogPVRChannelManager *dialog = (CGUIDialogPVRChannelManager *)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_CHANNEL_MANAGER);
      if (dialog)
        dialog->Open();
    }
  }
  else if (settingId == CSettings::SETTING_PVRMANAGER_GROUPMANAGER)
  {
    if (IsStarted())
    {
      CGUIDialogPVRGroupManager *dialog = (CGUIDialogPVRGroupManager *)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GROUP_MANAGER);
      if (dialog)
        dialog->Open();
    }
  }
  else if (settingId == CSettings::SETTING_PVRCLIENT_MENUHOOK)
  {
    if (IsStarted())
      m_addons->ProcessMenuHooks(-1, PVR_MENUHOOK_SETTING, NULL);
  }
}

void CPVRManager::Cleanup(void)
{
  CSingleLock lock(m_critSection);

  m_guiInfo.reset();
  m_timers.reset();
  m_recordings.reset();
  m_channelGroups.reset();
  m_parentalTimer.reset();
  SAFE_DELETE(m_database);
  m_triggerEvent.Set();

  m_currentFile           = NULL;
  m_bIsSwitchingChannels  = false;
  m_bEpgsCreated = false;

  for (unsigned int iJobPtr = 0; iJobPtr < m_pendingUpdates.size(); iJobPtr++)
    delete m_pendingUpdates.at(iJobPtr);
  m_pendingUpdates.clear();

  /* unregister application action listener */
  {
    CSingleExit exit(m_critSection);
    g_application.UnregisterActionListener(&CPVRActionListener::GetInstance());
  }

  HideProgressDialog();

  SetState(ManagerStateStopped);
}

void CPVRManager::ResetProperties(void)
{
  CSingleLock lock(m_critSection);
  Cleanup();

  m_channelGroups.reset(new CPVRChannelGroupsContainer);
  m_recordings.reset(new CPVRRecordings);
  m_timers.reset(new CPVRTimers);
  m_guiInfo.reset(new CPVRGUIInfo);
  m_parentalTimer.reset(new CStopWatch);
}

void CPVRManager::Init()
{
  std::set<std::string> settingSet;
  settingSet.insert(CSettings::SETTING_PVRMANAGER_CHANNELMANAGER);
  settingSet.insert(CSettings::SETTING_PVRMANAGER_GROUPMANAGER);
  settingSet.insert(CSettings::SETTING_PVRMANAGER_CHANNELSCAN);
  settingSet.insert(CSettings::SETTING_PVRMANAGER_RESETDB);
  settingSet.insert(CSettings::SETTING_PVRCLIENT_MENUHOOK);
  settingSet.insert(CSettings::SETTING_PVRMENU_SEARCHICONS);
  settingSet.insert(CSettings::SETTING_EPG_RESETEPG);
  settingSet.insert(CSettings::SETTING_EPG_DAYSTODISPLAY);
  settingSet.insert(CSettings::SETTING_PVRPARENTAL_ENABLED);
  CSettings::GetInstance().RegisterCallback(this, settingSet);

  // initial check for enabled addons
  // if at least one pvr addon is enabled, PVRManager start up
  CJobManager::GetInstance().AddJob(new CPVRStartupJob(), nullptr);
}

void CPVRManager::Start()
{
  Stop();

  CSingleLock lock(m_critSection);

  if (!m_addons->HasCreatedClients())
    return;

  ResetProperties();
  SetState(ManagerStateStarting);

  /* create the pvrmanager thread, which will ensure that all data will be loaded */
  Create();
  SetPriority(-1);
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
    CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_STOP);
  }

  /* stop all update threads */
  SetState(ManagerStateInterrupted);

  StopThread();
  if (m_guiInfo)
    m_guiInfo->Stop();

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
  if (m_managerState == state)
    return;

  m_managerState = state;
  m_events.Publish(m_managerState);
}

void CPVRManager::Process(void)
{
  /* create and open database */
  if (!m_database)
    m_database = new CPVRDatabase;
  m_database->Open();

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

    if (IsStarted() && !bRestart)
      m_triggerEvent.WaitMSec(1000);
  }

  if (IsStarted())
  {
    CLog::Log(LOGNOTICE, "PVRManager - %s - no add-ons enabled anymore. restarting the pvrmanager", __FUNCTION__);
    CApplicationMessenger::GetInstance().PostMsg(TMSG_SETPVRMANAGERSTATE, 1);
  }
}

bool CPVRManager::SetWakeupCommand(void)
{
  if (!CSettings::GetInstance().GetBool(CSettings::SETTING_PVRPOWERMANAGEMENT_ENABLED))
    return false;

  const std::string strWakeupCommand = CSettings::GetInstance().GetString(CSettings::SETTING_PVRPOWERMANAGEMENT_SETWAKEUPCMD);
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
  g_PVRClients->OnSystemSleep();
}

void CPVRManager::OnWake()
{
  g_PVRClients->OnSystemWake();
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

  /* reset observer for pvr windows */
  for (std::size_t i = 0; i != ARRAY_SIZE(m_pvrWindowIds); i++)
  {
    CSingleExit exit(m_critSection);
    CGUIWindowPVRBase *pWindow = (CGUIWindowPVRBase *) g_windowManager.GetWindow(m_pvrWindowIds[i]);
    if (pWindow)
      pWindow->ResetObservers();
  }

  /* load all channels and groups */
  if (bShowProgress)
    ShowProgressDialog(g_localizeStrings.Get(19236), 0); // Loading channels from clients
  if (!m_channelGroups->Load() || !IsInitialising())
    return false;

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

  /* close the progess dialog */
  if (bShowProgress)
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

bool CPVRManager::ContinueLastChannel(void)
{
  if (CSettings::GetInstance().GetInt(CSettings::SETTING_PVRPLAYBACK_STARTLAST) == CONTINUE_LAST_CHANNEL_OFF)
    return false;

  CFileItemPtr channel = m_channelGroups->GetLastPlayedChannel();
  if (channel && channel->HasPVRChannelInfoTag())
  {
    CLog::Log(LOGNOTICE, "PVRManager - %s - continue playback on channel '%s'", __FUNCTION__, channel->GetPVRChannelInfoTag()->ChannelName().c_str());
    SetPlayingGroup(m_channelGroups->GetLastPlayedGroup(channel->GetPVRChannelInfoTag()->ChannelID()));
    StartPlayback(channel->GetPVRChannelInfoTag(), (CSettings::GetInstance().GetInt(CSettings::SETTING_PVRPLAYBACK_STARTLAST) == CONTINUE_LAST_CHANNEL_IN_BACKGROUND));
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
  pDlgProgress->SetHeading(CVariant{313});
  pDlgProgress->SetLine(0, CVariant{g_localizeStrings.Get(19187)}); // All data in the PVR database is being erased
  pDlgProgress->SetLine(1, CVariant{""});
  pDlgProgress->SetLine(2, CVariant{""});
  pDlgProgress->Open();
  pDlgProgress->Progress();

  if (m_addons->IsPlaying())
  {
    CLog::Log(LOGNOTICE,"PVRManager - %s - stopping playback", __FUNCTION__);
    CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_STOP);
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
        videoDatabase.EraseVideoSettings(CPVRRecordingsPath::PATH_RECORDINGS);
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

  CLog::Log(LOGNOTICE,"PVRManager - %s - restarting the PVRManager", __FUNCTION__);
  m_database->Open();
  Cleanup();
  Start();

  pDlgProgress->SetPercentage(100);
  pDlgProgress->Close();
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

CPVRChannelPtr CPVRManager::GetCurrentChannel(void) const
{
  return m_addons->GetPlayingChannel();
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
  const CPVRChannelPtr channel(m_channelGroups->GetChannelById(iChannelId));
  if (!channel)
    return false;

  return SetRecordingOnChannel(channel, !channel->IsRecording());
}

void CPVRManager::StartRecordingOnPlayingChannel(bool bOnOff)
{
  // can be called from VideoPlayer thread. SetRecordingOnChannel can open a dialog. Thus, execute async.
  CJobManager::GetInstance().AddJob(new CPVRSetRecordingOnChannelJob(m_addons->GetPlayingChannel(), bOnOff), NULL);
}

namespace
{
enum PVRRECORD_INSTANTRECORDACTION
{
  NONE = -1,
  RECORD_CURRENT_SHOW = 0,
  RECORD_INSTANTRECORDTIME = 1,
  ASK = 2,
  RECORD_30_MINUTES = 3,
  RECORD_60_MINUTES = 4,
  RECORD_120_MINUTES = 5,
  RECORD_NEXT_SHOW = 6
};

class InstantRecordingActionSelector
{
public:
  InstantRecordingActionSelector();
  virtual ~InstantRecordingActionSelector() {}

  void AddAction(PVRRECORD_INSTANTRECORDACTION eAction, const std::string &title);
  void PreSelectAction(PVRRECORD_INSTANTRECORDACTION eAction);
  PVRRECORD_INSTANTRECORDACTION Select();

private:
  CGUIDialogSelect *m_pDlgSelect; // not owner!
  std::map<PVRRECORD_INSTANTRECORDACTION, int> m_actions;
};

InstantRecordingActionSelector::InstantRecordingActionSelector()
: m_pDlgSelect(dynamic_cast<CGUIDialogSelect *>(g_windowManager.GetWindow(WINDOW_DIALOG_SELECT)))
{
  if (m_pDlgSelect)
  {
    m_pDlgSelect->SetMultiSelection(false);
    m_pDlgSelect->SetHeading(CVariant{19086}); // Instant recording action
  }
  else
  {
    CLog::Log(LOGERROR, "InstantRecordingActionSelector - %s - unable to obtain WINDOW_DIALOG_SELECT instance", __FUNCTION__);
  }
}

void InstantRecordingActionSelector::AddAction(PVRRECORD_INSTANTRECORDACTION eAction, const std::string &title)
{
  if (m_actions.find(eAction) == m_actions.end())
  {
    switch (eAction)
    {
      case RECORD_INSTANTRECORDTIME:
        m_pDlgSelect->Add(StringUtils::Format(g_localizeStrings.Get(19090).c_str(),
                                              CSettings::GetInstance().GetInt(CSettings::SETTING_PVRRECORD_INSTANTRECORDTIME))); // Record next <default duration> minutes
        break;
      case RECORD_30_MINUTES:
        m_pDlgSelect->Add(StringUtils::Format(g_localizeStrings.Get(19090).c_str(), 30));  // Record next 30 minutes
        break;
      case RECORD_60_MINUTES:
        m_pDlgSelect->Add(StringUtils::Format(g_localizeStrings.Get(19090).c_str(), 60));  // Record next 60 minutes
        break;
      case RECORD_120_MINUTES:
        m_pDlgSelect->Add(StringUtils::Format(g_localizeStrings.Get(19090).c_str(), 120)); // Record next 120 minutes
        break;
      case RECORD_CURRENT_SHOW:
        m_pDlgSelect->Add(StringUtils::Format(g_localizeStrings.Get(19091).c_str(), title.c_str())); // Record current show (<title>)
        break;
      case RECORD_NEXT_SHOW:
        m_pDlgSelect->Add(StringUtils::Format(g_localizeStrings.Get(19092).c_str(), title.c_str())); // Record next show (<title>)
        break;
      case NONE:
      case ASK:
      default:
        return;
    }

    m_actions.insert(std::make_pair(eAction, m_actions.size()));
  }
}

void InstantRecordingActionSelector::PreSelectAction(PVRRECORD_INSTANTRECORDACTION eAction)
{
  const auto &it = m_actions.find(eAction);
  if (it != m_actions.end())
    m_pDlgSelect->SetSelected(it->second);
}

PVRRECORD_INSTANTRECORDACTION InstantRecordingActionSelector::Select()
{
  PVRRECORD_INSTANTRECORDACTION eAction = NONE;

  m_pDlgSelect->Open();

  if (m_pDlgSelect->IsConfirmed())
  {
    int iSelection = m_pDlgSelect->GetSelectedItem();
    for (const auto &action : m_actions)
    {
      if (action.second == iSelection)
      {
        eAction = action.first;
        break;
      }
    }
  }

  return eAction;
}

} // unnamed namespace

bool CPVRManager::SetRecordingOnChannel(const CPVRChannelPtr &channel, bool bOnOff)
{
  bool bReturn = false;

  if (!channel)
    return bReturn;

  if (!g_PVRManager.CheckParentalLock(channel))
    return bReturn;

  if (m_addons->HasTimerSupport(channel->ClientID()))
  {
    /* timers are supported on this channel */
    if (bOnOff && !channel->IsRecording())
    {
      CEpgInfoTagPtr epgTag;
      int iDuration = CSettings::GetInstance().GetInt(CSettings::SETTING_PVRRECORD_INSTANTRECORDTIME);

      int iAction = CSettings::GetInstance().GetInt(CSettings::SETTING_PVRRECORD_INSTANTRECORDACTION);
      switch (iAction)
      {
        case RECORD_CURRENT_SHOW:
          epgTag = channel->GetEPGNow();
          break;

        case RECORD_INSTANTRECORDTIME:
          epgTag.reset();
          break;

        case ASK:
        {
          PVRRECORD_INSTANTRECORDACTION ePreselect = RECORD_INSTANTRECORDTIME;
          InstantRecordingActionSelector selector;
          CEpgInfoTagPtr epgTagNext;

          // fixed length recordings
          selector.AddAction(RECORD_30_MINUTES, "");
          selector.AddAction(RECORD_60_MINUTES, "");
          selector.AddAction(RECORD_120_MINUTES, "");

          const int iDurationDefault = CSettings::GetInstance().GetInt(CSettings::SETTING_PVRRECORD_INSTANTRECORDTIME);
          if (iDurationDefault != 30 && iDurationDefault != 60 && iDurationDefault != 120)
            selector.AddAction(RECORD_INSTANTRECORDTIME, "");

          // epg-based recordings
          epgTag = channel->GetEPGNow();
          if (epgTag)
          {
            // "now"
            selector.AddAction(RECORD_CURRENT_SHOW, epgTag->Title());
            ePreselect = RECORD_CURRENT_SHOW;

            // "next"
            epgTagNext = channel->GetEPGNext();
            if (epgTagNext)
            {
              selector.AddAction(RECORD_NEXT_SHOW, epgTagNext->Title());

              // be smart. if current show is almost over, preselect next show.
              if (epgTag->ProgressPercentage() > 90.0f)
                ePreselect = RECORD_NEXT_SHOW;
            }
          }

          selector.PreSelectAction(ePreselect);

          PVRRECORD_INSTANTRECORDACTION eSelected = selector.Select();
          switch (eSelected)
          {
            case NONE:
              return false; // dialog canceled

            case RECORD_30_MINUTES:
              iDuration = 30;
              epgTag.reset();
              break;

            case RECORD_60_MINUTES:
              iDuration = 60;
              epgTag.reset();
              break;

            case RECORD_120_MINUTES:
              iDuration = 120;
              epgTag.reset();
              break;

            case RECORD_INSTANTRECORDTIME:
              iDuration = iDurationDefault;
              epgTag.reset();
              break;

            case RECORD_CURRENT_SHOW:
              break;

            case RECORD_NEXT_SHOW:
              epgTag = epgTagNext;
              break;

            default:
              CLog::Log(LOGERROR, "PVRManager - %s - unknown instant record action selection (%d), defaulting to fixed length recording.", __FUNCTION__, eSelected);
              epgTag.reset();
              break;
          }
          break;
        }

        default:
          CLog::Log(LOGERROR, "PVRManager - %s - unknown instant record action setting value (%d), defaulting to fixed length recording.", __FUNCTION__, iAction);
          break;
      }

      const CPVRTimerInfoTagPtr newTimer(epgTag ? CPVRTimerInfoTag::CreateFromEpg(epgTag, false) : CPVRTimerInfoTag::CreateInstantTimerTag(channel, iDuration));

      if (newTimer)
        bReturn = newTimer->AddToClient();

      if (!bReturn)
        CGUIDialogOK::ShowAndGetInput(CVariant{19033}, CVariant{19164});
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
      CSettings::GetInstance().GetBool(CSettings::SETTING_PVRPARENTAL_ENABLED) &&
      // channel is locked
      channel && channel->IsLocked())
  {
    float parentalDurationMs = CSettings::GetInstance().GetInt(CSettings::SETTING_PVRPARENTAL_DURATION) * 1000.0f;
    bReturn = m_parentalTimer &&
        (!m_parentalTimer->IsRunning() ||
          m_parentalTimer->GetElapsedMilliseconds() > parentalDurationMs);
  }

  return bReturn;
}

bool CPVRManager::CheckParentalPIN(const std::string& strTitle /* = "" */)
{
  std::string pinCode = CSettings::GetInstance().GetString(CSettings::SETTING_PVRPARENTAL_PIN);

  if (!CSettings::GetInstance().GetBool(CSettings::SETTING_PVRPARENTAL_ENABLED) || pinCode.empty())
    return true;

  // Locked channel. Enter PIN:
  bool bValidPIN = CGUIDialogNumeric::ShowAndVerifyInput(pinCode, !strTitle.empty() ? strTitle : g_localizeStrings.Get(19263), true);
  if (!bValidPIN)
    // display message: The entered PIN number was incorrect
    CGUIDialogOK::ShowAndGetInput(CVariant{19264}, CVariant{19265});
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

bool CPVRStartupJob::DoWork(void)
{
  g_PVRClients->Start();
  return true;
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
    if(m_currentFile)
      delete m_currentFile;
    m_currentFile = new CFileItem(fileItem);

    CPVRChannelPtr channel(m_addons->GetPlayingChannel());
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

  m_isChannelPreview = false;
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
    CFileItemList *l = new CFileItemList; //don't delete,
    l->Add(std::make_shared<CFileItem>(pvrItem));
    CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, -1, -1, static_cast<void*>(l));
    return true;
  }

  return g_application.PlayFile(pvrItem, "videoplayer", false) == PLAYBACK_OK;
}

void CPVRManager::UpdateCurrentChannel(void)
{
  CSingleLock lock(m_critSection);

  CPVRChannelPtr playingChannel(GetCurrentChannel());
  if (m_currentFile &&
      playingChannel &&
      !IsPlayingChannel(m_currentFile->GetPVRChannelInfoTag()))
  {
    delete m_currentFile;
    m_currentFile = new CFileItem(playingChannel);
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
  if (!m_currentFile || *m_currentFile->GetPVRChannelInfoTag() == *item.GetPVRChannelInfoTag())
    return false;

  g_application.SetCurrentFileItem(*m_currentFile);
  CFileItemPtr itemptr(new CFileItem(*m_currentFile));
  g_infoManager.SetCurrentItem(itemptr);

  CPVRChannelPtr channelTag(item.GetPVRChannelInfoTag());
  CEpgInfoTagPtr epgTagNow(channelTag->GetEPGNow());

  if (channelTag->IsRadio())
  {
    CMusicInfoTag* musictag = item.GetMusicInfoTag();
    if (musictag)
    {
      musictag->SetTitle(epgTagNow ?
          epgTagNow->Title() :
          CSettings::GetInstance().GetBool(CSettings::SETTING_EPG_HIDENOINFOAVAILABLE) ?
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
          CSettings::GetInstance().GetBool(CSettings::SETTING_EPG_HIDENOINFOAVAILABLE) ?
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
  CMediaSettings::GetInstance().SetVideoStartWindowed(bMinimised);
  
  CFileItemList *l = new CFileItemList; //don't delete,
  l->Add(std::make_shared<CFileItem>(channel));
  CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, -1, -1, static_cast<void*>(l));

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

      if (IsPlayingChannel(channel))
        m_isChannelPreview = false;
      else
        m_isChannelPreview = true;

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
          dailywakeuptime.SetFromDBTime(CSettings::GetInstance().GetString(CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME));
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
    0, 0, CSettings::GetInstance().GetInt(CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME), 0);
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

bool CPVRManager::IsRunningChannelScan(void) const
{
  return IsStarted() && m_addons->IsRunningChannelScan();
}

void CPVRManager::StartChannelScan(void)
{
  if (IsStarted())
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

void CPVRManager::ConnectionStateChange(int clientId, std::string connectString, PVR_CONNECTION_STATE state, std::string message)
{
  CJobManager::GetInstance().AddJob(new CPVRClientConnectionJob(clientId, connectString, state, message), NULL);
}

void CPVRManager::ExecutePendingJobs(void)
{
  CSingleLock lock(m_critSectionTriggers);

  while (!m_pendingUpdates.empty())
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
    ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::Player, "xbmc", "OnStop", CFileItemPtr(m_previous), data);
  }

  // announce OnPlay if the switch was successful
  if (m_next)
  {
    CVariant param;
    param["player"]["speed"] = 1;
    param["player"]["playerid"] = g_playlistPlayer.GetCurrentPlaylist();
    ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::Player, "xbmc", "OnPlay", CFileItemPtr(new CFileItem(*m_next)), param);
  }

  return true;
}

bool CPVRSearchMissingChannelIconsJob::DoWork(void)
{
  g_PVRManager.SearchMissingChannelIcons();
  return true;
}

bool CPVRClientConnectionJob::DoWork(void)
{
  g_PVRClients->ConnectionStateChange(m_clientId, m_connectString, m_state, m_message);
  return true;
}

bool CPVRSetRecordingOnChannelJob::DoWork(void)
{
  g_PVRManager.SetRecordingOnChannel(m_channel, m_bOnOff);
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

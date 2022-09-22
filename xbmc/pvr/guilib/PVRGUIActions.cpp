/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIActions.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "network/Network.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRItem.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClientMenuHooks.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgDatabase.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/recordings/PVRRecordingsPath.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/windows/GUIWindowPVRBase.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include <algorithm>
#include <chrono>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

using namespace KODI::MESSAGING;

namespace PVR
{
CPVRGUIActions::CPVRGUIActions()
  : m_settings({CSettings::SETTING_PVRMANAGER_PRESELECTPLAYINGCHANNEL,
                CSettings::SETTING_PVRPARENTAL_PIN, CSettings::SETTING_PVRPARENTAL_ENABLED,
                CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME,
                CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME})
{
}

bool CPVRGUIActions::HideChannel(const std::shared_ptr<CFileItem>& item) const
{
  const std::shared_ptr<CPVRChannel> channel(item->GetPVRChannelInfoTag());

  if (!channel)
    return false;

  if (!CGUIDialogYesNo::ShowAndGetInput(
          CVariant{19054}, // "Hide channel"
          CVariant{19039}, // "Are you sure you want to hide this channel?"
          CVariant{""}, CVariant{channel->ChannelName()}))
    return false;

  if (!CServiceBroker::GetPVRManager()
           .ChannelGroups()
           ->GetGroupAll(channel->IsRadio())
           ->RemoveFromGroup(channel))
    return false;

  CGUIWindowPVRBase* pvrWindow =
      dynamic_cast<CGUIWindowPVRBase*>(CServiceBroker::GetGUI()->GetWindowManager().GetWindow(
          CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow()));
  if (pvrWindow)
    pvrWindow->DoRefresh();
  else
    CLog::LogF(LOGERROR, "Called on non-pvr window. No refresh possible.");

  return true;
}

  bool CPVRGUIActions::StartChannelScan()
  {
    return StartChannelScan(PVR_INVALID_CLIENT_ID);
  }

  bool CPVRGUIActions::StartChannelScan(int clientId)
  {
    if (!CServiceBroker::GetPVRManager().IsStarted() || IsRunningChannelScan())
      return false;

    std::shared_ptr<CPVRClient> scanClient;
    std::vector<std::shared_ptr<CPVRClient>> possibleScanClients = CServiceBroker::GetPVRManager().Clients()->GetClientsSupportingChannelScan();
    m_bChannelScanRunning = true;

    if (clientId != PVR_INVALID_CLIENT_ID)
    {
      const auto it =
          std::find_if(possibleScanClients.cbegin(), possibleScanClients.cend(),
                       [clientId](const auto& client) { return client->GetID() == clientId; });

      if (it != possibleScanClients.cend())
        scanClient = (*it);

      if (!scanClient)
      {
        CLog::LogF(LOGERROR,
                   "Provided client id '{}' could not be found in list of possible scan clients!",
                   clientId);
        m_bChannelScanRunning = false;
        return false;
      }
    }
    /* multiple clients found */
    else if (possibleScanClients.size() > 1)
    {
      CGUIDialogSelect* pDialog= CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
      if (!pDialog)
      {
        CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT!");
        m_bChannelScanRunning = false;
        return false;
      }

      pDialog->Reset();
      pDialog->SetHeading(CVariant{19119}); // "On which backend do you want to search?"

      for (const auto& client : possibleScanClients)
        pDialog->Add(client->GetFriendlyName());

      pDialog->Open();

      int selection = pDialog->GetSelectedItem();
      if (selection >= 0)
        scanClient = possibleScanClients[selection];
    }
    /* one client found */
    else if (possibleScanClients.size() == 1)
    {
      scanClient = possibleScanClients[0];
    }
    /* no clients found */
    else if (!scanClient)
    {
      HELPERS::ShowOKDialogText(CVariant{19033},  // "Information"
                                    CVariant{19192}); // "None of the connected PVR backends supports scanning for channels."
      m_bChannelScanRunning = false;
      return false;
    }

    /* start the channel scan */
    CLog::LogFC(LOGDEBUG, LOGPVR, "Starting to scan for channels on client {}",
                scanClient->GetFriendlyName());
    auto start = std::chrono::steady_clock::now();

    /* do the scan */
    if (scanClient->StartChannelScan() != PVR_ERROR_NO_ERROR)
      HELPERS::ShowOKDialogText(CVariant{257},    // "Error"
                                    CVariant{19193}); // "The channel scan can't be started. Check the log for more information about this message."

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    CLog::LogFC(LOGDEBUG, LOGPVR, "Channel scan finished after {} ms", duration.count());

    m_bChannelScanRunning = false;
    return true;
  }

  bool CPVRGUIActions::ProcessSettingsMenuHooks()
  {
    const CPVRClientMap clients = CServiceBroker::GetPVRManager().Clients()->GetCreatedClients();

    std::vector<std::pair<std::shared_ptr<CPVRClient>, CPVRClientMenuHook>> settingsHooks;
    for (const auto& client : clients)
    {
      const auto hooks = client.second->GetMenuHooks()->GetSettingsHooks();
      std::transform(hooks.cbegin(), hooks.cend(), std::back_inserter(settingsHooks),
                     [&client](const auto& hook) { return std::make_pair(client.second, hook); });
    }

    if (settingsHooks.empty())
    {
      HELPERS::ShowOKDialogText(
          CVariant{19033}, // "Information"
          CVariant{
              19347}); // "None of the active PVR clients does provide client-specific settings."
      return true; // no settings hooks, no error
    }

    auto selectedHook = settingsHooks.begin();

    // if there is only one settings hook, execute it directly, otherwise let the user select
    if (settingsHooks.size() > 1)
    {
      CGUIDialogSelect* pDialog= CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
      if (!pDialog)
      {
        CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT!");
        return false;
      }

      pDialog->Reset();
      pDialog->SetHeading(CVariant{19196}); // "PVR client specific actions"

      for (const auto& hook : settingsHooks)
      {
        if (clients.size() == 1)
          pDialog->Add(hook.second.GetLabel());
        else
          pDialog->Add(hook.first->GetBackendName() + ": " + hook.second.GetLabel());
      }

      pDialog->Open();

      int selection = pDialog->GetSelectedItem();
      if (selection < 0)
        return true; // cancelled

      std::advance(selectedHook, selection);
    }
    return selectedHook->first->CallSettingsMenuHook(selectedHook->second) == PVR_ERROR_NO_ERROR;
  }

  namespace
  {
  class CPVRGUIDatabaseResetComponentsSelector
  {
  public:
    CPVRGUIDatabaseResetComponentsSelector() = default;
    virtual ~CPVRGUIDatabaseResetComponentsSelector() = default;

    bool Select()
    {
      CGUIDialogSelect* pDlgSelect =
          CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
              WINDOW_DIALOG_SELECT);
      if (!pDlgSelect)
      {
        CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT!");
        return false;
      }

      CFileItemList options;

      const std::shared_ptr<CFileItem> itemAll =
          std::make_shared<CFileItem>(StringUtils::Format(g_localizeStrings.Get(593))); // All
      itemAll->SetPath("all");
      options.Add(itemAll);

      // if channels are cleared, groups, EPG data and providers must also be cleared
      const std::shared_ptr<CFileItem> itemChannels = std::make_shared<CFileItem>(
          StringUtils::Format("{}, {}, {}, {}",
                              g_localizeStrings.Get(19019), // Channels
                              g_localizeStrings.Get(19146), // Groups
                              g_localizeStrings.Get(19069), // Guide
                              g_localizeStrings.Get(19334))); // Providers
      itemChannels->SetPath("channels");
      itemChannels->Select(true); // preselect this item in dialog
      options.Add(itemChannels);

      const std::shared_ptr<CFileItem> itemGroups =
          std::make_shared<CFileItem>(g_localizeStrings.Get(19146)); // Groups
      itemGroups->SetPath("groups");
      options.Add(itemGroups);

      const std::shared_ptr<CFileItem> itemGuide =
          std::make_shared<CFileItem>(g_localizeStrings.Get(19069)); // Guide
      itemGuide->SetPath("guide");
      options.Add(itemGuide);

      const std::shared_ptr<CFileItem> itemProviders =
          std::make_shared<CFileItem>(g_localizeStrings.Get(19334)); // Providers
      itemProviders->SetPath("providers");
      options.Add(itemProviders);

      const std::shared_ptr<CFileItem> itemReminders =
          std::make_shared<CFileItem>(g_localizeStrings.Get(19215)); // Reminders
      itemReminders->SetPath("reminders");
      options.Add(itemReminders);

      const std::shared_ptr<CFileItem> itemRecordings =
          std::make_shared<CFileItem>(g_localizeStrings.Get(19017)); // Recordings
      itemRecordings->SetPath("recordings");
      options.Add(itemRecordings);

      const std::shared_ptr<CFileItem> itemClients =
          std::make_shared<CFileItem>(g_localizeStrings.Get(24019)); // PVR clients
      itemClients->SetPath("clients");
      options.Add(itemClients);

      pDlgSelect->Reset();
      pDlgSelect->SetHeading(CVariant{g_localizeStrings.Get(19185)}); // "Clear data"
      pDlgSelect->SetItems(options);
      pDlgSelect->SetMultiSelection(true);
      pDlgSelect->Open();

      if (!pDlgSelect->IsConfirmed())
        return false;

      for (int i : pDlgSelect->GetSelectedItems())
      {
        const std::string path = options.Get(i)->GetPath();

        m_bResetChannels |= (path == "channels" || path == "all");
        m_bResetGroups |= (path == "groups" || path == "all");
        m_bResetGuide |= (path == "guide" || path == "all");
        m_bResetProviders |= (path == "providers" || path == "all");
        m_bResetReminders |= (path == "reminders" || path == "all");
        m_bResetRecordings |= (path == "recordings" || path == "all");
        m_bResetClients |= (path == "clients" || path == "all");
      }

      m_bResetGroups |= m_bResetChannels;
      m_bResetGuide |= m_bResetChannels;
      m_bResetProviders |= m_bResetChannels;

      return (m_bResetChannels || m_bResetGroups || m_bResetGuide || m_bResetProviders ||
              m_bResetReminders || m_bResetRecordings || m_bResetClients);
    }

    bool IsResetChannelsSelected() const { return m_bResetChannels; }
    bool IsResetGroupsSelected() const { return m_bResetGroups; }
    bool IsResetGuideSelected() const { return m_bResetGuide; }
    bool IsResetProvidersSelected() const { return m_bResetProviders; }
    bool IsResetRemindersSelected() const { return m_bResetReminders; }
    bool IsResetRecordingsSelected() const { return m_bResetRecordings; }
    bool IsResetClientsSelected() const { return m_bResetClients; }

  private:
    bool m_bResetChannels = false;
    bool m_bResetGroups = false;
    bool m_bResetGuide = false;
    bool m_bResetProviders = false;
    bool m_bResetReminders = false;
    bool m_bResetRecordings = false;
    bool m_bResetClients = false;
  };

  } // unnamed namespace

  bool CPVRGUIActions::ResetPVRDatabase(bool bResetEPGOnly)
  {
    CGUIDialogProgress* pDlgProgress = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
    if (!pDlgProgress)
    {
      CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_PROGRESS!");
      return false;
    }

    bool bResetChannels = false;
    bool bResetGroups = false;
    bool bResetGuide = false;
    bool bResetProviders = false;
    bool bResetReminders = false;
    bool bResetRecordings = false;
    bool bResetClients = false;

    if (bResetEPGOnly)
    {
      if (!CGUIDialogYesNo::ShowAndGetInput(
              CVariant{19098}, // "Warning!"
              CVariant{19188})) // "All guide data will be cleared. Are you sure?"
        return false;

      bResetGuide = true;
    }
    else
    {
      if (CheckParentalPIN() != ParentalCheckResult::SUCCESS)
        return false;

      CPVRGUIDatabaseResetComponentsSelector selector;
      if (!selector.Select())
        return false;

      if (!CGUIDialogYesNo::ShowAndGetInput(
              CVariant{19098}, // "Warning!"
              CVariant{19186})) // "All selected data will be cleared. ... Are you sure?"
        return false;

      bResetChannels = selector.IsResetChannelsSelected();
      bResetGroups = selector.IsResetGroupsSelected();
      bResetGuide = selector.IsResetGuideSelected();
      bResetProviders = selector.IsResetProvidersSelected();
      bResetReminders = selector.IsResetRemindersSelected();
      bResetRecordings = selector.IsResetRecordingsSelected();
      bResetClients = selector.IsResetClientsSelected();
    }

    CDateTime::ResetTimezoneBias();

    CLog::LogFC(LOGDEBUG, LOGPVR, "PVR clearing {} database",
                bResetEPGOnly ? "EPG" : "PVR and EPG");

    pDlgProgress->SetHeading(CVariant{313}); // "Cleaning database"
    pDlgProgress->SetLine(0, CVariant{g_localizeStrings.Get(19187)}); // "Clearing all related data."
    pDlgProgress->SetLine(1, CVariant{""});
    pDlgProgress->SetLine(2, CVariant{""});

    pDlgProgress->Open();
    pDlgProgress->Progress();

    if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlaying())
    {
      CLog::Log(LOGINFO, "PVR is stopping playback for {} database reset",
                bResetEPGOnly ? "EPG" : "PVR and EPG");
      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_STOP);
    }

    const std::shared_ptr<CPVRDatabase> pvrDatabase(CServiceBroker::GetPVRManager().GetTVDatabase());
    const std::shared_ptr<CPVREpgDatabase> epgDatabase(CServiceBroker::GetPVRManager().EpgContainer().GetEpgDatabase());

    // increase db open refcounts, so they don't get closed during following pvr manager shutdown
    pvrDatabase->Open();
    epgDatabase->Open();

    // stop pvr manager; close both pvr and epg databases
    CServiceBroker::GetPVRManager().Stop();

    const int iProgressStepPercentage =
        100 / ((2 * bResetChannels) + bResetGroups + bResetGuide + bResetProviders +
               bResetReminders + bResetRecordings + bResetClients + 1);
    int iProgressStepsDone = 0;

    if (bResetProviders)
    {
      pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
      pDlgProgress->Progress();

      // delete all providers
      pvrDatabase->DeleteProviders();
    }

    if (bResetGuide)
    {
      pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
      pDlgProgress->Progress();

      // reset channel's EPG pointers
      pvrDatabase->ResetEPG();

      // delete all entries from the EPG database
      epgDatabase->DeleteEpg();
    }

    if (bResetGroups)
    {
      pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
      pDlgProgress->Progress();

      // delete all channel groups (including data only available locally, like user defined groups)
      pvrDatabase->DeleteChannelGroups();
    }

    if (bResetChannels)
    {
      pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
      pDlgProgress->Progress();

      // delete all channels (including data only available locally, like user set icons)
      pvrDatabase->DeleteChannels();
    }

    if (bResetReminders)
    {
      pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
      pDlgProgress->Progress();

      // delete all timers data (e.g. all reminders, which are only stored locally)
      pvrDatabase->DeleteTimers();
    }

    if (bResetClients)
    {
      pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
      pDlgProgress->Progress();

      // delete all clients data (e.g priorities, which are only stored locally)
      pvrDatabase->DeleteClients();
    }

    if (bResetChannels || bResetRecordings)
    {
      CVideoDatabase videoDatabase;

      if (videoDatabase.Open())
      {
        if (bResetChannels)
        {
          pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
          pDlgProgress->Progress();

          // delete all channel's entries (e.g. settings, bookmarks, stream details)
          videoDatabase.EraseAllForPath("pvr://channels/");
        }

        if (bResetRecordings)
        {
          pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
          pDlgProgress->Progress();

          // delete all recording's entries (e.g. settings, bookmarks, stream details)
          videoDatabase.EraseAllForPath(CPVRRecordingsPath::PATH_RECORDINGS);
        }

        videoDatabase.Close();
      }
    }

    // decrease db open refcounts; this actually closes dbs because refcounts drops to zero
    pvrDatabase->Close();
    epgDatabase->Close();

    CLog::LogFC(LOGDEBUG, LOGPVR, "{} database cleared", bResetEPGOnly ? "EPG" : "PVR and EPG");

    CLog::Log(LOGINFO, "Restarting the PVR Manager after {} database reset",
              bResetEPGOnly ? "EPG" : "PVR and EPG");
    CServiceBroker::GetPVRManager().Start();

    pDlgProgress->SetPercentage(100);
    pDlgProgress->Close();
    return true;
  }

  ParentalCheckResult CPVRGUIActions::CheckParentalLock(const std::shared_ptr<CPVRChannel>& channel) const
  {
    if (!CServiceBroker::GetPVRManager().IsParentalLocked(channel))
      return ParentalCheckResult::SUCCESS;

    ParentalCheckResult ret = CheckParentalPIN();

    if (ret == ParentalCheckResult::FAILED)
      CLog::LogF(LOGERROR, "Parental lock verification failed for channel '{}': wrong PIN entered.",
                 channel->ChannelName());

    return ret;
  }

  ParentalCheckResult CPVRGUIActions::CheckParentalPIN() const
  {
    if (!m_settings.GetBoolValue(CSettings::SETTING_PVRPARENTAL_ENABLED))
      return ParentalCheckResult::SUCCESS;

    std::string pinCode = m_settings.GetStringValue(CSettings::SETTING_PVRPARENTAL_PIN);
    if (pinCode.empty())
      return ParentalCheckResult::SUCCESS;

    InputVerificationResult ret = CGUIDialogNumeric::ShowAndVerifyInput(pinCode, g_localizeStrings.Get(19262), true); // "Parental control. Enter PIN:"

    if (ret == InputVerificationResult::SUCCESS)
    {
      CServiceBroker::GetPVRManager().RestartParentalTimer();
      return ParentalCheckResult::SUCCESS;
    }
    else if (ret == InputVerificationResult::FAILED)
    {
      HELPERS::ShowOKDialogText(CVariant{19264}, CVariant{19265}); // "Incorrect PIN", "The entered PIN was incorrect."
      return ParentalCheckResult::FAILED;
    }
    else
    {
      return ParentalCheckResult::CANCELED;
    }
  }

  bool CPVRGUIActions::CanSystemPowerdown(bool bAskUser /*= true*/) const
  {
    bool bReturn(true);
    if (CServiceBroker::GetPVRManager().IsStarted())
    {
      std::shared_ptr<CPVRTimerInfoTag> cause;
      if (!AllLocalBackendsIdle(cause))
      {
        if (bAskUser)
        {
          std::string text;

          if (cause)
          {
            if (cause->IsRecording())
            {
              text = StringUtils::Format(
                  g_localizeStrings.Get(19691), // "PVR is currently recording...."
                  cause->Title(), cause->ChannelName());
            }
            else
            {
              // Next event is due to a local recording or reminder.
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
                dueStr = StringUtils::Format(g_localizeStrings.Get(19694), mins);
              }
              else
              {
                // "about a minute"
                dueStr = g_localizeStrings.Get(19695);
              }

              text = StringUtils::Format(
                  cause->IsReminder()
                      ? g_localizeStrings.Get(19690) // "PVR has scheduled a reminder...."
                      : g_localizeStrings.Get(19692), // "PVR will start recording...."
                  cause->Title(), cause->ChannelName(), dueStr);
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
              dueStr = StringUtils::Format(g_localizeStrings.Get(19694), mins);
            }
            else
            {
              // "about a minute"
              dueStr = g_localizeStrings.Get(19695);
            }

            text = StringUtils::Format(g_localizeStrings.Get(19693), // "Daily wakeup is due in...."
                                       dueStr);
          }

          // Inform user about PVR being busy. Ask if user wants to powerdown anyway.
          bReturn =
              HELPERS::ShowYesNoDialogText(CVariant{19685}, // "Confirm shutdown"
                                           CVariant{text}, CVariant{222}, // "Shutdown anyway",
                                           CVariant{19696}, // "Cancel"
                                           10000) // timeout value before closing
              == HELPERS::DialogResponse::CHOICE_YES;
        }
        else
          bReturn = false; // do not powerdown (busy, but no user interaction requested).
      }
    }
    return bReturn;
  }

  bool CPVRGUIActions::AllLocalBackendsIdle(std::shared_ptr<CPVRTimerInfoTag>& causingEvent) const
  {
    // active recording on local backend?
    const std::vector<std::shared_ptr<CPVRTimerInfoTag>> activeRecordings = CServiceBroker::GetPVRManager().Timers()->GetActiveRecordings();
    for (const auto& timer : activeRecordings)
    {
      if (EventOccursOnLocalBackend(std::make_shared<CFileItem>(timer)))
      {
        causingEvent = timer;
        return false;
      }
    }

    // soon recording on local backend?
    if (IsNextEventWithinBackendIdleTime())
    {
      const std::shared_ptr<CPVRTimerInfoTag> timer = CServiceBroker::GetPVRManager().Timers()->GetNextActiveTimer(false);
      if (!timer)
      {
        // Next event is due to automatic daily wakeup of PVR!
        causingEvent.reset();
        return false;
      }

      if (EventOccursOnLocalBackend(std::make_shared<CFileItem>(timer)))
      {
        causingEvent = timer;
        return false;
      }
    }
    return true;
  }

  bool CPVRGUIActions::EventOccursOnLocalBackend(const CFileItemPtr& item) const
  {
    if (item && item->HasPVRTimerInfoTag())
    {
      const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(*item);
      if (client)
      {
        const std::string hostname = client->GetBackendHostname();
        if (!hostname.empty() && CServiceBroker::GetNetwork().IsLocalHost(hostname))
          return true;
      }
    }
    return false;
  }

  bool CPVRGUIActions::IsNextEventWithinBackendIdleTime() const
  {
    // timers going off soon?
    const CDateTime now(CDateTime::GetUTCDateTime());
    const CDateTimeSpan idle(0, 0, m_settings.GetIntValue(CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME), 0);
    const CDateTime next(CServiceBroker::GetPVRManager().Timers()->GetNextEventTime());
    const CDateTimeSpan delta(next - now);

    return (delta <= idle);
  }

  void CPVRGUIActions::SetSelectedItemPath(bool bRadio, const std::string& path)
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    if (bRadio)
      m_selectedItemPathRadio = path;
    else
      m_selectedItemPathTV = path;
  }

  std::string CPVRGUIActions::GetSelectedItemPath(bool bRadio) const
  {
    if (m_settings.GetBoolValue(CSettings::SETTING_PVRMANAGER_PRESELECTPLAYINGCHANNEL))
    {
      CPVRManager& mgr = CServiceBroker::GetPVRManager();

      // if preselect playing channel is activated, return the path of the playing channel, if any.
      const std::shared_ptr<CPVRChannel> playingChannel = mgr.PlaybackState()->GetPlayingChannel();
      if (playingChannel && playingChannel->IsRadio() == bRadio)
        return GetChannelGroupMember(playingChannel)->Path();

      const std::shared_ptr<CPVREpgInfoTag> playingTag = mgr.PlaybackState()->GetPlayingEpgTag();
      if (playingTag && playingTag->IsRadio() == bRadio)
      {
        const std::shared_ptr<CPVRChannel> channel =
            mgr.ChannelGroups()->GetChannelForEpgTag(playingTag);
        if (channel)
          return GetChannelGroupMember(channel)->Path();
      }
    }

    std::unique_lock<CCriticalSection> lock(m_critSection);
    return bRadio ? m_selectedItemPathRadio : m_selectedItemPathTV;
  }

  std::shared_ptr<CPVRChannelGroupMember> CPVRGUIActions::GetChannelGroupMember(
      const std::shared_ptr<CPVRChannel>& channel) const
  {
    std::shared_ptr<CPVRChannelGroupMember> groupMember;
    if (channel)
    {
      // first, try whether the channel is contained in the active channel group
      std::shared_ptr<CPVRChannelGroup> group =
          CServiceBroker::GetPVRManager().PlaybackState()->GetActiveChannelGroup(
              channel->IsRadio());
      if (group)
        groupMember = group->GetByUniqueID(channel->StorageId());

      // as fallback, obtain the member from the 'all channels' group
      if (!groupMember)
      {
        group = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAll(channel->IsRadio());
        if (group)
          groupMember = group->GetByUniqueID(channel->StorageId());
      }
    }
    return groupMember;
  }

  std::shared_ptr<CPVRChannelGroupMember> CPVRGUIActions::GetChannelGroupMember(
      const CFileItem& item) const
  {
    std::shared_ptr<CPVRChannelGroupMember> groupMember = item.GetPVRChannelGroupMemberInfoTag();

    if (!groupMember)
      groupMember = GetChannelGroupMember(CPVRItem(std::make_shared<CFileItem>(item)).GetChannel());

    return groupMember;
  }

  CPVRChannelNumberInputHandler& CPVRGUIActions::GetChannelNumberInputHandler()
  {
    // window/dialog specific input handler
    CPVRChannelNumberInputHandler* windowInputHandler = dynamic_cast<CPVRChannelNumberInputHandler*>(CServiceBroker::GetGUI()->GetWindowManager().GetWindow(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog()));
    if (windowInputHandler)
      return *windowInputHandler;

    // default
    return m_channelNumberInputHandler;
  }

  CPVRGUIChannelNavigator& CPVRGUIActions::GetChannelNavigator()
  {
    return m_channelNavigator;
  }

  void CPVRGUIActions::OnPlaybackStarted(const CFileItemPtr& item)
  {
    const std::shared_ptr<CPVRChannelGroupMember> groupMember = GetChannelGroupMember(*item);
    if (groupMember)
    {
      m_channelNavigator.SetPlayingChannel(groupMember);
      SetSelectedItemPath(groupMember->Channel()->IsRadio(), groupMember->Path());
    }
  }

  void CPVRGUIActions::OnPlaybackStopped(const CFileItemPtr& item)
  {
    if (item->HasPVRChannelInfoTag() || item->HasEPGInfoTag())
    {
      m_channelNavigator.ClearPlayingChannel();
    }
  }

  bool CPVRGUIActions::OnInfo(const std::shared_ptr<CFileItem>& item)
  {
    if (item->HasPVRRecordingInfoTag())
    {
      return ShowRecordingInfo(item);
    }
    else if (item->HasPVRChannelInfoTag() || item->HasPVRTimerInfoTag())
    {
      return ShowEPGInfo(item);
    }
    else if (item->HasEPGSearchFilter())
    {
      return EditSavedSearch(item);
    }
    return false;
  }

  void CPVRChannelSwitchingInputHandler::AppendChannelNumberCharacter(char cCharacter)
  {
    // special case. if only a single zero was typed in, switch to previously played channel.
    if (GetCurrentDigitCount() == 0 && cCharacter == '0')
    {
      SwitchToPreviousChannel();
      return;
    }

    CPVRChannelNumberInputHandler::AppendChannelNumberCharacter(cCharacter);
  }

  void CPVRChannelSwitchingInputHandler::GetChannelNumbers(std::vector<std::string>& channelNumbers)
  {
    const CPVRManager& pvrMgr = CServiceBroker::GetPVRManager();
    const std::shared_ptr<CPVRChannel> playingChannel = pvrMgr.PlaybackState()->GetPlayingChannel();
    if (playingChannel)
    {
      const std::shared_ptr<CPVRChannelGroup> group = pvrMgr.ChannelGroups()->GetGroupAll(playingChannel->IsRadio());
      if (group)
        group->GetChannelNumbers(channelNumbers);
    }
  }

  void CPVRChannelSwitchingInputHandler::OnInputDone()
  {
    CPVRChannelNumber channelNumber = GetChannelNumber();
    if (channelNumber.GetChannelNumber())
      SwitchToChannel(channelNumber);
  }

  void CPVRChannelSwitchingInputHandler::SwitchToChannel(const CPVRChannelNumber& channelNumber)
  {
    if (channelNumber.IsValid() && CServiceBroker::GetPVRManager().PlaybackState()->IsPlaying())
    {
      const std::shared_ptr<CPVRChannel> playingChannel = CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();
      if (playingChannel)
      {
        bool bRadio = playingChannel->IsRadio();
        const std::shared_ptr<CPVRChannelGroup> group =
            CServiceBroker::GetPVRManager().PlaybackState()->GetActiveChannelGroup(bRadio);

        if (channelNumber != group->GetChannelNumber(playingChannel))
        {
          // channel number present in active group?
          std::shared_ptr<CPVRChannelGroupMember> groupMember =
              group->GetByChannelNumber(channelNumber);

          if (!groupMember)
          {
            // channel number present in any group?
            const CPVRChannelGroups* groupAccess = CServiceBroker::GetPVRManager().ChannelGroups()->Get(bRadio);
            const std::vector<std::shared_ptr<CPVRChannelGroup>> groups = groupAccess->GetMembers(true);
            for (const auto& currentGroup : groups)
            {
              if (currentGroup == group) // we have already checked this group
                continue;

              groupMember = currentGroup->GetByChannelNumber(channelNumber);
              if (groupMember)
                break;
            }
          }

          if (groupMember)
          {
            CServiceBroker::GetAppMessenger()->PostMsg(
                TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                static_cast<void*>(new CAction(
                    ACTION_CHANNEL_SWITCH, static_cast<float>(channelNumber.GetChannelNumber()),
                    static_cast<float>(channelNumber.GetSubChannelNumber()))));
          }
        }
      }
    }
  }

  void CPVRChannelSwitchingInputHandler::SwitchToPreviousChannel()
  {
    const std::shared_ptr<CPVRPlaybackState> playbackState =
        CServiceBroker::GetPVRManager().PlaybackState();
    if (playbackState->IsPlaying())
    {
      const std::shared_ptr<CPVRChannel> playingChannel = playbackState->GetPlayingChannel();
      if (playingChannel)
      {
        const std::shared_ptr<CPVRChannelGroupMember> groupMember =
            playbackState->GetPreviousToLastPlayedChannelGroupMember(playingChannel->IsRadio());
        if (groupMember)
        {
          const CPVRChannelNumber channelNumber = groupMember->ChannelNumber();
          CServiceBroker::GetAppMessenger()->SendMsg(
              TMSG_GUI_ACTION, WINDOW_INVALID, -1,
              static_cast<void*>(new CAction(
                  ACTION_CHANNEL_SWITCH, static_cast<float>(channelNumber.GetChannelNumber()),
                  static_cast<float>(channelNumber.GetSubChannelNumber()))));
        }
      }
    }
  }

  } // namespace PVR

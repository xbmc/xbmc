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
#include "application/ApplicationEnums.h"
#include "cores/DataCacheCore.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogKaiToast.h"
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
#include "pvr/PVRStreamProperties.h"
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
#include "pvr/recordings/PVRRecording.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/recordings/PVRRecordingsPath.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/windows/GUIWindowPVRBase.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
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
  : m_settings({CSettings::SETTING_LOOKANDFEEL_STARTUPACTION,
                CSettings::SETTING_PVRMANAGER_PRESELECTPLAYINGCHANNEL,
                CSettings::SETTING_PVRPLAYBACK_CONFIRMCHANNELSWITCH,
                CSettings::SETTING_PVRPLAYBACK_SWITCHTOFULLSCREENCHANNELTYPES,
                CSettings::SETTING_PVRPARENTAL_PIN, CSettings::SETTING_PVRPARENTAL_ENABLED,
                CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME,
                CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME})
{
}

  std::string CPVRGUIActions::GetResumeLabel(const CFileItem& item) const
  {
    std::string resumeString;

    const std::shared_ptr<CPVRRecording> recording(CPVRItem(CFileItemPtr(new CFileItem(item))).GetRecording());
    if (recording && !recording->IsDeleted())
    {
      int positionInSeconds = lrint(recording->GetResumePoint().timeInSeconds);
      if (positionInSeconds > 0)
        resumeString = StringUtils::Format(
            g_localizeStrings.Get(12022),
            StringUtils::SecondsToTimeString(positionInSeconds, TIME_FORMAT_HH_MM_SS));
    }
    return resumeString;
  }

  bool CPVRGUIActions::CheckResumeRecording(const CFileItemPtr& item) const
  {
    bool bPlayIt(true);
    std::string resumeString(GetResumeLabel(*item));
    if (!resumeString.empty())
    {
      CContextButtons choices;
      choices.Add(CONTEXT_BUTTON_RESUME_ITEM, resumeString);
      choices.Add(CONTEXT_BUTTON_PLAY_ITEM, 12021); // Play from beginning
      int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
      if (choice > 0)
        item->SetStartOffset(choice == CONTEXT_BUTTON_RESUME_ITEM ? STARTOFFSET_RESUME : 0);
      else
        bPlayIt = false; // context menu cancelled
    }
    return bPlayIt;
  }

  bool CPVRGUIActions::ResumePlayRecording(const CFileItemPtr& item, bool bFallbackToPlay) const
  {
    bool bCanResume = !GetResumeLabel(*item).empty();
    if (bCanResume)
    {
      item->SetStartOffset(STARTOFFSET_RESUME);
    }
    else
    {
      if (bFallbackToPlay)
        item->SetStartOffset(0);
      else
        return false;
    }

    return PlayRecording(item, false);
  }

  void CPVRGUIActions::CheckAndSwitchToFullscreen(bool bFullscreen) const
  {
    CMediaSettings::GetInstance().SetMediaStartWindowed(!bFullscreen);

    if (bFullscreen)
    {
      CGUIMessage msg(GUI_MSG_FULLSCREEN, 0, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow());
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
    }
  }

  void CPVRGUIActions::StartPlayback(CFileItem* item,
                                     bool bFullscreen,
                                     const CPVRStreamProperties* epgProps) const
  {
    // Obtain dynamic playback url and properties from the respective pvr client
    const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(*item);
    if (client)
    {
      CPVRStreamProperties props;

      if (item->IsPVRChannel())
      {
        // If this was an EPG Tag to be played as live then PlayEpgTag() will create a channel
        // fileitem instead and pass the epg tags props so we use those and skip the client call
        if (epgProps)
          props = *epgProps;
        else
          client->GetChannelStreamProperties(item->GetPVRChannelInfoTag(), props);
      }
      else if (item->IsPVRRecording())
      {
        client->GetRecordingStreamProperties(item->GetPVRRecordingInfoTag(), props);
      }
      else if (item->IsEPG())
      {
        if (epgProps) // we already have props from PlayEpgTag()
          props = *epgProps;
        else
          client->GetEpgTagStreamProperties(item->GetEPGInfoTag(), props);
      }

      if (props.size())
      {
        const std::string url = props.GetStreamURL();
        if (!url.empty())
          item->SetDynPath(url);

        const std::string mime = props.GetStreamMimeType();
        if (!mime.empty())
        {
          item->SetMimeType(mime);
          item->SetContentLookup(false);
        }

        for (const auto& prop : props)
          item->SetProperty(prop.first, prop.second);
      }
    }

    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(item));
    CheckAndSwitchToFullscreen(bFullscreen);
  }

  bool CPVRGUIActions::PlayRecording(const CFileItemPtr& item, bool bCheckResume) const
  {
    const std::shared_ptr<CPVRRecording> recording(CPVRItem(item).GetRecording());
    if (!recording)
      return false;

    if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingRecording(recording))
    {
      CGUIMessage msg(GUI_MSG_FULLSCREEN, 0, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow());
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
      return true;
    }

    if (!bCheckResume || CheckResumeRecording(item))
    {
      CFileItem* itemToPlay = new CFileItem(recording);
      itemToPlay->SetStartOffset(item->GetStartOffset());
      StartPlayback(itemToPlay, true);
    }
    return true;
  }

  bool CPVRGUIActions::PlayEpgTag(const CFileItemPtr& item) const
  {
    const std::shared_ptr<CPVREpgInfoTag> epgTag(CPVRItem(item).GetEpgInfoTag());
    if (!epgTag)
      return false;

    if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingEpgTag(epgTag))
    {
      CGUIMessage msg(GUI_MSG_FULLSCREEN, 0,
                      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow());
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
      return true;
    }

    // Obtain dynamic playback url and properties from the respective pvr client
    const std::shared_ptr<CPVRClient> client =
        CServiceBroker::GetPVRManager().GetClient(epgTag->ClientID());
    if (!client)
      return false;

    CPVRStreamProperties props;
    client->GetEpgTagStreamProperties(epgTag, props);

    CFileItem* itemToPlay = nullptr;
    if (props.EPGPlaybackAsLive())
    {
      const std::shared_ptr<CPVRChannelGroupMember> groupMember = GetChannelGroupMember(*item);
      if (!groupMember)
        return false;

      itemToPlay = new CFileItem(groupMember);
    }
    else
    {
      itemToPlay = new CFileItem(epgTag);
    }

    StartPlayback(itemToPlay, true, &props);
    return true;
  }

  bool CPVRGUIActions::SwitchToChannel(const CFileItemPtr& item, bool bCheckResume) const
  {
    if (item->m_bIsFolder)
      return false;

    std::shared_ptr<CPVRRecording> recording;
    const std::shared_ptr<CPVRChannel> channel(CPVRItem(item).GetChannel());
    if (channel)
    {
      bool bSwitchToFullscreen = CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingChannel(channel);

      if (!bSwitchToFullscreen)
      {
        recording = CServiceBroker::GetPVRManager().Recordings()->GetRecordingForEpgTag(channel->GetEPGNow());
        bSwitchToFullscreen = recording && CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingRecording(recording);
      }

      if (bSwitchToFullscreen)
      {
        CGUIMessage msg(GUI_MSG_FULLSCREEN, 0, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow());
        CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
        return true;
      }
    }

    ParentalCheckResult result = channel ? CheckParentalLock(channel) : ParentalCheckResult::FAILED;
    if (result == ParentalCheckResult::SUCCESS)
    {
      // switch to channel or if recording present, ask whether to switch or play recording...
      if (!recording)
        recording = CServiceBroker::GetPVRManager().Recordings()->GetRecordingForEpgTag(channel->GetEPGNow());

      if (recording)
      {
        bool bCancel(false);
        bool bPlayRecording = CGUIDialogYesNo::ShowAndGetInput(CVariant{19687}, // "Play recording"
                                                               CVariant{""},
                                                               CVariant{12021}, // "Play from beginning"
                                                               CVariant{recording->m_strTitle},
                                                               bCancel,
                                                               CVariant{19000}, // "Switch to channel"
                                                               CVariant{19687}, // "Play recording"
                                                               0); // no autoclose
        if (bCancel)
          return false;

        if (bPlayRecording)
        {
          const CFileItemPtr recordingItem(new CFileItem(recording));
          return PlayRecording(recordingItem, bCheckResume);
        }
      }

      bool bFullscreen;
      switch (m_settings.GetIntValue(CSettings::SETTING_PVRPLAYBACK_SWITCHTOFULLSCREENCHANNELTYPES))
      {
        case 0: // never
          bFullscreen = false;
          break;
        case 1: // TV channels
          bFullscreen = !channel->IsRadio();
          break;
        case 2: // Radio channels
          bFullscreen = channel->IsRadio();
          break;
        case 3: // TV and radio channels
        default:
          bFullscreen = true;
          break;
      }
      const std::shared_ptr<CPVRChannelGroupMember> groupMember = GetChannelGroupMember(*item);
      if (!groupMember)
        return false;

      StartPlayback(new CFileItem(groupMember), bFullscreen);
      return true;
    }
    else if (result == ParentalCheckResult::FAILED)
    {
      const std::string channelName = channel ? channel->ChannelName() : g_localizeStrings.Get(19029); // Channel
      const std::string msg = StringUtils::Format(
          g_localizeStrings.Get(19035),
          channelName); // CHANNELNAME could not be played. Check the log for details.

      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(19166), msg); // PVR information
    }

    return false;
  }

  bool CPVRGUIActions::SwitchToChannel(PlaybackType type) const
  {
    std::shared_ptr<CPVRChannelGroupMember> groupMember;
    bool bIsRadio(false);

    // check if the desired PlaybackType is already playing,
    // and if not, try to grab the last played channel of this type
    switch (type)
    {
      case PlaybackTypeRadio:
      {
        if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingRadio())
          return true;

        const std::shared_ptr<CPVRChannelGroup> allGroup = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAllRadio();
        if (allGroup)
          groupMember = allGroup->GetLastPlayedChannelGroupMember();

        bIsRadio = true;
        break;
      }
      case PlaybackTypeTV:
      {
        if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingTV())
          return true;

        const std::shared_ptr<CPVRChannelGroup> allGroup = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAllTV();
        if (allGroup)
          groupMember = allGroup->GetLastPlayedChannelGroupMember();

        break;
      }
      default:
        if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlaying())
          return true;

        groupMember =
            CServiceBroker::GetPVRManager().ChannelGroups()->GetLastPlayedChannelGroupMember();
        break;
    }

    // if we have a last played channel, start playback
    if (groupMember)
    {
      return SwitchToChannel(std::make_shared<CFileItem>(groupMember), true);
    }
    else
    {
      // if we don't, find the active channel group of the demanded type and play it's first channel
      const std::shared_ptr<CPVRChannelGroup> channelGroup =
          CServiceBroker::GetPVRManager().PlaybackState()->GetActiveChannelGroup(bIsRadio);
      if (channelGroup)
      {
        // try to start playback of first channel in this group
        const std::vector<std::shared_ptr<CPVRChannelGroupMember>> groupMembers =
            channelGroup->GetMembers();
        if (!groupMembers.empty())
        {
          return SwitchToChannel(std::make_shared<CFileItem>(*groupMembers.begin()), true);
        }
      }
    }

    CLog::LogF(LOGERROR,
               "Could not determine {} channel to playback. No last played channel found, and "
               "first channel of active group could also not be determined.",
               bIsRadio ? "Radio" : "TV");

    CGUIDialogKaiToast::QueueNotification(
        CGUIDialogKaiToast::Error,
        g_localizeStrings.Get(19166), // PVR information
        StringUtils::Format(
            g_localizeStrings.Get(19035),
            g_localizeStrings.Get(
                bIsRadio ? 19021
                         : 19020))); // Radio/TV could not be played. Check the log for details.
    return false;
  }

  bool CPVRGUIActions::PlayChannelOnStartup() const
  {
    int iAction = m_settings.GetIntValue(CSettings::SETTING_LOOKANDFEEL_STARTUPACTION);
    if (iAction != STARTUP_ACTION_PLAY_TV &&
        iAction != STARTUP_ACTION_PLAY_RADIO)
      return false;

    bool playRadio = (iAction == STARTUP_ACTION_PLAY_RADIO);

    // get the last played channel or fallback to first channel of all channels group
    std::shared_ptr<CPVRChannelGroupMember> groupMember =
        CServiceBroker::GetPVRManager().PlaybackState()->GetLastPlayedChannelGroupMember(playRadio);

    if (!groupMember)
    {
      const std::shared_ptr<CPVRChannelGroup> group =
          CServiceBroker::GetPVRManager().ChannelGroups()->Get(playRadio)->GetGroupAll();
      auto channels = group->GetMembers();
      if (channels.empty())
        return false;

      groupMember = channels.front();
      if (!groupMember)
        return false;
    }

    CLog::Log(LOGINFO, "PVR is starting playback of channel '{}'",
              groupMember->Channel()->ChannelName());
    return SwitchToChannel(std::make_shared<CFileItem>(groupMember), true);
  }

  bool CPVRGUIActions::PlayMedia(const CFileItemPtr& item) const
  {
    CFileItemPtr pvrItem(item);
    if (URIUtils::IsPVRChannel(item->GetPath()) && !item->HasPVRChannelInfoTag())
    {
      const std::shared_ptr<CPVRChannelGroupMember> groupMember =
          CServiceBroker::GetPVRManager().ChannelGroups()->GetChannelGroupMemberByPath(
              item->GetPath());
      if (groupMember)
        pvrItem = std::make_shared<CFileItem>(groupMember);
    }
    else if (URIUtils::IsPVRRecording(item->GetPath()) && !item->HasPVRRecordingInfoTag())
      pvrItem = std::make_shared<CFileItem>(CServiceBroker::GetPVRManager().Recordings()->GetByPath(item->GetPath()));

    bool bCheckResume = true;
    if (item->HasProperty("check_resume"))
      bCheckResume = item->GetProperty("check_resume").asBoolean();

    if (pvrItem && pvrItem->HasPVRChannelInfoTag())
    {
      return SwitchToChannel(pvrItem, bCheckResume);
    }
    else if (pvrItem && pvrItem->HasPVRRecordingInfoTag())
    {
      return PlayRecording(pvrItem, bCheckResume);
    }

    return false;
  }

  bool CPVRGUIActions::HideChannel(const CFileItemPtr& item) const
  {
    const std::shared_ptr<CPVRChannel> channel(item->GetPVRChannelInfoTag());

    if (!channel)
      return false;

    if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{19054}, // "Hide channel"
                                          CVariant{19039}, // "Are you sure you want to hide this channel?"
                                          CVariant{""},
                                          CVariant{channel->ChannelName()}))
      return false;

    if (!CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAll(channel->IsRadio())->RemoveFromGroup(channel))
      return false;

    CGUIWindowPVRBase* pvrWindow = dynamic_cast<CGUIWindowPVRBase*>(CServiceBroker::GetGUI()->GetWindowManager().GetWindow(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow()));
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

  void CPVRGUIActions::SeekForward()
  {
    time_t playbackStartTime = CServiceBroker::GetDataCacheCore().GetStartTime();
    if (playbackStartTime > 0)
    {
      const std::shared_ptr<CPVRChannel> playingChannel = CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();
      if (playingChannel)
      {
        time_t nextTime = 0;
        std::shared_ptr<CPVREpgInfoTag> next = playingChannel->GetEPGNext();
        if (next)
        {
          next->StartAsUTC().GetAsTime(nextTime);
        }
        else
        {
          // if there is no next event, jump to end of currently playing event
          next = playingChannel->GetEPGNow();
          if (next)
            next->EndAsUTC().GetAsTime(nextTime);
        }

        int64_t seekTime = 0;
        if (nextTime != 0)
        {
          seekTime = (nextTime - playbackStartTime) * 1000;
        }
        else
        {
          // no epg; jump to end of buffer
          seekTime = CServiceBroker::GetDataCacheCore().GetMaxTime();
        }
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_SEEK_TIME, seekTime);
      }
    }
  }

  void CPVRGUIActions::SeekBackward(unsigned int iThreshold)
  {
    time_t playbackStartTime = CServiceBroker::GetDataCacheCore().GetStartTime();
    if (playbackStartTime > 0)
    {
      const std::shared_ptr<CPVRChannel> playingChannel = CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();
      if (playingChannel)
      {
        time_t prevTime = 0;
        std::shared_ptr<CPVREpgInfoTag> prev = playingChannel->GetEPGNow();
        if (prev)
        {
          prev->StartAsUTC().GetAsTime(prevTime);

          // if playback time of current event is above threshold jump to start of current event
          int64_t playTime = CServiceBroker::GetDataCacheCore().GetPlayTime() / 1000;
          if ((playbackStartTime + playTime - prevTime) <= iThreshold)
          {
            // jump to start of previous event
            prevTime = 0;
            prev = playingChannel->GetEPGPrevious();
            if (prev)
              prev->StartAsUTC().GetAsTime(prevTime);
          }
        }

        int64_t seekTime = 0;
        if (prevTime != 0)
        {
          seekTime = (prevTime - playbackStartTime) * 1000;
        }
        else
        {
          // no epg; jump to begin of buffer
          seekTime = CServiceBroker::GetDataCacheCore().GetMinTime();
        }
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_SEEK_TIME, seekTime);
      }
    }
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

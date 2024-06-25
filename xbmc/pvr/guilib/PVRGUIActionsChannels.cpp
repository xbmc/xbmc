/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIActionsChannels.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "pvr/PVRItem.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/windows/GUIWindowPVRBase.h"
#include "settings/Settings.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

using namespace PVR;
using namespace KODI::MESSAGING;

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
  const std::shared_ptr<const CPVRChannel> playingChannel =
      pvrMgr.PlaybackState()->GetPlayingChannel();
  if (playingChannel)
  {
    const std::shared_ptr<const CPVRChannelGroup> group =
        pvrMgr.ChannelGroups()->GetGroupAll(playingChannel->IsRadio());
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

namespace
{
void UpdateActiveGroup(const std::shared_ptr<CPVRChannelGroupMember>& newChannel)
{
  const std::shared_ptr<CPVRPlaybackState> playbackState{
      CServiceBroker::GetPVRManager().PlaybackState()};
  const std::shared_ptr<const CPVRChannelGroupsContainer> groups{
      CServiceBroker::GetPVRManager().ChannelGroups()};
  const std::shared_ptr<CPVRChannelGroup> group{
      groups->Get(newChannel->IsRadio())->GetById(newChannel->GroupID())};

  // Switch group if new channel is not in the active group.
  if (group && group != playbackState->GetActiveChannelGroup(newChannel->IsRadio()))
    playbackState->SetActiveChannelGroup(group);
}

void TriggerChannelSwitchAction(const CPVRChannelNumber& channelNumber)
{
  CServiceBroker::GetAppMessenger()->SendMsg(
      TMSG_GUI_ACTION, WINDOW_INVALID, -1,
      static_cast<void*>(new CAction(ACTION_CHANNEL_SWITCH,
                                     static_cast<float>(channelNumber.GetChannelNumber()),
                                     static_cast<float>(channelNumber.GetSubChannelNumber()))));
}
} // unnamed namespace

void CPVRChannelSwitchingInputHandler::SwitchToChannel(const CPVRChannelNumber& channelNumber)
{
  if (channelNumber.IsValid() && CServiceBroker::GetPVRManager().PlaybackState()->IsPlaying())
  {
    const std::shared_ptr<const CPVRChannel> playingChannel =
        CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();
    if (playingChannel)
    {
      bool bRadio = playingChannel->IsRadio();
      const std::shared_ptr<const CPVRChannelGroup> group =
          CServiceBroker::GetPVRManager().PlaybackState()->GetActiveChannelGroup(bRadio);

      if (channelNumber != group->GetChannelNumber(playingChannel))
      {
        // channel number present in active group?
        std::shared_ptr<CPVRChannelGroupMember> groupMember =
            group->GetByChannelNumber(channelNumber);

        if (!groupMember)
        {
          // channel number present in any group?
          const CPVRChannelGroups* groupAccess =
              CServiceBroker::GetPVRManager().ChannelGroups()->Get(bRadio);
          const std::vector<std::shared_ptr<CPVRChannelGroup>> groups =
              groupAccess->GetMembers(true);
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
          UpdateActiveGroup(groupMember);
          TriggerChannelSwitchAction(channelNumber);
        }
      }
    }
  }
}

void CPVRChannelSwitchingInputHandler::SwitchToPreviousChannel()
{
  const std::shared_ptr<const CPVRPlaybackState> playbackState =
      CServiceBroker::GetPVRManager().PlaybackState();
  if (playbackState->IsPlaying())
  {
    const std::shared_ptr<const CPVRChannel> playingChannel = playbackState->GetPlayingChannel();
    if (playingChannel)
    {
      const std::shared_ptr<CPVRChannelGroupMember> groupMember =
          playbackState->GetPreviousToLastPlayedChannelGroupMember(playingChannel->IsRadio());
      if (groupMember)
      {
        UpdateActiveGroup(groupMember);
        TriggerChannelSwitchAction(groupMember->ChannelNumber());
      }
    }
  }
}

CPVRGUIActionsChannels::CPVRGUIActionsChannels()
  : m_settings({CSettings::SETTING_PVRMANAGER_PRESELECTPLAYINGCHANNEL})
{
  RegisterChannelNumberInputHandler(&m_channelNumberInputHandler);
}

CPVRGUIActionsChannels::~CPVRGUIActionsChannels()
{
  DeregisterChannelNumberInputHandler(&m_channelNumberInputHandler);
}

void CPVRGUIActionsChannels::RegisterChannelNumberInputHandler(
    CPVRChannelNumberInputHandler* handler)
{
  if (handler)
    handler->Events().Subscribe(this, &CPVRGUIActionsChannels::Notify);
}

void CPVRGUIActionsChannels::DeregisterChannelNumberInputHandler(
    CPVRChannelNumberInputHandler* handler)
{
  if (handler)
    handler->Events().Unsubscribe(this);
}

void CPVRGUIActionsChannels::Notify(const PVRChannelNumberInputChangedEvent& event)
{
  m_events.Publish(event);
}

bool CPVRGUIActionsChannels::HideChannel(const CFileItem& item) const
{
  const auto groupMember = item.GetPVRChannelGroupMemberInfoTag();

  if (!groupMember)
    return false;

  const auto channel = groupMember->Channel();

  if (!CGUIDialogYesNo::ShowAndGetInput(
          CVariant{19054}, // "Hide channel"
          CVariant{19039}, // "Are you sure you want to hide this channel?"
          CVariant{""}, CVariant{channel->ChannelName()}))
    return false;

  const auto groups{CServiceBroker::GetPVRManager().ChannelGroups()};
  if (!groups->Get(channel->IsRadio())
           ->RemoveFromGroup(groups->GetGroupAll(channel->IsRadio()), groupMember))
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

bool CPVRGUIActionsChannels::StartChannelScan()
{
  return StartChannelScan(PVR_INVALID_CLIENT_ID);
}

bool CPVRGUIActionsChannels::StartChannelScan(int clientId)
{
  if (!CServiceBroker::GetPVRManager().IsStarted() || IsRunningChannelScan())
    return false;

  std::shared_ptr<CPVRClient> scanClient;
  std::vector<std::shared_ptr<CPVRClient>> possibleScanClients =
      CServiceBroker::GetPVRManager().Clients()->GetClientsSupportingChannelScan();
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
    CGUIDialogSelect* pDialog =
        CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
            WINDOW_DIALOG_SELECT);
    if (!pDialog)
    {
      CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT!");
      m_bChannelScanRunning = false;
      return false;
    }

    pDialog->Reset();
    pDialog->SetHeading(CVariant{19119}); // "On which backend do you want to search?"

    for (const auto& client : possibleScanClients)
      pDialog->Add(client->GetFullClientName());

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
    HELPERS::ShowOKDialogText(
        CVariant{19033}, // "Information"
        CVariant{19192}); // "None of the connected PVR backends supports scanning for channels."
    m_bChannelScanRunning = false;
    return false;
  }

  /* start the channel scan */
  CLog::LogFC(LOGDEBUG, LOGPVR, "Starting to scan for channels on client {}",
              scanClient->GetFullClientName());
  auto start = std::chrono::steady_clock::now();

  /* do the scan */
  if (scanClient->StartChannelScan() != PVR_ERROR_NO_ERROR)
    HELPERS::ShowOKDialogText(CVariant{257}, // "Error"
                              CVariant{19193}); // "The channel scan can't be started."

  auto end = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  CLog::LogFC(LOGDEBUG, LOGPVR, "Channel scan finished after {} ms", duration.count());

  m_bChannelScanRunning = false;
  return true;
}

std::shared_ptr<CPVRChannelGroupMember> CPVRGUIActionsChannels::GetChannelGroupMember(
    const std::shared_ptr<const CPVRChannel>& channel) const
{
  if (!channel)
    return {};

  std::shared_ptr<CPVRChannelGroupMember> groupMember;

  // first, try whether the channel is contained in the active channel group, except
  // if a window is active which never uses the active channel group, e.g. Timers window
  const int activeWindowID = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();

  static std::vector<int> windowIDs = {
      WINDOW_TV_RECORDINGS,    WINDOW_TV_TIMERS,    WINDOW_TV_TIMER_RULES,    WINDOW_TV_SEARCH,
      WINDOW_RADIO_RECORDINGS, WINDOW_RADIO_TIMERS, WINDOW_RADIO_TIMER_RULES, WINDOW_RADIO_SEARCH,
  };

  if (std::find(windowIDs.cbegin(), windowIDs.cend(), activeWindowID) == windowIDs.cend())
  {
    const std::shared_ptr<const CPVRChannelGroup> group =
        CServiceBroker::GetPVRManager().PlaybackState()->GetActiveChannelGroup(channel->IsRadio());
    if (group)
      groupMember = group->GetByUniqueID(channel->StorageId());
  }

  // as fallback, obtain the member from the 'all channels' group
  if (!groupMember)
  {
    const std::shared_ptr<const CPVRChannelGroup> group =
        CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAll(channel->IsRadio());
    if (group)
      groupMember = group->GetByUniqueID(channel->StorageId());
  }

  return groupMember;
}

std::shared_ptr<CPVRChannelGroupMember> CPVRGUIActionsChannels::GetChannelGroupMember(
    const CFileItem& item) const
{
  std::shared_ptr<CPVRChannelGroupMember> groupMember = item.GetPVRChannelGroupMemberInfoTag();

  if (!groupMember)
    groupMember = GetChannelGroupMember(CPVRItem(std::make_shared<CFileItem>(item)).GetChannel());

  return groupMember;
}

CPVRChannelNumberInputHandler& CPVRGUIActionsChannels::GetChannelNumberInputHandler()
{
  // window/dialog specific input handler
  CPVRChannelNumberInputHandler* windowInputHandler = dynamic_cast<CPVRChannelNumberInputHandler*>(
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow(
          CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog()));
  if (windowInputHandler)
    return *windowInputHandler;

  // default
  return m_channelNumberInputHandler;
}

CPVRGUIChannelNavigator& CPVRGUIActionsChannels::GetChannelNavigator()
{
  return m_channelNavigator;
}

void CPVRGUIActionsChannels::OnPlaybackStarted(const CFileItem& item)
{
  const std::shared_ptr<CPVRChannelGroupMember> groupMember = GetChannelGroupMember(item);
  if (groupMember)
  {
    m_channelNavigator.SetPlayingChannel(groupMember);
    SetSelectedChannelPath(groupMember->Channel()->IsRadio(), groupMember->Path());
  }
}

void CPVRGUIActionsChannels::OnPlaybackStopped(const CFileItem& item)
{
  if (item.HasPVRChannelInfoTag() || item.HasEPGInfoTag())
  {
    m_channelNavigator.ClearPlayingChannel();
  }
}

void CPVRGUIActionsChannels::SetSelectedChannelPath(bool bRadio, const std::string& path)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (bRadio)
    m_selectedChannelPathRadio = path;
  else
    m_selectedChannelPathTV = path;
}

std::string CPVRGUIActionsChannels::GetSelectedChannelPath(bool bRadio) const
{
  if (m_settings.GetBoolValue(CSettings::SETTING_PVRMANAGER_PRESELECTPLAYINGCHANNEL))
  {
    CPVRManager& mgr = CServiceBroker::GetPVRManager();

    // if preselect playing channel is activated, return the path of the playing channel, if any.
    const std::shared_ptr<const CPVRChannelGroupMember> playingChannel =
        mgr.PlaybackState()->GetPlayingChannelGroupMember();
    if (playingChannel && playingChannel->IsRadio() == bRadio)
      return GetChannelGroupMember(playingChannel->Channel())->Path();

    const std::shared_ptr<const CPVREpgInfoTag> playingTag =
        mgr.PlaybackState()->GetPlayingEpgTag();
    if (playingTag && playingTag->IsRadio() == bRadio)
    {
      const std::shared_ptr<const CPVRChannel> channel =
          mgr.ChannelGroups()->GetChannelForEpgTag(playingTag);
      if (channel)
        return GetChannelGroupMember(channel)->Path();
    }
  }

  std::unique_lock<CCriticalSection> lock(m_critSection);
  return bRadio ? m_selectedChannelPathRadio : m_selectedChannelPathTV;
}

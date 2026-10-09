/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRChannelsOSD.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "pvr/guilib/PVRGUIActionsPlayback.h"
#include "pvr/providers/PVRProvider.h"
#include "pvr/providers/PVRProviders.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

using namespace PVR;

using namespace std::chrono_literals;

namespace
{
constexpr auto MAX_INVALIDATION_FREQUENCY = 2000ms; // limit to one invalidation per X milliseconds

bool IsProviderChannel(const CPVRChannel& channel, const CPVRProvider& provider)
{
  return channel.ClientID() == provider.GetClientId() &&
         channel.ClientProviderUid() == provider.GetUniqueId();
}

bool HasProviderChannel(const CPVRChannelGroup& group, const CPVRProvider& provider)
{
  return std::ranges::any_of(group.GetMembers(CPVRChannelGroup::Include::ONLY_VISIBLE),
                             [&provider](const auto& member)
                             { return IsProviderChannel(*member->Channel(), provider); });
}

struct ProviderGroup
{
  std::shared_ptr<CPVRProvider> provider;
  std::shared_ptr<CPVRChannelGroup> group;
};

std::vector<ProviderGroup> GetProviderGroups(const CPVRChannelGroups& groups,
                                             const CPVRProviders& providers)
{
  std::vector<ProviderGroup> result;
  const auto allChannels = groups.GetGroupAll();
  if (!allChannels)
    return result;

  const auto visibleGroups = groups.GetMembers(true);
  for (const auto& provider : providers.GetProviders())
  {
    if (!HasProviderChannel(*allChannels, *provider))
      continue;

    result.push_back({provider, allChannels});
    for (const auto& group : visibleGroups)
    {
      if (group != allChannels &&
          group->GroupType() != PVR_GROUP_TYPE_SYSTEM_ALL_CHANNELS_SINGLE_CLIENT &&
          HasProviderChannel(*group, *provider))
        result.push_back({provider, group});
    }
  }

  return result;
}

} // unnamed namespace

CGUIDialogPVRChannelsOSD::CGUIDialogPVRChannelsOSD()
  : CGUIDialogPVRItemsViewBase(WINDOW_DIALOG_PVR_OSD_CHANNELS, "DialogPVRChannelsOSD.xml")
{
  CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().RegisterChannelNumberInputHandler(this);
}

CGUIDialogPVRChannelsOSD::~CGUIDialogPVRChannelsOSD()
{
  auto& mgr = CServiceBroker::GetPVRManager();
  mgr.Events().Unsubscribe(this);
  mgr.Get<PVR::GUI::Channels>().DeregisterChannelNumberInputHandler(this);
}

bool CGUIDialogPVRChannelsOSD::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_REFRESH_LIST)
  {
    switch (static_cast<PVREvent>(message.GetParam1()))
    {
      using enum PVR::PVREvent;

      case CurrentItem:
        m_viewControl.SetItems(*m_vecItems);
        return true;

      case Epg:
      case EpgContainer:
      case EpgActiveItem:
        if (IsActive())
          SetInvalid();
        return true;

      default:
        break;
    }
  }
  return CGUIDialogPVRItemsViewBase::OnMessage(message);
}

void CGUIDialogPVRChannelsOSD::OnInitWindow()
{
  if (!CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingTV() &&
      !CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingRadio())
  {
    Close();
    return;
  }

  m_useProviderGroups = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
      CSettings::SETTING_PVRMENU_PROVIDERCHANNELOSD);
  Init();
  m_provider.reset();
  Update();
  CGUIDialogPVRItemsViewBase::OnInitWindow();
}

void CGUIDialogPVRChannelsOSD::OnDeinitWindow(int nextWindowID)
{
  if (m_group)
  {
    CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().SetSelectedChannelPath(
        m_group->IsRadio(), m_viewControl.GetSelectedItemPath());

    // next OnInitWindow will set the group which is then selected
    m_group.reset();
  }
  m_provider.reset();

  CGUIDialogPVRItemsViewBase::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogPVRChannelsOSD::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    case ACTION_SELECT_ITEM:
    case ACTION_MOUSE_LEFT_CLICK:
    {
      // If direct channel number input is active, select the entered channel.
      if (CServiceBroker::GetPVRManager()
              .Get<PVR::GUI::Channels>()
              .GetChannelNumberInputHandler()
              .CheckInputAndExecuteAction())
        return true;

      if (m_viewControl.HasControl(GetFocusedControlID()))
      {
        // Switch to channel
        GotoChannel(m_viewControl.GetSelectedItem());
        return true;
      }
      break;
    }
    case ACTION_PREVIOUS_CHANNELGROUP:
    case ACTION_NEXT_CHANNELGROUP:
    {
      // save control states and currently selected item of group
      SaveControlStates();

      // switch to next or previous group
      CPVRManager& pvrMgr = CServiceBroker::GetPVRManager();
      const auto groups = pvrMgr.ChannelGroups()->Get(m_group->IsRadio());
      const bool next = action.GetID() == ACTION_NEXT_CHANNELGROUP;
      if (!m_useProviderGroups)
      {
        m_group = next ? groups->GetNextGroup(*m_group) : groups->GetPreviousGroup(*m_group);
      }
      else
      {
        const auto providerGroups = GetProviderGroups(*groups, *pvrMgr.Providers());
        if (providerGroups.empty())
        {
          m_provider.reset();
          m_group = next ? groups->GetNextGroup(*m_group) : groups->GetPreviousGroup(*m_group);
        }
        else
        {
          const auto it = std::ranges::find_if(
              providerGroups,
              [this](const ProviderGroup& entry)
              {
                return m_provider && entry.provider->GetClientId() == m_provider->GetClientId() &&
                       entry.provider->GetUniqueId() == m_provider->GetUniqueId() &&
                       entry.group->GroupID() == m_group->GroupID();
              });
          const size_t index = it == providerGroups.end()
                                   ? (next ? providerGroups.size() - 1 : 0)
                                   : static_cast<size_t>(std::distance(providerGroups.begin(), it));
          const auto& selected = providerGroups[(index + (next ? 1 : providerGroups.size() - 1)) %
                                                providerGroups.size()];
          m_provider = selected.provider;
          m_group = selected.group;
        }
      }

      pvrMgr.PlaybackState()->SetActiveChannelGroup(m_group);
      Init();
      Update();

      // restore control states and previously selected item of group
      RestoreControlStates();
      return true;
    }
    case REMOTE_0:
    case REMOTE_1:
    case REMOTE_2:
    case REMOTE_3:
    case REMOTE_4:
    case REMOTE_5:
    case REMOTE_6:
    case REMOTE_7:
    case REMOTE_8:
    case REMOTE_9:
    {
      AppendChannelNumberCharacter(static_cast<char>(action.GetID() - REMOTE_0) + '0');
      return true;
    }
    case ACTION_CHANNEL_NUMBER_SEP:
    {
      AppendChannelNumberCharacter(CPVRChannelNumber::SEPARATOR);
      return true;
    }
    default:
      break;
  }

  return CGUIDialogPVRItemsViewBase::OnAction(action);
}

void CGUIDialogPVRChannelsOSD::Update()
{
  CPVRManager& pvrMgr = CServiceBroker::GetPVRManager();
  SetProperty("PVRProviderName", "");
  SetProperty("PVRGroupName", "");
  pvrMgr.Events().Subscribe(this,
                            [this](const PVREvent& event)
                            {
                              const CGUIMessage m(GUI_MSG_REFRESH_LIST, GetID(), 0,
                                                  static_cast<int>(event));
                              CServiceBroker::GetAppMessenger()->SendGUIMessage(m);
                            });

  const std::shared_ptr<const CPVRChannel> channel = pvrMgr.PlaybackState()->GetPlayingChannel();
  if (channel)
  {
    if (m_useProviderGroups && !m_provider)
      m_provider = channel->GetProvider();
    const auto& provider = m_provider;
    if (m_useProviderGroups && provider)
    {
      std::string providerName{provider->GetName()};
      if (provider->IsClientProvider())
      {
        if (const auto client = pvrMgr.GetClient(provider->GetClientId()))
        {
          const std::string instanceName{client->GetInstanceName()};
          if (!instanceName.empty())
            providerName = instanceName;
        }
      }
      SetProperty("PVRProviderName", providerName);
    }
    else if (m_useProviderGroups)
    {
      if (const auto client = pvrMgr.GetClient(channel->ClientID()))
        SetProperty("PVRProviderName", client->GetInstanceName());
    }

    std::shared_ptr<CPVRChannelGroup> group =
        m_useProviderGroups && m_group
            ? m_group
            : pvrMgr.PlaybackState()->GetActiveChannelGroup(channel->IsRadio());
    if (m_useProviderGroups && group && provider && !HasProviderChannel(*group, *provider))
      group = pvrMgr.ChannelGroups()->Get(channel->IsRadio())->GetGroupAll();
    if (m_group && m_group != group)
      m_group = group;
    if (group)
    {
      if (m_useProviderGroups)
      {
        if (group->GroupType() == PVR_GROUP_TYPE_SYSTEM_ALL_CHANNELS_ALL_CLIENTS ||
            group->GroupType() == PVR_GROUP_TYPE_SYSTEM_ALL_CHANNELS_SINGLE_CLIENT)
          SetProperty("PVRGroupName",
                      pvrMgr.ChannelGroups()->Get(channel->IsRadio())->GetGroupAll()->GroupName());
        else
          SetProperty("PVRGroupName", group->GroupName());
      }

      const std::vector<std::shared_ptr<CPVRChannelGroupMember>> groupMembers =
          group->GetMembers(CPVRChannelGroup::Include::ONLY_VISIBLE);
      for (const auto& groupMember : groupMembers)
      {
        if (m_useProviderGroups && provider &&
            !IsProviderChannel(*groupMember->Channel(), *provider))
          continue;
        m_vecItems->Add(std::make_shared<CFileItem>(groupMember));
      }

      m_viewControl.SetItems(*m_vecItems);

      if (!m_group)
      {
        m_group = group;
        m_viewControl.SetSelectedItem(
            pvrMgr.Get<PVR::GUI::Channels>().GetSelectedChannelPath(channel->IsRadio()));
        SaveSelectedItemPath(group->GroupID());
      }
    }
  }
}

void CGUIDialogPVRChannelsOSD::SetInvalid()
{
  if (m_refreshTimeout.IsTimePast())
  {
    for (const auto& item : *m_vecItems)
      item->SetInvalid();

    CGUIDialogPVRItemsViewBase::SetInvalid();
    m_refreshTimeout.Set(MAX_INVALIDATION_FREQUENCY);
  }
}

void CGUIDialogPVRChannelsOSD::SaveControlStates()
{
  CGUIDialogPVRItemsViewBase::SaveControlStates();

  if (m_group)
    SaveSelectedItemPath(m_group->GroupID());
}

void CGUIDialogPVRChannelsOSD::RestoreControlStates()
{
  CGUIDialogPVRItemsViewBase::RestoreControlStates();

  if (m_group)
  {
    const std::string path = GetLastSelectedItemPath(m_group->GroupID());
    if (path.empty())
      m_viewControl.SetSelectedItem(0);
    else
      m_viewControl.SetSelectedItem(path);
  }
}

void CGUIDialogPVRChannelsOSD::GotoChannel(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return;

  // Preserve the item before closing self, because this will clear m_vecItems
  const std::shared_ptr<CFileItem> item = m_vecItems->Get(iItem);

  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_PVRMENU_CLOSECHANNELOSDONSWITCH))
    Close();

  CServiceBroker::GetPVRManager().Get<PVR::GUI::Playback>().SwitchToChannel(*item);
}

void CGUIDialogPVRChannelsOSD::SaveSelectedItemPath(int iGroupID)
{
  const int clientId = m_provider ? m_provider->GetClientId() : -1;
  const int providerUid = m_provider ? m_provider->GetUniqueId() : PVR_PROVIDER_INVALID_UID;
  m_groupSelectedItemPaths[{clientId, providerUid, iGroupID}] = m_viewControl.GetSelectedItemPath();
}

std::string CGUIDialogPVRChannelsOSD::GetLastSelectedItemPath(int iGroupID) const
{
  const int clientId = m_provider ? m_provider->GetClientId() : -1;
  const int providerUid = m_provider ? m_provider->GetUniqueId() : PVR_PROVIDER_INVALID_UID;
  const auto it = m_groupSelectedItemPaths.find({clientId, providerUid, iGroupID});
  if (it != m_groupSelectedItemPaths.end())
    return it->second;

  return std::string();
}

void CGUIDialogPVRChannelsOSD::GetChannelNumbers(std::vector<std::string>& channelNumbers)
{
  if (m_group)
    m_group->GetChannelNumbers(channelNumbers);
}

void CGUIDialogPVRChannelsOSD::OnInputDone()
{
  const CPVRChannelNumber channelNumber = GetChannelNumber();
  if (channelNumber.IsValid())
  {
    int itemIndex = 0;
    for (const CFileItemPtr& channel : *m_vecItems)
    {
      if (channel->GetPVRChannelGroupMemberInfoTag()->ChannelNumber() == channelNumber)
      {
        m_viewControl.SetSelectedItem(itemIndex);
        return;
      }
      ++itemIndex;
    }
  }
}

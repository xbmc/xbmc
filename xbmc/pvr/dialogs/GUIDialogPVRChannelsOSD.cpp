/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRChannelsOSD.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "GUIInfoManager.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgContainer.h"

using namespace PVR;
using namespace KODI::MESSAGING;

#define MAX_INVALIDATION_FREQUENCY 2000 // limit to one invalidation per X milliseconds

CGUIDialogPVRChannelsOSD::CGUIDialogPVRChannelsOSD()
: CGUIDialogPVRItemsViewBase(WINDOW_DIALOG_PVR_OSD_CHANNELS, "DialogPVRChannelsOSD.xml")
{
}

CGUIDialogPVRChannelsOSD::~CGUIDialogPVRChannelsOSD()
{
  CServiceBroker::GetGUI()->GetInfoManager().UnregisterObserver(this);
  CServiceBroker::GetPVRManager().EpgContainer().UnregisterObserver(this);
}

bool CGUIDialogPVRChannelsOSD::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_REFRESH_LIST)
  {
    switch (message.GetParam1())
    {
      case ObservableMessageCurrentItem:
        m_viewControl.SetItems(*m_vecItems);
        return true;
      case ObservableMessageEpg:
      case ObservableMessageEpgContainer:
      case ObservableMessageEpgActiveItem:
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
  if (!CServiceBroker::GetPVRManager().IsPlayingTV() && !CServiceBroker::GetPVRManager().IsPlayingRadio())
  {
    Close();
    return;
  }

  Init();
  Update();
  CGUIDialogPVRItemsViewBase::OnInitWindow();
}

void CGUIDialogPVRChannelsOSD::OnDeinitWindow(int nextWindowID)
{
  if (m_group)
  {
    CServiceBroker::GetPVRManager().GUIActions()->SetSelectedItemPath(m_group->IsRadio(), m_viewControl.GetSelectedItemPath());

    // next OnInitWindow will set the group which is then selected
    m_group.reset();
  }

  CGUIDialogPVRItemsViewBase::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogPVRChannelsOSD::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
    case ACTION_SELECT_ITEM:
    case ACTION_MOUSE_LEFT_CLICK:
    {
      // If direct channel number input is active, select the entered channel.
      if (CServiceBroker::GetPVRManager().GUIActions()->GetChannelNumberInputHandler().CheckInputAndExecuteAction())
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
      const CPVRChannelGroups* groups = CServiceBroker::GetPVRManager().ChannelGroups()->Get(m_group->IsRadio());
      const std::shared_ptr<CPVRChannelGroup> nextGroup = action.GetID() == ACTION_NEXT_CHANNELGROUP
                                                        ? groups->GetNextGroup(*m_group)
                                                        : groups->GetPreviousGroup(*m_group);
      CServiceBroker::GetPVRManager().SetPlayingGroup(nextGroup);
      m_group = nextGroup;
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
      AppendChannelNumberCharacter((action.GetID() - REMOTE_0) + '0');
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
  CServiceBroker::GetGUI()->GetInfoManager().RegisterObserver(this);

  CPVRManager& pvrMgr = CServiceBroker::GetPVRManager();
  pvrMgr.EpgContainer().RegisterObserver(this);

  const CPVRChannelPtr channel = pvrMgr.GetPlayingChannel();
  if (channel)
  {
    const CPVRChannelGroupPtr group = pvrMgr.GetPlayingGroup(channel->IsRadio());
    if (group)
    {
      group->GetMembers(*m_vecItems);
      m_viewControl.SetItems(*m_vecItems);

      if (!m_group)
      {
        m_group = group;
        m_viewControl.SetSelectedItem(pvrMgr.GUIActions()->GetSelectedItemPath(channel->IsRadio()));
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

void CGUIDialogPVRChannelsOSD::GotoChannel(int item)
{
  if (item < 0 || item >= m_vecItems->Size())
    return;

  // Preserve the item before closing self, because this will clear m_vecItems
  const CFileItemPtr itemptr = m_vecItems->Get(item);
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRMENU_CLOSECHANNELOSDONSWITCH))
    Close();

  CServiceBroker::GetPVRManager().GUIActions()->SwitchToChannel(itemptr, true /* bCheckResume */);
}

void CGUIDialogPVRChannelsOSD::Notify(const Observable &obs, const ObservableMessage msg)
{
  const CGUIMessage m(GUI_MSG_REFRESH_LIST, GetID(), 0, msg);
  CApplicationMessenger::GetInstance().SendGUIMessage(m);
}

void CGUIDialogPVRChannelsOSD::SaveSelectedItemPath(int iGroupID)
{
  m_groupSelectedItemPaths[iGroupID] = m_viewControl.GetSelectedItemPath();
}

std::string CGUIDialogPVRChannelsOSD::GetLastSelectedItemPath(int iGroupID) const
{
  const auto it = m_groupSelectedItemPaths.find(iGroupID);
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
    for (const CFileItemPtr channel : *m_vecItems)
    {
      if (channel->GetPVRChannelInfoTag()->ChannelNumber() == channelNumber)
      {
        m_viewControl.SetSelectedItem(itemIndex);
        return;
      }
      ++itemIndex;
    }
  }
}

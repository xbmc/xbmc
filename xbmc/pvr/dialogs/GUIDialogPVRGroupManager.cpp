/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRGroupManager.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/helpers//DialogOKHelper.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/filesystem/PVRGUIDirectory.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <memory>
#include <string>
#include <vector>

using namespace KODI::MESSAGING;
using namespace PVR;

#define CONTROL_LIST_CHANNELS_LEFT    11
#define CONTROL_LIST_CHANNELS_RIGHT   12
#define CONTROL_LIST_CHANNEL_GROUPS   13
#define CONTROL_CURRENT_GROUP_LABEL   20
#define CONTROL_UNGROUPED_LABEL       21
#define CONTROL_IN_GROUP_LABEL        22
#define BUTTON_HIDE_GROUP             25
#define BUTTON_NEWGROUP               26
#define BUTTON_RENAMEGROUP            27
#define BUTTON_DELGROUP               28
#define BUTTON_OK                     29
#define BUTTON_TOGGLE_RADIO_TV        34
#define BUTTON_RECREATE_GROUP_THUMB   35

namespace
{
constexpr const char* PROPERTY_CLIENT_NAME = "ClientName";

} // namespace

CGUIDialogPVRGroupManager::CGUIDialogPVRGroupManager() :
    CGUIDialog(WINDOW_DIALOG_PVR_GROUP_MANAGER, "DialogPVRGroupManager.xml")
{
  m_ungroupedChannels = new CFileItemList;
  m_groupMembers = new CFileItemList;
  m_channelGroups = new CFileItemList;

  SetRadio(false);
}

CGUIDialogPVRGroupManager::~CGUIDialogPVRGroupManager()
{
  delete m_ungroupedChannels;
  delete m_groupMembers;
  delete m_channelGroups;
}

void CGUIDialogPVRGroupManager::SetRadio(bool bIsRadio)
{
  m_bIsRadio = bIsRadio;
  SetProperty("IsRadio", m_bIsRadio ? "true" : "");
}

bool CGUIDialogPVRGroupManager::PersistChanges()
{
  return CServiceBroker::GetPVRManager().ChannelGroups()->Get(m_bIsRadio)->PersistAll();
}

bool CGUIDialogPVRGroupManager::OnPopupMenu(int itemNumber)
{
  // Currently, the only context menu item is "move".
  if (!m_allowReorder)
    return false;

  CContextButtons buttons;

  if (itemNumber < 0 || itemNumber >= m_channelGroups->Size())
    return false;

  const auto item = m_channelGroups->Get(itemNumber);
  if (!item)
    return false;

  item->Select(true);

  buttons.Add(CONTEXT_BUTTON_MOVE, 116); // Move

  const int choice = CGUIDialogContextMenu::ShowAndGetChoice(buttons);

  item->Select(false);

  if (choice < 0)
    return false;

  return OnContextButton(itemNumber, choice);
}

bool CGUIDialogPVRGroupManager::OnContextButton(int itemNumber, int button)
{
  if (itemNumber < 0 || itemNumber >= m_channelGroups->Size())
    return false;

  const auto item = m_channelGroups->Get(itemNumber);
  if (!item)
    return false;

  if (button == CONTEXT_BUTTON_MOVE)
  {
    // begin moving item
    m_movingItem = true;
    item->Select(true);
  }
  return true;
}

bool CGUIDialogPVRGroupManager::ActionButtonOk(const CGUIMessage& message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (iControl == BUTTON_OK)
  {
    PersistChanges();
    Close();
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonNewGroup(const CGUIMessage& message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (iControl == BUTTON_NEWGROUP)
  {
    std::string strGroupName;
    if (CGUIKeyboardFactory::ShowAndGetInput(strGroupName, CVariant{g_localizeStrings.Get(19139)}, false))
    {
      if (!strGroupName.empty())
      {
        // add the group if it doesn't already exist
        auto groups = CServiceBroker::GetPVRManager().ChannelGroups()->Get(m_bIsRadio);
        const auto group = groups->AddGroup(strGroupName);
        if (group)
        {
          m_selectedGroup = group;
          m_iSelectedChannelGroup = -1; // recalc index in Update()
          Update();
        }
      }
    }
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonDeleteGroup(const CGUIMessage& message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (iControl == BUTTON_DELGROUP)
  {
    if (!m_selectedGroup)
      return bReturn;

    CGUIDialogYesNo* pDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogYesNo>(WINDOW_DIALOG_YES_NO);
    if (!pDialog)
      return bReturn;

    pDialog->SetHeading(CVariant{117});
    pDialog->SetLine(0, CVariant{""});
    pDialog->SetLine(1, CVariant{m_selectedGroup->GroupName()});
    pDialog->SetLine(2, CVariant{""});
    pDialog->Open();

    if (pDialog->IsConfirmed())
    {
      ClearSelectedGroupsThumbnail();
      if (CServiceBroker::GetPVRManager()
              .ChannelGroups()
              ->Get(m_bIsRadio)
              ->DeleteGroup(m_selectedGroup))
        Update();
    }

    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonRenameGroup(const CGUIMessage& message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (iControl == BUTTON_RENAMEGROUP)
  {
    if (!m_selectedGroup)
      return bReturn;

    std::string strGroupName(m_selectedGroup->GroupName());
    if (CGUIKeyboardFactory::ShowAndGetInput(strGroupName, CVariant{g_localizeStrings.Get(19139)},
                                             true /* allow empty result */))
    {
      // if an empty string was given we reset the name to the client-supplied name, if available
      const bool resetName = strGroupName.empty() && !m_selectedGroup->ClientGroupName().empty();
      if (resetName)
        strGroupName = m_selectedGroup->ClientGroupName();

      if (!strGroupName.empty())
      {
        ClearSelectedGroupsThumbnail();
        m_selectedGroup->SetGroupName(strGroupName, !resetName);
        Update();
      }
    }

    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonUngroupedChannels(const CGUIMessage& message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (m_viewUngroupedChannels.HasControl(iControl)) // list/thumb control
  {
    m_iSelectedUngroupedChannel = m_viewUngroupedChannels.GetSelectedItem();
    if (m_selectedGroup->SupportsMemberAdd())
    {
      const int actionID = message.GetParam1();

      if (actionID == ACTION_SELECT_ITEM || actionID == ACTION_MOUSE_LEFT_CLICK)
      {
        if (m_channelGroups->GetFolderCount() == 0)
        {
          HELPERS::ShowOKDialogText(CVariant{19033}, CVariant{19137});
        }
        else if (m_ungroupedChannels->GetFileCount() > 0)
        {
          const auto itemChannel = m_ungroupedChannels->Get(m_iSelectedUngroupedChannel);

          if (m_selectedGroup->AppendToGroup(itemChannel->GetPVRChannelGroupMemberInfoTag()))
          {
            ClearSelectedGroupsThumbnail();
            Update();
          }
        }
      }
    }
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonGroupMembers(const CGUIMessage& message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (m_viewGroupMembers.HasControl(iControl)) // list/thumb control
  {
    m_iSelectedGroupMember = m_viewGroupMembers.GetSelectedItem();
    if (m_selectedGroup->SupportsMemberRemove())
    {
      const int actionID = message.GetParam1();

      if (actionID == ACTION_SELECT_ITEM || actionID == ACTION_MOUSE_LEFT_CLICK)
      {
        if (m_selectedGroup && m_groupMembers->GetFileCount() > 0)
        {
          const auto itemChannel = m_groupMembers->Get(m_iSelectedGroupMember);
          m_selectedGroup->RemoveFromGroup(itemChannel->GetPVRChannelGroupMemberInfoTag());
          ClearSelectedGroupsThumbnail();
          Update();
        }
      }
    }
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonChannelGroups(const CGUIMessage& message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (m_viewChannelGroups.HasControl(iControl)) // list/thumb control
  {
    if (!m_movingItem)
    {
      const int iAction = message.GetParam1();

      if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
      {
        m_iSelectedChannelGroup = m_viewChannelGroups.GetSelectedItem();
        Update();
      }
      else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
      {
        m_iSelectedChannelGroup = m_viewChannelGroups.GetSelectedItem();
        OnPopupMenu(m_iSelectedChannelGroup);
      }
      bReturn = true;
    }
    else
    {
      const auto item = m_channelGroups->Get(m_iSelectedChannelGroup);
      if (item)
      {
        // end moving item
        item->Select(false);
        m_movingItem = false;

        // reset group positions
        auto* groups = CServiceBroker::GetPVRManager().ChannelGroups()->Get(m_bIsRadio);
        int pos = 1;
        for (auto& groupItem : *m_channelGroups)
        {
          const auto group = groups->GetGroupByPath(groupItem->GetPath());
          if (group)
            group->SetPosition(pos++);
        }
        groups->SortGroups();
        bReturn = true;
      }
    }
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonHideGroup(const CGUIMessage& message)
{
  bool bReturn = false;

  if (message.GetSenderId() == BUTTON_HIDE_GROUP && m_selectedGroup)
  {
    CGUIRadioButtonControl* button = static_cast<CGUIRadioButtonControl*>(GetControl(message.GetSenderId()));
    if (button)
    {
      CServiceBroker::GetPVRManager()
          .ChannelGroups()
          ->Get(m_bIsRadio)
          ->HideGroup(m_selectedGroup, button->IsSelected());
      Update();
    }

    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonToggleRadioTV(const CGUIMessage& message)
{
  bool bReturn = false;

  if (message.GetSenderId() == BUTTON_TOGGLE_RADIO_TV)
  {
    PersistChanges();
    SetRadio(!m_bIsRadio);
    Update();
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonRecreateThumbnail(const CGUIMessage& message)
{
  bool bReturn = false;

  if (message.GetSenderId() == BUTTON_RECREATE_GROUP_THUMB)
  {
    m_thumbLoader.ClearCachedImages(*m_channelGroups);
    Update();
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::OnMessageClick(const CGUIMessage& message)
{
  return ActionButtonOk(message) ||
      ActionButtonNewGroup(message) ||
      ActionButtonDeleteGroup(message) ||
      ActionButtonRenameGroup(message) ||
      ActionButtonUngroupedChannels(message) ||
      ActionButtonGroupMembers(message) ||
      ActionButtonChannelGroups(message) ||
      ActionButtonHideGroup(message) ||
      ActionButtonToggleRadioTV(message) ||
      ActionButtonRecreateThumbnail(message);
}

bool CGUIDialogPVRGroupManager::OnMessage(CGUIMessage& message)
{
  unsigned int iMessage = message.GetMessage();

  switch (iMessage)
  {
    case GUI_MSG_CLICKED:
    {
      OnMessageClick(message);
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogPVRGroupManager::OnActionMove(const CAction& action)
{
  bool bReturn = false;
  int iActionId = action.GetID();

  if (GetFocusedControlID() == CONTROL_LIST_CHANNEL_GROUPS)
  {
    if (iActionId == ACTION_MOUSE_MOVE)
    {
      int iSelected = m_viewChannelGroups.GetSelectedItem();
      if (m_iSelectedChannelGroup < iSelected)
      {
        iActionId = ACTION_MOVE_DOWN;
      }
      else if (m_iSelectedChannelGroup > iSelected)
      {
        iActionId = ACTION_MOVE_UP;
      }
      else
      {
        return bReturn;
      }
    }

    if (iActionId == ACTION_MOVE_DOWN || iActionId == ACTION_MOVE_UP ||
        iActionId == ACTION_PAGE_DOWN || iActionId == ACTION_PAGE_UP ||
        iActionId == ACTION_FIRST_PAGE || iActionId == ACTION_LAST_PAGE)
    {
      CGUIDialog::OnAction(action);
      int iSelected = m_viewChannelGroups.GetSelectedItem();

      bReturn = true;
      if (!m_movingItem)
      {
        if (iSelected != m_iSelectedChannelGroup)
        {
          m_iSelectedChannelGroup = iSelected;
          Update();
        }
      }
      else
      {
        bool moveUp = iActionId == ACTION_PAGE_UP || iActionId == ACTION_MOVE_UP ||
                      iActionId == ACTION_FIRST_PAGE;
        unsigned int lines = moveUp ? std::abs(m_iSelectedChannelGroup - iSelected) : 1;
        bool outOfBounds = moveUp ? m_iSelectedChannelGroup <= 0
                                  : m_iSelectedChannelGroup >= m_channelGroups->Size() - 1;
        if (outOfBounds)
        {
          moveUp = !moveUp;
          lines = m_channelGroups->Size() - 1;
        }
        for (unsigned int line = 0; line < lines; ++line)
        {
          const unsigned int newSelect =
              moveUp ? m_iSelectedChannelGroup - 1 : m_iSelectedChannelGroup + 1;

          // swap items
          m_channelGroups->Swap(newSelect, m_iSelectedChannelGroup);
          m_iSelectedChannelGroup = newSelect;
        }

        m_viewChannelGroups.SetItems(*m_channelGroups);
        m_viewChannelGroups.SetSelectedItem(m_iSelectedChannelGroup);
      }
    }
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::OnAction(const CAction& action)
{
  return OnActionMove(action) ||
         CGUIDialog::OnAction(action);
}

void CGUIDialogPVRGroupManager::OnInitWindow()
{
  CGUIDialog::OnInitWindow();
  m_iSelectedUngroupedChannel = 0;
  m_iSelectedGroupMember = 0;
  m_iSelectedChannelGroup = 0;
  m_movingItem = false;

  // prevent resorting groups if backend group order shall be used
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  m_allowReorder = !settings->GetBool(CSettings::SETTING_PVRMANAGER_BACKENDCHANNELGROUPSORDER);

  Update();
}

void CGUIDialogPVRGroupManager::OnDeinitWindow(int nextWindowID)
{
  Clear();
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

void CGUIDialogPVRGroupManager::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  m_viewUngroupedChannels.Reset();
  m_viewUngroupedChannels.SetParentWindow(GetID());
  m_viewUngroupedChannels.AddView(GetControl(CONTROL_LIST_CHANNELS_LEFT));

  m_viewGroupMembers.Reset();
  m_viewGroupMembers.SetParentWindow(GetID());
  m_viewGroupMembers.AddView(GetControl(CONTROL_LIST_CHANNELS_RIGHT));

  m_viewChannelGroups.Reset();
  m_viewChannelGroups.SetParentWindow(GetID());
  m_viewChannelGroups.AddView(GetControl(CONTROL_LIST_CHANNEL_GROUPS));
}

void CGUIDialogPVRGroupManager::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();
  m_viewUngroupedChannels.Reset();
  m_viewGroupMembers.Reset();
  m_viewChannelGroups.Reset();
}

void CGUIDialogPVRGroupManager::Update()
{
  m_viewUngroupedChannels.SetCurrentView(CONTROL_LIST_CHANNELS_LEFT);
  m_viewGroupMembers.SetCurrentView(CONTROL_LIST_CHANNELS_RIGHT);
  m_viewChannelGroups.SetCurrentView(CONTROL_LIST_CHANNEL_GROUPS);

  Clear();

  // get the groups list
  CPVRGUIDirectory::GetChannelGroupsDirectory(m_bIsRadio, false, *m_channelGroups);

  for (auto& group : *m_channelGroups)
  {
    const std::shared_ptr<const CPVRClient> client =
        CServiceBroker::GetPVRManager().GetClient(*group);
    if (client)
      group->SetProperty(PROPERTY_CLIENT_NAME, client->GetFriendlyName());
  }

  // Load group thumbnails
  m_thumbLoader.Load(*m_channelGroups);

  m_viewChannelGroups.SetItems(*m_channelGroups);

  if (m_iSelectedChannelGroup >= 0)
  {
    m_viewChannelGroups.SetSelectedItem(m_iSelectedChannelGroup);

    // select a group or select the default group if no group was selected
    const auto item = m_channelGroups->Get(m_viewChannelGroups.GetSelectedItem());
    m_selectedGroup = CServiceBroker::GetPVRManager()
                          .ChannelGroups()
                          ->Get(m_bIsRadio)
                          ->GetGroupByPath(item->GetPath());
  }
  else if (m_selectedGroup)
  {
    m_viewChannelGroups.SetSelectedItem(m_selectedGroup->GetPath());
    m_iSelectedChannelGroup = m_viewChannelGroups.GetSelectedItem();
  }

  if (m_selectedGroup)
  {
    /* set this group in the pvrmanager, so it becomes the selected group in other dialogs too */
    CServiceBroker::GetPVRManager().PlaybackState()->SetActiveChannelGroup(m_selectedGroup);
    SET_CONTROL_LABEL(CONTROL_CURRENT_GROUP_LABEL, m_selectedGroup->GroupName());
    SET_CONTROL_SELECTED(GetID(), BUTTON_HIDE_GROUP, m_selectedGroup->IsHidden());

    CONTROL_ENABLE_ON_CONDITION(BUTTON_DELGROUP, m_selectedGroup->SupportsDelete());

    SET_CONTROL_LABEL(CONTROL_UNGROUPED_LABEL, g_localizeStrings.Get(19219));
    SET_CONTROL_LABEL(
        CONTROL_IN_GROUP_LABEL,
        StringUtils::Format("{} {}", g_localizeStrings.Get(19220), m_selectedGroup->GroupName()));

    const std::vector<std::shared_ptr<CPVRChannelGroupMember>> groupMembers =
        m_selectedGroup->GetMembers(CPVRChannelGroup::Include::ONLY_VISIBLE);
    for (const auto& groupMember : groupMembers)
    {
      m_groupMembers->Add(std::make_shared<CFileItem>(groupMember));
    }

    const auto groups = CServiceBroker::GetPVRManager().ChannelGroups()->Get(m_bIsRadio);
    const auto availableMembers = groups->GetMembersAvailableForGroup(m_selectedGroup);

    for (const auto& groupMember : availableMembers)
    {
      m_ungroupedChannels->Add(std::make_shared<CFileItem>(groupMember));
    }

    m_viewGroupMembers.SetItems(*m_groupMembers);
    m_viewGroupMembers.SetSelectedItem(m_iSelectedGroupMember);

    m_viewUngroupedChannels.SetItems(*m_ungroupedChannels);
    m_viewUngroupedChannels.SetSelectedItem(m_iSelectedUngroupedChannel);
  }
}

void CGUIDialogPVRGroupManager::Clear()
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  m_viewUngroupedChannels.Clear();
  m_viewGroupMembers.Clear();
  m_viewChannelGroups.Clear();

  m_ungroupedChannels->Clear();
  m_groupMembers->Clear();
  m_channelGroups->Clear();
}

void CGUIDialogPVRGroupManager::ClearSelectedGroupsThumbnail()
{
  m_thumbLoader.ClearCachedImage(*m_channelGroups->Get(m_iSelectedChannelGroup));
}

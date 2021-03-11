/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRGroupManager.h"

#include "FileItem.h"
#include "ServiceBroker.h"
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
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/filesystem/PVRGUIDirectory.h"
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

bool CGUIDialogPVRGroupManager::ActionButtonOk(CGUIMessage& message)
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

bool CGUIDialogPVRGroupManager::ActionButtonNewGroup(CGUIMessage& message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (iControl == BUTTON_NEWGROUP)
  {
    std::string strGroupName = "";
    /* prompt for a group name */
    if (CGUIKeyboardFactory::ShowAndGetInput(strGroupName, CVariant{g_localizeStrings.Get(19139)}, false))
    {
      if (strGroupName != "")
      {
        /* add the group if it doesn't already exist */
        CPVRChannelGroups* groups = CServiceBroker::GetPVRManager().ChannelGroups()->Get(m_bIsRadio);
        if (groups->AddGroup(strGroupName))
        {
          CServiceBroker::GetPVRManager().ChannelGroups()->Get(m_bIsRadio)->GetByName(strGroupName)->SetGroupType(PVR_GROUP_TYPE_USER_DEFINED);
          m_iSelectedChannelGroup = groups->Size() - 1;
          Update();
        }
      }
    }
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonDeleteGroup(CGUIMessage& message)
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
      if (CServiceBroker::GetPVRManager().ChannelGroups()->Get(m_bIsRadio)->DeleteGroup(*m_selectedGroup))
        Update();
    }

    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonRenameGroup(CGUIMessage& message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (iControl == BUTTON_RENAMEGROUP)
  {
    if (!m_selectedGroup)
      return bReturn;

    std::string strGroupName(m_selectedGroup->GroupName());
    if (CGUIKeyboardFactory::ShowAndGetInput(strGroupName, CVariant{g_localizeStrings.Get(19139)}, false))
    {
      if (!strGroupName.empty())
      {
        ClearSelectedGroupsThumbnail();
        m_selectedGroup->SetGroupName(strGroupName);
        Update();
      }
    }

    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonUngroupedChannels(CGUIMessage& message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (m_viewUngroupedChannels.HasControl(iControl)) // list/thumb control
  {
    m_iSelectedUngroupedChannel = m_viewUngroupedChannels.GetSelectedItem();
    int iAction = message.GetParam1();

    if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
    {
      if (m_channelGroups->GetFolderCount() == 0)
      {
        HELPERS::ShowOKDialogText(CVariant{19033}, CVariant{19137});
      }
      else if (m_ungroupedChannels->GetFileCount() > 0)
      {
        CFileItemPtr pItemChannel = m_ungroupedChannels->Get(m_iSelectedUngroupedChannel);

        if (m_selectedGroup->AppendToGroup(pItemChannel->GetPVRChannelInfoTag()))
        {
          ClearSelectedGroupsThumbnail();
          Update();
        }
      }
    }
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonGroupMembers(CGUIMessage& message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (m_viewGroupMembers.HasControl(iControl)) // list/thumb control
  {
    m_iSelectedGroupMember = m_viewGroupMembers.GetSelectedItem();
    int iAction = message.GetParam1();

    if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
    {
      if (m_selectedGroup && m_groupMembers->GetFileCount() > 0)
      {
        CFileItemPtr pItemChannel = m_groupMembers->Get(m_iSelectedGroupMember);
        m_selectedGroup->RemoveFromGroup(pItemChannel->GetPVRChannelInfoTag());
        ClearSelectedGroupsThumbnail();
        Update();
      }
    }
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonChannelGroups(CGUIMessage& message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (m_viewChannelGroups.HasControl(iControl)) // list/thumb control
  {
    int iAction = message.GetParam1();

    if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
    {
      m_iSelectedChannelGroup = m_viewChannelGroups.GetSelectedItem();
      Update();
    }
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonHideGroup(CGUIMessage& message)
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

bool CGUIDialogPVRGroupManager::ActionButtonToggleRadioTV(CGUIMessage& message)
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

bool CGUIDialogPVRGroupManager::ActionButtonRecreateThumbnail(CGUIMessage& message)
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

bool CGUIDialogPVRGroupManager::OnMessageClick(CGUIMessage& message)
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
      if (iSelected != m_iSelectedChannelGroup)
      {
        m_iSelectedChannelGroup = iSelected;
        Update();
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

  // Load group thumbnails
  m_thumbLoader.Load(*m_channelGroups);

  m_viewChannelGroups.SetItems(*m_channelGroups);
  m_viewChannelGroups.SetSelectedItem(m_iSelectedChannelGroup);

  /* select a group or select the default group if no group was selected */
  CFileItemPtr pItem = m_channelGroups->Get(m_viewChannelGroups.GetSelectedItem());
  m_selectedGroup = CServiceBroker::GetPVRManager().ChannelGroups()->Get(m_bIsRadio)->GetByName(pItem->m_strTitle);
  if (m_selectedGroup)
  {
    /* set this group in the pvrmanager, so it becomes the selected group in other dialogs too */
    CServiceBroker::GetPVRManager().PlaybackState()->SetPlayingGroup(m_selectedGroup);
    SET_CONTROL_LABEL(CONTROL_CURRENT_GROUP_LABEL, m_selectedGroup->GroupName());
    SET_CONTROL_SELECTED(GetID(), BUTTON_HIDE_GROUP, m_selectedGroup->IsHidden());

    CONTROL_ENABLE_ON_CONDITION(BUTTON_DELGROUP, !m_selectedGroup->IsInternalGroup());

    if (m_selectedGroup->IsInternalGroup())
    {
      std::string strNewLabel = StringUtils::Format("%s %s",
                                        g_localizeStrings.Get(19022).c_str(),
                                        m_bIsRadio ? g_localizeStrings.Get(19024).c_str() : g_localizeStrings.Get(19023).c_str());
      SET_CONTROL_LABEL(CONTROL_UNGROUPED_LABEL, strNewLabel);

      strNewLabel = StringUtils::Format("%s %s",
                                        g_localizeStrings.Get(19218).c_str(),
                                        m_bIsRadio ? g_localizeStrings.Get(19024).c_str() : g_localizeStrings.Get(19023).c_str());
      SET_CONTROL_LABEL(CONTROL_IN_GROUP_LABEL, strNewLabel);
    }
    else
    {
      std::string strNewLabel = StringUtils::Format("%s", g_localizeStrings.Get(19219).c_str());
      SET_CONTROL_LABEL(CONTROL_UNGROUPED_LABEL, strNewLabel);

      strNewLabel = StringUtils::Format("%s %s", g_localizeStrings.Get(19220).c_str(), m_selectedGroup->GroupName().c_str());
      SET_CONTROL_LABEL(CONTROL_IN_GROUP_LABEL, strNewLabel);
    }

    // Slightly different handling for "all" group...
    if (m_selectedGroup->IsInternalGroup())
    {
      const std::vector<std::shared_ptr<PVRChannelGroupMember>> groupMembers = m_selectedGroup->GetMembers(CPVRChannelGroup::Include::ALL);
      for (const auto& groupMember : groupMembers)
      {
        if (groupMember->channel->IsHidden())
          m_ungroupedChannels->Add(std::make_shared<CFileItem>(groupMember->channel));
        else
          m_groupMembers->Add(std::make_shared<CFileItem>(groupMember->channel));
      }
    }
    else
    {
      const std::vector<std::shared_ptr<PVRChannelGroupMember>> groupMembers = m_selectedGroup->GetMembers(CPVRChannelGroup::Include::ONLY_VISIBLE);
      for (const auto& groupMember : groupMembers)
      {
        m_groupMembers->Add(std::make_shared<CFileItem>(groupMember->channel));
      }

      /* for the center part, get all channels of the "all" channels group that are not in this group */
      const std::shared_ptr<CPVRChannelGroup> allGroup = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAll(m_bIsRadio);
      const std::vector<std::shared_ptr<PVRChannelGroupMember>> allGroupMembers = allGroup->GetMembers(CPVRChannelGroup::Include::ONLY_VISIBLE);
      for (const auto& groupMember : allGroupMembers)
      {
        if (!m_selectedGroup->IsGroupMember(groupMember->channel))
          m_ungroupedChannels->Add(std::make_shared<CFileItem>(groupMember->channel));
      }
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

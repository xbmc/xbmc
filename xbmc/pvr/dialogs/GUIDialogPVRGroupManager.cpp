/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIDialogPVRGroupManager.h"
#include "FileItem.h"
#include "guilib/GUIKeyboardFactory.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/GUIRadioButtonControl.h"
#include "utils/StringUtils.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"

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

CGUIDialogPVRGroupManager::CGUIDialogPVRGroupManager() :
    CGUIDialog(WINDOW_DIALOG_PVR_GROUP_MANAGER, "DialogPVRGroupManager.xml")
{
  m_bIsRadio = 0;
  m_iSelectedUngroupedChannel = 0;
  m_iSelectedGroupMember = 0;
  m_iSelectedChannelGroup = 0;
  m_ungroupedChannels = new CFileItemList;
  m_groupMembers      = new CFileItemList;
  m_channelGroups     = new CFileItemList;
}

CGUIDialogPVRGroupManager::~CGUIDialogPVRGroupManager()
{
  delete m_ungroupedChannels;
  delete m_groupMembers;
  delete m_channelGroups;
}

bool CGUIDialogPVRGroupManager::PersistChanges(void)
{
  return g_PVRChannelGroups->Get(m_bIsRadio)->PersistAll();
}

bool CGUIDialogPVRGroupManager::CancelChanges(void)
{
  // TODO
  return false;
}

bool CGUIDialogPVRGroupManager::ActionButtonOk(CGUIMessage &message)
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

bool CGUIDialogPVRGroupManager::ActionButtonNewGroup(CGUIMessage &message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (iControl == BUTTON_NEWGROUP)
  {
    std::string strGroupName = "";
    /* prompt for a group name */
    if (CGUIKeyboardFactory::ShowAndGetInput(strGroupName, g_localizeStrings.Get(19139), false))
    {
      if (strGroupName != "")
      {
        /* add the group if it doesn't already exist */
        CPVRChannelGroups *groups = ((CPVRChannelGroups *) g_PVRChannelGroups->Get(m_bIsRadio));
        if (groups->AddGroup(strGroupName))
        {
          g_PVRChannelGroups->Get(m_bIsRadio)->GetByName(strGroupName)->SetGroupType(PVR_GROUP_TYPE_USER_DEFINED);
          m_iSelectedChannelGroup = groups->Size() - 1;
          Update();
        }
      }
    }
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonDeleteGroup(CGUIMessage &message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (iControl == BUTTON_DELGROUP)
  {
    if (!m_selectedGroup)
      return bReturn;

    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialog)
      return bReturn;

    pDialog->SetHeading(117);
    pDialog->SetLine(0, "");
    pDialog->SetLine(1, m_selectedGroup->GroupName());
    pDialog->SetLine(2, "");
    pDialog->DoModal();

    if (pDialog->IsConfirmed())
    {
      if (((CPVRChannelGroups *) g_PVRChannelGroups->Get(m_bIsRadio))->DeleteGroup(*m_selectedGroup))
        Update();
    }

    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonRenameGroup(CGUIMessage &message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (iControl == BUTTON_RENAMEGROUP)
  {
    if (!m_selectedGroup)
      return bReturn;

    std::string strGroupName(m_selectedGroup->GroupName());
    if (CGUIKeyboardFactory::ShowAndGetInput(strGroupName, g_localizeStrings.Get(19139), false))
    {
      if (strGroupName != "")
      {
        m_selectedGroup->SetGroupName(strGroupName, true);
        Update();
      }
    }

    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonUngroupedChannels(CGUIMessage &message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (m_viewUngroupedChannels.HasControl(iControl))   // list/thumb control
  {
    m_iSelectedUngroupedChannel = m_viewUngroupedChannels.GetSelectedItem();
    int iAction     = message.GetParam1();

    if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
    {
      if (m_channelGroups->GetFolderCount() == 0)
      {
        CGUIDialogOK::ShowAndGetInput(19033, 19137);
      }
      else if (m_ungroupedChannels->GetFileCount() > 0)
      {
        CFileItemPtr pItemChannel = m_ungroupedChannels->Get(m_iSelectedUngroupedChannel);
        if (m_selectedGroup->AddToGroup(pItemChannel->GetPVRChannelInfoTag()))
          Update();
      }
    }
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonGroupMembers(CGUIMessage &message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (m_viewGroupMembers.HasControl(iControl))   // list/thumb control
  {
    m_iSelectedGroupMember = m_viewGroupMembers.GetSelectedItem();
    int iAction      = message.GetParam1();

    if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
    {
      if (m_selectedGroup && m_groupMembers->GetFileCount() > 0)
      {
        CFileItemPtr pItemChannel = m_groupMembers->Get(m_iSelectedGroupMember);
        m_selectedGroup->RemoveFromGroup(pItemChannel->GetPVRChannelInfoTag());
        Update();
      }
    }
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonChannelGroups(CGUIMessage &message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (m_viewChannelGroups.HasControl(iControl))   // list/thumb control
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

bool CGUIDialogPVRGroupManager::ActionButtonHideGroup(CGUIMessage &message)
{
  bool bReturn = false;

  if (message.GetSenderId() == BUTTON_HIDE_GROUP && m_selectedGroup)
  {
    CGUIRadioButtonControl *button = (CGUIRadioButtonControl*) GetControl(message.GetSenderId());
    if (button)
    {
      m_selectedGroup->SetHidden(button->IsSelected());
      Update();
    }

    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::OnMessageClick(CGUIMessage &message)
{
  return ActionButtonOk(message) ||
      ActionButtonNewGroup(message) ||
      ActionButtonDeleteGroup(message) ||
      ActionButtonRenameGroup(message) ||
      ActionButtonUngroupedChannels(message) ||
      ActionButtonGroupMembers(message) ||
      ActionButtonChannelGroups(message) ||
      ActionButtonHideGroup(message);
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

void CGUIDialogPVRGroupManager::OnInitWindow()
{
  CGUIDialog::OnInitWindow();
  m_iSelectedUngroupedChannel  = 0;
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
  /* lock our display, as this window is rendered from the player thread */
  g_graphicsContext.Lock();
  m_viewUngroupedChannels.SetCurrentView(CONTROL_LIST_CHANNELS_LEFT);
  m_viewGroupMembers.SetCurrentView(CONTROL_LIST_CHANNELS_RIGHT);
  m_viewChannelGroups.SetCurrentView(CONTROL_LIST_CHANNEL_GROUPS);

  Clear();

  /* get the groups list */
  g_PVRChannelGroups->Get(m_bIsRadio)->GetGroupList(m_channelGroups);
  m_viewChannelGroups.SetItems(*m_channelGroups);
  m_viewChannelGroups.SetSelectedItem(m_iSelectedChannelGroup);

  /* select a group or select the default group if no group was selected */
  CFileItemPtr pItem = m_channelGroups->Get(m_viewChannelGroups.GetSelectedItem());
  m_selectedGroup = g_PVRChannelGroups->Get(m_bIsRadio)->GetByName(pItem->m_strTitle);
  if (m_selectedGroup)
  {
    /* set this group in the pvrmanager, so it becomes the selected group in other dialogs too */
    g_PVRManager.SetPlayingGroup(m_selectedGroup);
    SET_CONTROL_LABEL(CONTROL_CURRENT_GROUP_LABEL, m_selectedGroup->GroupName());
    SET_CONTROL_SELECTED(GetID(), BUTTON_HIDE_GROUP, m_selectedGroup->IsHidden());

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

    /* get all channels that are not in this group for the center part */
    m_selectedGroup->GetMembers(*m_ungroupedChannels, false);
    m_viewUngroupedChannels.SetItems(*m_ungroupedChannels);
    m_viewUngroupedChannels.SetSelectedItem(m_iSelectedUngroupedChannel);

    /* get all channels in this group for the right side part */
    m_selectedGroup->GetMembers(*m_groupMembers, true);
    m_viewGroupMembers.SetItems(*m_groupMembers);
    m_viewGroupMembers.SetSelectedItem(m_iSelectedGroupMember);
  }

  g_graphicsContext.Unlock();
}

void CGUIDialogPVRGroupManager::Clear()
{
  m_viewUngroupedChannels.Clear();
  m_viewGroupMembers.Clear();
  m_viewChannelGroups.Clear();

  m_ungroupedChannels->Clear();
  m_groupMembers->Clear();
  m_channelGroups->Clear();
}

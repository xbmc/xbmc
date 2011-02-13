/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIDialogPVRGroupManager.h"
#include "Application.h"
#include "FileItem.h"
#include "dialogs/GUIDialogKeyboard.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"

using namespace std;

#define CONTROL_LIST_CHANNELS_LEFT    11
#define CONTROL_LIST_CHANNELS_RIGHT   12
#define CONTROL_LIST_CHANNEL_GROUPS   13
#define CONTROL_CURRENT_GROUP_LABEL   20
#define BUTTON_NEWGROUP               26
#define BUTTON_RENAMEGROUP            27
#define BUTTON_DELGROUP               28
#define BUTTON_OK                     29

CGUIDialogPVRGroupManager::CGUIDialogPVRGroupManager() :
    CGUIDialog(WINDOW_DIALOG_PVR_GROUP_MANAGER, "DialogPVRGroupManager.xml")
{
  m_channelLeftItems  = new CFileItemList;
  m_channelRightItems = new CFileItemList;
  m_channelGroupItems = new CFileItemList;
}

CGUIDialogPVRGroupManager::~CGUIDialogPVRGroupManager()
{
  delete m_channelLeftItems;
  delete m_channelRightItems;
  delete m_channelGroupItems;
}

bool CGUIDialogPVRGroupManager::ActionButtonOk(CGUIMessage &message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (iControl == BUTTON_OK)
  {
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
    CStdString strDescription = "";
    if (CGUIDialogKeyboard::ShowAndGetInput(strDescription, g_localizeStrings.Get(19139), false))
    {
      if (strDescription != "")
      {
        ((CPVRChannelGroups *) CPVRManager::GetChannelGroups()->Get(m_bIsRadio))->AddGroup(strDescription);
        Update();
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

  if (iControl == BUTTON_DELGROUP && m_channelGroupItems->GetFileCount() != 0)
  {
    m_iSelectedGroup        = m_viewControlGroup.GetSelectedItem();
    CFileItemPtr pItemGroup = m_channelGroupItems->Get(m_iSelectedGroup);

    // prompt user for confirmation of channel record
    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (pDialog)
    {
      CStdString groupName;
      CPVRManager::GetChannelGroups()->Get(m_bIsRadio)->GetGroupName(atoi(pItemGroup->m_strPath.c_str()));

      pDialog->SetHeading(117);
      pDialog->SetLine(0, "");
      pDialog->SetLine(1, groupName);
      pDialog->SetLine(2, "");
      pDialog->DoModal();

      if (pDialog->IsConfirmed())
      {
        ((CPVRChannelGroups *) CPVRManager::GetChannelGroups()->Get(m_bIsRadio))->DeleteGroup(atoi(pItemGroup->m_strPath.c_str()));
        Update();
      }
    }
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonRenameGroup(CGUIMessage &message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (iControl == BUTTON_RENAMEGROUP && m_channelGroupItems->GetFileCount() != 0)
  {
    if (CGUIDialogKeyboard::ShowAndGetInput(m_CurrentGroupName, g_localizeStrings.Get(19139), false))
    {
      if (m_CurrentGroupName != "")
      {
        CPVRChannelGroup *group = (CPVRChannelGroup *) CPVRManager::GetChannelGroups()->Get(m_bIsRadio)->GetById(atoi(m_channelGroupItems->Get(m_iSelectedGroup)->m_strPath.c_str()));
        if (group)
          group->SetGroupName(m_CurrentGroupName, true);

        Update();
      }
    }
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonControlLeft(CGUIMessage &message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (m_viewControlLeft.HasControl(iControl))   // list/thumb control
  {
    m_iSelectedLeft = m_viewControlLeft.GetSelectedItem();
    int iAction     = message.GetParam1();

    if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
    {
      if (m_channelGroupItems->GetFileCount() == 0)
      {
        CGUIDialogOK::ShowAndGetInput(19033,19137,0,19138);
      }
      else if (m_channelLeftItems->GetFileCount() > 0)
      {
        CFileItemPtr pItemGroup   = m_channelGroupItems->Get(m_iSelectedGroup);
        CFileItemPtr pItemChannel = m_channelLeftItems->Get(m_iSelectedLeft);
        ((CPVRChannelGroups *) CPVRManager::GetChannelGroups()->Get(m_bIsRadio))->AddChannelToGroup(*pItemChannel->GetPVRChannelInfoTag(), atoi(pItemGroup->m_strPath.c_str()));
        Update();
      }
    }
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonControlRight(CGUIMessage &message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (m_viewControlRight.HasControl(iControl))   // list/thumb control
  {
    m_iSelectedRight = m_viewControlRight.GetSelectedItem();
    int iAction      = message.GetParam1();

    if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
    {
      if (m_channelRightItems->GetFileCount() > 0)
      {
        CFileItemPtr pItemChannel = m_channelRightItems->Get(m_iSelectedRight);
        ((CPVRChannelGroups *) CPVRManager::GetChannelGroups()->Get(m_bIsRadio))->AddChannelToGroup(*pItemChannel->GetPVRChannelInfoTag(), 0);
        Update();
      }
    }
    bReturn = true;
  }

  return bReturn;
}

bool CGUIDialogPVRGroupManager::ActionButtonControlGroup(CGUIMessage &message)
{
  bool bReturn = false;
  unsigned int iControl = message.GetSenderId();

  if (m_viewControlGroup.HasControl(iControl))   // list/thumb control
  {
    int iAction = message.GetParam1();

    if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
    {
      m_iSelectedGroup = m_viewControlGroup.GetSelectedItem();
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
      ActionButtonControlLeft(message) ||
      ActionButtonControlRight(message) ||
      ActionButtonControlGroup(message);
}

bool CGUIDialogPVRGroupManager::OnMessage(CGUIMessage& message)
{
  unsigned int iMessage = message.GetMessage();

  switch (iMessage)
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      Clear();
    }
    break;

    case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      m_iSelectedLeft  = 0;
      m_iSelectedRight = 0;
      m_iSelectedGroup = 0;
      Update();
      return true;
    }
    break;

    case GUI_MSG_CLICKED:
    {
      OnMessageClick(message);
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogPVRGroupManager::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  m_viewControlLeft.Reset();
  m_viewControlLeft.SetParentWindow(GetID());
  m_viewControlLeft.AddView(GetControl(CONTROL_LIST_CHANNELS_LEFT));

  m_viewControlRight.Reset();
  m_viewControlRight.SetParentWindow(GetID());
  m_viewControlRight.AddView(GetControl(CONTROL_LIST_CHANNELS_RIGHT));

  m_viewControlGroup.Reset();
  m_viewControlGroup.SetParentWindow(GetID());
  m_viewControlGroup.AddView(GetControl(CONTROL_LIST_CHANNEL_GROUPS));
}

void CGUIDialogPVRGroupManager::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();
  m_viewControlLeft.Reset();
  m_viewControlRight.Reset();
  m_viewControlGroup.Reset();
}

void CGUIDialogPVRGroupManager::Update()
{
  m_CurrentGroupName = "";

  // lock our display, as this window is rendered from the player thread
  g_graphicsContext.Lock();
  m_viewControlLeft.SetCurrentView(CONTROL_LIST_CHANNELS_LEFT);
  m_viewControlRight.SetCurrentView(CONTROL_LIST_CHANNELS_RIGHT);
  m_viewControlGroup.SetCurrentView(CONTROL_LIST_CHANNEL_GROUPS);

  // empty the lists ready for population
  Clear();

  int groups = CPVRManager::GetChannelGroups()->Get(m_bIsRadio)->GetGroupList(m_channelGroupItems);

  m_viewControlGroup.SetItems(*m_channelGroupItems);
  m_viewControlGroup.SetSelectedItem(m_iSelectedGroup);

  CPVRChannelGroup *channels = (CPVRChannelGroup *) CPVRManager::GetChannelGroups()->GetGroupAll(m_bIsRadio);
  channels->GetChannels(m_channelLeftItems, 0);
  m_viewControlLeft.SetItems(*m_channelLeftItems);
  m_viewControlLeft.SetSelectedItem(m_iSelectedLeft);

  if (groups > 0)
  {
    CFileItemPtr pItem = m_channelGroupItems->Get(m_viewControlGroup.GetSelectedItem());
    m_CurrentGroupName = pItem->m_strTitle;
    SET_CONTROL_LABEL(CONTROL_CURRENT_GROUP_LABEL, m_CurrentGroupName);

    channels->GetChannels(m_channelRightItems, atoi(pItem->m_strPath.c_str()));
    m_viewControlRight.SetItems(*m_channelRightItems);
    m_viewControlRight.SetSelectedItem(m_iSelectedRight);
  }

  g_graphicsContext.Unlock();
}

void CGUIDialogPVRGroupManager::Clear()
{
  m_viewControlLeft.Clear();
  m_viewControlRight.Clear();
  m_viewControlGroup.Clear();

  m_channelLeftItems->Clear();
  m_channelRightItems->Clear();
  m_channelGroupItems->Clear();
}

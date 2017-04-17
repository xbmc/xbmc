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

#include "GUIDialogPVRChannelsOSD.h"

#include "FileItem.h"
#include "GUIInfoManager.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "ServiceBroker.h"
#include "view/ViewState.h"

#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/windows/GUIWindowPVRBase.h"

using namespace PVR;
using namespace KODI::MESSAGING;

#define MAX_INVALIDATION_FREQUENCY 2000 // limit to one invalidation per X milliseconds

#define CONTROL_LIST                  11

CGUIDialogPVRChannelsOSD::CGUIDialogPVRChannelsOSD() :
    CGUIDialog(WINDOW_DIALOG_PVR_OSD_CHANNELS, "DialogPVRChannelsOSD.xml"),
    CPVRChannelNumberInputHandler(1000)
{
  m_vecItems = new CFileItemList;
}

CGUIDialogPVRChannelsOSD::~CGUIDialogPVRChannelsOSD()
{
  delete m_vecItems;

  g_infoManager.UnregisterObserver(this);
  CServiceBroker::GetPVRManager().EpgContainer()->UnregisterObserver(this);
}

bool CGUIDialogPVRChannelsOSD::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();

      if (m_viewControl.HasControl(iControl))   // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          /* Switch to channel */
          GotoChannel(iItem);
          return true;
        }
        else if (iAction == ACTION_SHOW_INFO || iAction == ACTION_MOUSE_RIGHT_CLICK)
        {
          /* Show information Dialog */
          ShowInfo(iItem);
          return true;
        }
      }
    }
    break;
  case GUI_MSG_REFRESH_LIST:
    {
      switch(message.GetParam1())
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
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogPVRChannelsOSD::OnInitWindow()
{
  /* Close dialog immediately if neither a TV nor a radio channel is playing */
  if (!CServiceBroker::GetPVRManager().IsPlayingTV() && !CServiceBroker::GetPVRManager().IsPlayingRadio())
  {
    Close();
    return;
  }

  Update();

  CGUIDialog::OnInitWindow();
}

void CGUIDialogPVRChannelsOSD::OnDeinitWindow(int nextWindowID)
{
  if (m_group)
  {
    if (m_group != GetPlayingGroup())
    {
      CGUIWindowPVRBase::SetSelectedItemPath(CServiceBroker::GetPVRManager().IsPlayingRadio(), GetLastSelectedItemPath(m_group->GroupID()));
      CServiceBroker::GetPVRManager().SetPlayingGroup(m_group);
    }
    else
    {
      CGUIWindowPVRBase::SetSelectedItemPath(CServiceBroker::GetPVRManager().IsPlayingRadio(), m_viewControl.GetSelectedItemPath());
    }

    m_group.reset();
  }

  CGUIDialog::OnDeinitWindow(nextWindowID);

  Clear();
}

bool CGUIDialogPVRChannelsOSD::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
  case ACTION_PREVIOUS_CHANNELGROUP:
  case ACTION_NEXT_CHANNELGROUP:
    {
      // save control states and currently selected item of group
      SaveControlStates();

      // switch to next or previous group
      CPVRChannelGroupPtr group = GetPlayingGroup();
      CPVRChannelGroupPtr nextGroup = action.GetID() == ACTION_NEXT_CHANNELGROUP ? group->GetNextGroup() : group->GetPreviousGroup();
      CServiceBroker::GetPVRManager().SetPlayingGroup(nextGroup);
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
      AppendChannelNumberDigit(action.GetID() - REMOTE_0);
      return true;
    }
  }

  return CGUIDialog::OnAction(action);
}

CPVRChannelGroupPtr CGUIDialogPVRChannelsOSD::GetPlayingGroup()
{
  CPVRChannelPtr channel(CServiceBroker::GetPVRManager().GetCurrentChannel());
  if (channel)
    return CServiceBroker::GetPVRManager().GetPlayingGroup(channel->IsRadio());
  else
    return CPVRChannelGroupPtr();
}

void CGUIDialogPVRChannelsOSD::Update()
{
  g_infoManager.RegisterObserver(this);
  CServiceBroker::GetPVRManager().EpgContainer()->RegisterObserver(this);

  // lock our display, as this window is rendered from the player thread
  g_graphicsContext.Lock();
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);

  // empty the list ready for population
  Clear();

  CPVRChannelPtr channel(CServiceBroker::GetPVRManager().GetCurrentChannel());
  if (channel)
  {
    CPVRChannelGroupPtr group = CServiceBroker::GetPVRManager().GetPlayingGroup(channel->IsRadio());
    if (group)
    {
      group->GetMembers(*m_vecItems);
      m_viewControl.SetItems(*m_vecItems);

      if (!m_group)
      {
        m_group = group;
        m_viewControl.SetSelectedItem(CGUIWindowPVRBase::GetSelectedItemPath(channel->IsRadio()));
        SaveSelectedItemPath(group->GroupID());
      }
    }
  }

  g_graphicsContext.Unlock();
}

void CGUIDialogPVRChannelsOSD::SetInvalid()
{
  if (m_refreshTimeout.IsTimePast())
  {
    VECFILEITEMS items = m_vecItems->GetList();
    for (VECFILEITEMS::iterator it = items.begin(); it != items.end(); ++it)
      (*it)->SetInvalid();
    CGUIDialog::SetInvalid();
    m_refreshTimeout.Set(MAX_INVALIDATION_FREQUENCY);
  }
}

void CGUIDialogPVRChannelsOSD::SaveControlStates()
{
  CGUIDialog::SaveControlStates();

  CPVRChannelGroupPtr group = GetPlayingGroup();
  if (group)
    SaveSelectedItemPath(group->GroupID());
}

void CGUIDialogPVRChannelsOSD::RestoreControlStates()
{
  CGUIDialog::RestoreControlStates();

  CPVRChannelGroupPtr group = GetPlayingGroup();
  if (group)
  {
    std::string path = GetLastSelectedItemPath(group->GroupID());
    if (!path.empty())
      m_viewControl.SetSelectedItem(path);
    else
      m_viewControl.SetSelectedItem(0);
  }
}

void CGUIDialogPVRChannelsOSD::Clear()
{
  m_viewControl.Clear();
  m_vecItems->Clear();
}

void CGUIDialogPVRChannelsOSD::GotoChannel(int item)
{
  if (item < 0 || item >= (int)m_vecItems->Size())
    return;

  Close();
  CServiceBroker::GetPVRManager().GUIActions()->SwitchToChannel(m_vecItems->Get(item), true /* bCheckResume */);
  m_group = GetPlayingGroup();
}

void CGUIDialogPVRChannelsOSD::ShowInfo(int item)
{
  if (item < 0 || item >= (int)m_vecItems->Size())
    return;

  CServiceBroker::GetPVRManager().GUIActions()->ShowEPGInfo(m_vecItems->Get(item));
}

void CGUIDialogPVRChannelsOSD::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_LIST));
}

void CGUIDialogPVRChannelsOSD::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();
  m_viewControl.Reset();
}

CGUIControl *CGUIDialogPVRChannelsOSD::GetFirstFocusableControl(int id)
{
  if (m_viewControl.HasControl(id))
    id = m_viewControl.GetCurrentControl();

  return CGUIWindow::GetFirstFocusableControl(id);
}

void CGUIDialogPVRChannelsOSD::Notify(const Observable &obs, const ObservableMessage msg)
{
  CGUIMessage m(GUI_MSG_REFRESH_LIST, GetID(), 0, msg);
  CApplicationMessenger::GetInstance().SendGUIMessage(m);
}

void CGUIDialogPVRChannelsOSD::SaveSelectedItemPath(int iGroupID)
{
  m_groupSelectedItemPaths[iGroupID] = m_viewControl.GetSelectedItemPath();
}

std::string CGUIDialogPVRChannelsOSD::GetLastSelectedItemPath(int iGroupID) const
{
  std::map<int, std::string>::const_iterator it = m_groupSelectedItemPaths.find(iGroupID);
  if (it != m_groupSelectedItemPaths.end())
    return it->second;
  return "";
}

void CGUIDialogPVRChannelsOSD::OnInputDone()
{
  const int iChannelNumber = GetChannelNumber();
  if (iChannelNumber >= 0)
  {
    int itemIndex = 0;
    for (const CFileItemPtr channel : m_vecItems->GetList())
    {
      if (channel->GetPVRChannelInfoTag()->ChannelNumber() == iChannelNumber)
      {
        m_viewControl.SetSelectedItem(itemIndex);
        return;
      }
      ++itemIndex;
    }
  }
}

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

#include "Application.h"
#include "FileItem.h"
#include "GUIInfoManager.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "epg/EpgContainer.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "view/ViewState.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/windows/GUIWindowPVRBase.h"

#include "GUIDialogPVRChannelsOSD.h"
#include "GUIDialogPVRGuideInfo.h"

using namespace PVR;
using namespace EPG;
using namespace KODI::MESSAGING;

#define MAX_INVALIDATION_FREQUENCY 2000 // limit to one invalidation per X milliseconds

#define CONTROL_LIST                  11

CGUIDialogPVRChannelsOSD::CGUIDialogPVRChannelsOSD() :
    CGUIDialog(WINDOW_DIALOG_PVR_OSD_CHANNELS, "DialogPVRChannelsOSD.xml")
{
  m_vecItems = new CFileItemList;
}

CGUIDialogPVRChannelsOSD::~CGUIDialogPVRChannelsOSD()
{
  delete m_vecItems;

  g_infoManager.UnregisterObserver(this);
  g_EpgContainer.UnregisterObserver(this);
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
  if (!g_PVRManager.IsPlayingTV() && !g_PVRManager.IsPlayingRadio())
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
      CGUIWindowPVRBase::SetSelectedItemPath(g_PVRManager.IsPlayingRadio(), GetLastSelectedItemPath(m_group->GroupID()));
      g_PVRManager.SetPlayingGroup(m_group);
    }
    else
    {
      CGUIWindowPVRBase::SetSelectedItemPath(g_PVRManager.IsPlayingRadio(), m_viewControl.GetSelectedItemPath());
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
      g_PVRManager.SetPlayingGroup(nextGroup);
      Update();

      // restore control states and previously selected item of group
      RestoreControlStates();
      return true;
    }
  }

  return CGUIDialog::OnAction(action);
}

CPVRChannelGroupPtr CGUIDialogPVRChannelsOSD::GetPlayingGroup()
{
  CPVRChannelPtr channel(g_PVRManager.GetCurrentChannel());
  if (channel)
    return g_PVRManager.GetPlayingGroup(channel->IsRadio());
  else
    return CPVRChannelGroupPtr();
}

void CGUIDialogPVRChannelsOSD::Update()
{
  g_infoManager.RegisterObserver(this);
  g_EpgContainer.RegisterObserver(this);

  // lock our display, as this window is rendered from the player thread
  g_graphicsContext.Lock();
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);

  // empty the list ready for population
  Clear();

  CPVRChannelPtr channel(g_PVRManager.GetCurrentChannel());
  if (channel)
  {
    CPVRChannelGroupPtr group = g_PVRManager.GetPlayingGroup(channel->IsRadio());
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

void CGUIDialogPVRChannelsOSD::CloseOrSelect(unsigned int iItem)
{
  if (CSettings::GetInstance().GetBool(CSettings::SETTING_PVRMENU_CLOSECHANNELOSDONSWITCH))
  {
    if (CSettings::GetInstance().GetInt(CSettings::SETTING_PVRMENU_DISPLAYCHANNELINFO) > 0)
      g_PVRManager.ShowPlayerInfo(CSettings::GetInstance().GetInt(CSettings::SETTING_PVRMENU_DISPLAYCHANNELINFO));
    Close();
  }
  else
    m_viewControl.SetSelectedItem(iItem);
}

void CGUIDialogPVRChannelsOSD::GotoChannel(int item)
{
  /* Check file item is in list range and get his pointer */
  if (item < 0 || item >= (int)m_vecItems->Size()) return;
  CFileItemPtr pItem = m_vecItems->Get(item);

  if (pItem->GetPath() == g_application.CurrentFile())
  {
    CloseOrSelect(item);
    return;
  }

  if (g_PVRManager.IsPlaying() && pItem->HasPVRChannelInfoTag() && g_application.m_pPlayer->HasPlayer())
  {
    CPVRChannelPtr channel = pItem->GetPVRChannelInfoTag();
    if (!g_PVRManager.CheckParentalLock(channel) ||
        !g_application.m_pPlayer->SwitchChannel(channel))
    {
      std::string msg = StringUtils::Format(g_localizeStrings.Get(19035).c_str(), channel->ChannelName().c_str()); // CHANNELNAME could not be played. Check the log for details.
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error,
              g_localizeStrings.Get(19166), // PVR information
              msg);
      return;
    }
  }
  else
    CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(new CFileItem(*pItem)));

  m_group = GetPlayingGroup();

  CloseOrSelect(item);
}

void CGUIDialogPVRChannelsOSD::ShowInfo(int item)
{
  /* Check file item is in list range and get his pointer */
  if (item < 0 || item >= (int)m_vecItems->Size()) return;

  CFileItemPtr pItem = m_vecItems->Get(item);
  if (pItem && pItem->IsPVRChannel())
  {
    CPVRChannelPtr channel(pItem->GetPVRChannelInfoTag());
    if (!g_PVRManager.CheckParentalLock(channel))
      return;

    /* Get the current running show on this channel from the EPG storage */
    CEpgInfoTagPtr epgnow(channel->GetEPGNow());
    if (!epgnow)
      return;

    /* Load programme info dialog */
    CGUIDialogPVRGuideInfo* pDlgInfo = (CGUIDialogPVRGuideInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GUIDE_INFO);
    if (!pDlgInfo)
      return;

    /* inform dialog about the file item and open dialog window */
    pDlgInfo->SetProgInfo(epgnow);
    pDlgInfo->Open();
  }

  return;
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

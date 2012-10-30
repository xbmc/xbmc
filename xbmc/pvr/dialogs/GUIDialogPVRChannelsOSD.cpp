/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIDialogPVRChannelsOSD.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "FileItem.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogOK.h"
#include "GUIDialogPVRGuideInfo.h"
#include "ViewState.h"
#include "settings/GUISettings.h"
#include "GUIInfoManager.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "epg/Epg.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"

using namespace std;
using namespace PVR;
using namespace EPG;

#define CONTROL_LIST                  11

CGUIDialogPVRChannelsOSD::CGUIDialogPVRChannelsOSD() :
    CGUIDialog(WINDOW_DIALOG_PVR_OSD_CHANNELS, "DialogPVRChannelsOSD.xml"),
    Observer()
{
  m_vecItems = new CFileItemList;
}

CGUIDialogPVRChannelsOSD::~CGUIDialogPVRChannelsOSD()
{
  delete m_vecItems;

  if (IsObserving(g_infoManager))
    g_infoManager.UnregisterObserver(this);
}

bool CGUIDialogPVRChannelsOSD::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      g_PVRManager.SetPlayingGroup(m_group);
      SetLastSelectedItem(m_group->GroupID());
      Clear();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      /* Close dialog immediately if now TV or radio channel is playing */
      if (!g_PVRManager.IsPlaying())
      {
        Close();
        return true;
      }

      m_group = GetPlayingGroup();

      CGUIWindow::OnMessage(message);
      Update(true);

      return true;
    }
    break;

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

  case GUI_MSG_MOVE:
    {
      int iAction = message.GetParam1();

      if (iAction == ACTION_MOVE_RIGHT || iAction == ACTION_MOVE_LEFT)
      {
        CPVRChannelGroupPtr group = GetPlayingGroup();
        CPVRChannelGroupPtr nextGroup = iAction == ACTION_MOVE_RIGHT ? group->GetNextGroup() : group->GetPreviousGroup();
        g_PVRManager.SetPlayingGroup(nextGroup);
        SetLastSelectedItem(group->GroupID());

        Update();

        return true;
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

CPVRChannelGroupPtr CGUIDialogPVRChannelsOSD::GetPlayingGroup()
{
  CPVRChannelPtr channel;
  g_PVRManager.GetCurrentChannel(channel);
  return g_PVRManager.GetPlayingGroup(channel->IsRadio());
}

void CGUIDialogPVRChannelsOSD::Update()
{
  CGUIDialogPVRChannelsOSD::Update(false);
}

void CGUIDialogPVRChannelsOSD::Update(bool selectPlayingChannel)
{
  // lock our display, as this window is rendered from the player thread
  g_graphicsContext.Lock();

  if (!IsObserving(g_infoManager))
    g_infoManager.RegisterObserver(this);

  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);

  // empty the list ready for population
  Clear();

  CPVRChannelPtr channel;
  g_PVRManager.GetCurrentChannel(channel);
  CPVRChannelGroupPtr group = g_PVRManager.GetPlayingGroup(channel->IsRadio());

  if (group)
  {
    group->GetMembers(*m_vecItems);
    m_viewControl.SetItems(*m_vecItems);
    m_viewControl.SetSelectedItem(selectPlayingChannel ? group->GetIndex(*channel) : GetLastSelectedItem(group->GroupID()));
  }

  g_graphicsContext.Unlock();
}

void CGUIDialogPVRChannelsOSD::Clear()
{
  m_viewControl.Clear();
  m_vecItems->Clear();
}

void CGUIDialogPVRChannelsOSD::CloseOrSelect(unsigned int iItem)
{
  if (g_guiSettings.GetBool("pvrmenu.closechannelosdonswitch"))
    Close();
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

  if (g_PVRManager.IsPlaying() && pItem->HasPVRChannelInfoTag() && g_application.m_pPlayer)
  {
    CPVRChannel *channel = pItem->GetPVRChannelInfoTag();
    if (!g_PVRManager.CheckParentalLock(*channel) ||
        !g_application.m_pPlayer->SwitchChannel(*channel))
    {
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error,
              g_localizeStrings.Get(19166), // PVR information
              g_localizeStrings.Get(19035)); // This channel cannot be played. Check the log for details.
      return;
    }
  }
  else
    CApplicationMessenger::Get().PlayFile(*pItem);

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
    CPVRChannel *channel = pItem->GetPVRChannelInfoTag();
    if (!g_PVRManager.CheckParentalLock(*channel))
      return;

    /* Get the current running show on this channel from the EPG storage */
    CEpgInfoTag epgnow;
    if (!channel->GetEPGNow(epgnow))
      return;
    CFileItem *itemNow  = new CFileItem(epgnow);

    /* Load programme info dialog */
    CGUIDialogPVRGuideInfo* pDlgInfo = (CGUIDialogPVRGuideInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GUIDE_INFO);
    if (!pDlgInfo)
      return;

    /* inform dialog about the file item and open dialog window */
    pDlgInfo->SetProgInfo(itemNow);
    pDlgInfo->DoModal();
    delete itemNow; /* delete previuosly created FileItem */
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
  if (msg == ObservableMessageCurrentItem)
  {
    g_graphicsContext.Lock();
    m_viewControl.SetItems(*m_vecItems);
    g_graphicsContext.Unlock();
  }
}

void CGUIDialogPVRChannelsOSD::SetLastSelectedItem(int iGroupID)
{
  m_groupSelectedItems[iGroupID] = m_viewControl.GetSelectedItem();
}

int CGUIDialogPVRChannelsOSD::GetLastSelectedItem(int iGroupID) const
{
  map<int,int>::const_iterator it = m_groupSelectedItems.find(iGroupID);
  if(it != m_groupSelectedItems.end())
    return it->second;
  return 0;
}

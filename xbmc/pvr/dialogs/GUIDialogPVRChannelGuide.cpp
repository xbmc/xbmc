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

#include "GUIDialogPVRChannelGuide.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "view/ViewState.h"

#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/epg/Epg.h"

using namespace PVR;

#define CONTROL_LIST  11

CGUIDialogPVRChannelGuide::CGUIDialogPVRChannelGuide()
    : CGUIDialog(WINDOW_DIALOG_PVR_CHANNEL_GUIDE, "DialogPVRChannelGuide.xml")
{
  m_vecItems.reset(new CFileItemList);
}

CGUIDialogPVRChannelGuide::~CGUIDialogPVRChannelGuide() = default;

bool CGUIDialogPVRChannelGuide::OnMessage(CGUIMessage& message)
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
          ShowInfo(iItem);
          return true;
        }
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogPVRChannelGuide::Open(const CPVRChannelPtr &channel)
{
  m_channel = channel;
  CGUIDialog::Open();
}

void CGUIDialogPVRChannelGuide::OnInitWindow()
{
  // no user-specific channel is set use current playing channel
  if (!m_channel)
    m_channel = CServiceBroker::GetPVRManager().GetCurrentChannel();

  // no channel at all, close the dialog
  if (!m_channel)
  {
    Close();
    return;
  }

  // lock our display, as this window is rendered from the player thread
  g_graphicsContext.Lock();
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);

  // empty the list ready for population
  Clear();

  m_channel->GetEPG(*m_vecItems);
  m_viewControl.SetItems(*m_vecItems);

  g_graphicsContext.Unlock();

  // call init
  CGUIDialog::OnInitWindow();

  // select the active entry
  unsigned int iSelectedItem = 0;
  for (int iEpgPtr = 0; iEpgPtr < m_vecItems->Size(); ++iEpgPtr)
  {
    CFileItemPtr entry = m_vecItems->Get(iEpgPtr);
    if (entry->HasEPGInfoTag() && entry->GetEPGInfoTag()->IsActive())
    {
      iSelectedItem = iEpgPtr;
      break;
    }
  }
  m_viewControl.SetSelectedItem(iSelectedItem);
}

void CGUIDialogPVRChannelGuide::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
  m_channel.reset();
  Clear();
}

void CGUIDialogPVRChannelGuide::Clear()
{
  m_viewControl.Clear();
  m_vecItems->Clear();
}

void CGUIDialogPVRChannelGuide::ShowInfo(int item)
{
  if (item < 0 || item >= (int)m_vecItems->Size())
    return;

  CServiceBroker::GetPVRManager().GUIActions()->ShowEPGInfo(m_vecItems->Get(item));
}

void CGUIDialogPVRChannelGuide::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_LIST));
}

void CGUIDialogPVRChannelGuide::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();
  m_viewControl.Reset();
}

CGUIControl *CGUIDialogPVRChannelGuide::GetFirstFocusableControl(int id)
{
  if (m_viewControl.HasControl(id))
    id = m_viewControl.GetCurrentControl();

  return CGUIWindow::GetFirstFocusableControl(id);
}

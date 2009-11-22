/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "GUIDialogPVRChannelsOSD.h"
#include "PVRManager.h"
#include "Application.h"
#include "GUIDialogPVRGuideInfo.h"
#include "GUIDialogOK.h"
#include "GUIWindowManager.h"
#include "FileItem.h"
#include "ViewState.h"

using namespace std;

#define CONTROL_LIST                  11

CGUIDialogPVRChannelsOSD::CGUIDialogPVRChannelsOSD()
    : CGUIDialog(WINDOW_DIALOG_PVR_OSD_CHANNELS, "DialogPVRChannelsOSD.xml")
{
  m_vecItems = new CFileItemList;
}

CGUIDialogPVRChannelsOSD::~CGUIDialogPVRChannelsOSD()
{
  delete m_vecItems;
}

bool CGUIDialogPVRChannelsOSD::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      Clear();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      Update();
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
        else if (iAction == ACTION_SHOW_INFO)
        {
          /* Show information Dialog */
          ShowInfo(iItem);
          return true;
        }
      }
    }
    break;
  case GUI_MSG_SETFOCUS:
    {
      if (m_viewControl.HasControl(message.GetControlId()) && m_viewControl.GetCurrentControl() != message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogPVRChannelsOSD::Update()
{
  // lock our display, as this window is rendered from the player thread
  g_graphicsContext.Lock();
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);

  // empty the list ready for population
  Clear();

  bool RadioPlaying;
  int CurrentChannel;
  g_PVRManager.GetCurrentChannel(&CurrentChannel, &RadioPlaying);

  if (!RadioPlaying)
    PVRChannelsTV.GetChannels(m_vecItems, g_PVRManager.GetPlayingGroup());
  else
    PVRChannelsRadio.GetChannels(m_vecItems, g_PVRManager.GetPlayingGroup());

  m_viewControl.SetItems(*m_vecItems);
  m_viewControl.SetSelectedItem(CurrentChannel-1);
  g_graphicsContext.Unlock();
}

void CGUIDialogPVRChannelsOSD::Clear()
{
  m_viewControl.Clear();
  m_vecItems->Clear();
}

void CGUIDialogPVRChannelsOSD::GotoChannel(int item)
{
  /* Check file item is in list range and get his pointer */
  if (item < 0 || item >= (int)m_vecItems->Size()) return;

  CFileItemPtr pItem = m_vecItems->Get(item);

  if (pItem->m_strPath == g_application.CurrentFile())
  {
    Close();
    return;
  }

  if (!g_application.PlayFile(*pItem))
  {
    CGUIDialogOK::ShowAndGetInput(18100,0,18134,0);
  }
}

void CGUIDialogPVRChannelsOSD::ShowInfo(int item)
{
  /* Check file item is in list range and get his pointer */
  if (item < 0 || item >= (int)m_vecItems->Size()) return;

  CFileItemPtr pItem = m_vecItems->Get(item);
  if (pItem && pItem->IsPVRChannel())
  {
    /* Lock first the EPG storage to make sure no one other change it */
    cPVREpgsLock EpgsLock;
    cPVREpgs *s = (cPVREpgs *)cPVREpgs::EPGs(EpgsLock);
    if (s)
    {
      /* Get the current running show on this channel from the EPG storage */
      const cPVREPGInfoTag *epgnow = s->GetEPG(pItem->GetPVRChannelInfoTag(), true)->GetInfoTagNow();
      CFileItem *itemNow  = new CFileItem(*epgnow);

      /* Load programme info dialog */
      CGUIDialogPVRGuideInfo* pDlgInfo = (CGUIDialogPVRGuideInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GUIDE_INFO);
      if (!pDlgInfo)
        return;

      /* inform dialog about the file item and open dialog window */
      pDlgInfo->SetProgInfo(itemNow);
      pDlgInfo->DoModal();
      delete itemNow; /* delete previuosly created FileItem */
    }
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

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

#include "GUIDialogPVRGuideOSD.h"
#include "Application.h"
#include "FileItem.h"
#include "GUIDialogPVRGuideInfo.h"
#include "guilib/GUIWindowManager.h"
#include "ViewState.h"
#include "epg/Epg.h"

#include "pvr/PVRManager.h"

using namespace std;
using namespace PVR;

#define CONTROL_LIST  11

CGUIDialogPVRGuideOSD::CGUIDialogPVRGuideOSD()
    : CGUIDialog(WINDOW_DIALOG_PVR_OSD_GUIDE, "DialogPVRGuideOSD.xml")
{
  m_vecItems = new CFileItemList;
}

CGUIDialogPVRGuideOSD::~CGUIDialogPVRGuideOSD()
{
  delete m_vecItems;
}

bool CGUIDialogPVRGuideOSD::OnMessage(CGUIMessage& message)
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
      /* Close dialog immediately if now TV or radio channel is playing */
      if (!g_PVRManager.IsPlaying())
      {
        Close();
        return true;
      }
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
          ShowInfo(iItem);
          return true;
        }
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogPVRGuideOSD::Update()
{
  // lock our display, as this window is rendered from the player thread
  g_graphicsContext.Lock();
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);

  // empty the list ready for population
  Clear();

  g_PVRManager.GetCurrentEpg(*m_vecItems);
  m_viewControl.SetItems(*m_vecItems);

  /* select the active entry */
  unsigned int iSelectedItem = 0;
  for (int iEpgPtr = 0; iEpgPtr < m_vecItems->Size(); iEpgPtr++)
  {
    CFileItemPtr entry = m_vecItems->Get(iEpgPtr);
    if (entry->GetEPGInfoTag()->IsActive())
    {
      iSelectedItem = iEpgPtr;
      break;
    }
  }
  m_viewControl.SetSelectedItem(iSelectedItem);

  g_graphicsContext.Unlock();
}

void CGUIDialogPVRGuideOSD::Clear()
{
  m_viewControl.Clear();
  m_vecItems->Clear();
}

void CGUIDialogPVRGuideOSD::ShowInfo(int item)
{
  /* Check file item is in list range and get his pointer */
  if (item < 0 || item >= (int)m_vecItems->Size()) return;

  CFileItemPtr pItem = m_vecItems->Get(item);

  /* Load programme info dialog */
  CGUIDialogPVRGuideInfo* pDlgInfo = (CGUIDialogPVRGuideInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GUIDE_INFO);
  if (!pDlgInfo)
    return;

  /* inform dialog about the file item and open dialog window */
  pDlgInfo->SetProgInfo(pItem.get());
  pDlgInfo->DoModal();
}

void CGUIDialogPVRGuideOSD::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_LIST));
}

void CGUIDialogPVRGuideOSD::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();
  m_viewControl.Reset();
}

CGUIControl *CGUIDialogPVRGuideOSD::GetFirstFocusableControl(int id)
{
  if (m_viewControl.HasControl(id))
    id = m_viewControl.GetCurrentControl();

  return CGUIWindow::GetFirstFocusableControl(id);
}

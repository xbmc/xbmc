/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogVideoOSD.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/InputManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/mouse/MouseEvent.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

using namespace KODI;
using namespace PVR;

CGUIDialogVideoOSD::CGUIDialogVideoOSD(void)
    : CGUIDialog(WINDOW_DIALOG_VIDEO_OSD, "VideoOSD.xml")
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogVideoOSD::~CGUIDialogVideoOSD(void) = default;

void CGUIDialogVideoOSD::FrameMove()
{
  if (m_autoClosing)
  {
    // check for movement of mouse or a submenu open
    if (CServiceBroker::GetInputManager().IsMouseActive()
                           || CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_DIALOG_AUDIO_OSD_SETTINGS)
                           || CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_DIALOG_SUBTITLE_OSD_SETTINGS)
                           || CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_DIALOG_VIDEO_OSD_SETTINGS)
                           || CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_DIALOG_CMS_OSD_SETTINGS)
                           || CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_DIALOG_VIDEO_BOOKMARKS)
                           || CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_DIALOG_PVR_OSD_CHANNELS)
                           || CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_DIALOG_PVR_CHANNEL_GUIDE)
                           || CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_DIALOG_OSD_TELETEXT))
      // extend show time by original value
      SetAutoClose(m_showDuration);
  }
  CGUIDialog::FrameMove();
}

bool CGUIDialogVideoOSD::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SHOW_OSD)
  {
    Close();
    return true;
  }

  return CGUIDialog::OnAction(action);
}

EVENT_RESULT CGUIDialogVideoOSD::OnMouseEvent(const CPoint& point, const MOUSE::CMouseEvent& event)
{
  if (event.m_id == ACTION_MOUSE_WHEEL_UP)
  {
    return g_application.OnAction(CAction(ACTION_ANALOG_SEEK_FORWARD, 0.5f)) ? EVENT_RESULT_HANDLED : EVENT_RESULT_UNHANDLED;
  }
  if (event.m_id == ACTION_MOUSE_WHEEL_DOWN)
  {
    return g_application.OnAction(CAction(ACTION_ANALOG_SEEK_BACK, 0.5f)) ? EVENT_RESULT_HANDLED : EVENT_RESULT_UNHANDLED;
  }

  return CGUIDialog::OnMouseEvent(point, event);
}

bool CGUIDialogVideoOSD::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_VIDEO_MENU_STARTED:
    {
      // We have gone to the DVD menu, so close the OSD.
      Close();
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:  // fired when OSD is hidden
    {
      // Remove our subdialogs if visible
      CGUIDialog *pDialog = CServiceBroker::GetGUI()->GetWindowManager().GetDialog(WINDOW_DIALOG_AUDIO_OSD_SETTINGS);
      if (pDialog && pDialog->IsDialogRunning())
        pDialog->Close(true);
      pDialog = CServiceBroker::GetGUI()->GetWindowManager().GetDialog(WINDOW_DIALOG_SUBTITLE_OSD_SETTINGS);
      if (pDialog && pDialog->IsDialogRunning())
        pDialog->Close(true);
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}


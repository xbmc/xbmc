/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowStartup.h"

#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"

CGUIWindowStartup::CGUIWindowStartup(void)
    : CGUIWindow(WINDOW_STARTUP_ANIM, "Startup.xml")
{
}

CGUIWindowStartup::~CGUIWindowStartup(void) = default;

bool CGUIWindowStartup::OnAction(const CAction &action)
{
  if (action.IsMouse())
    return true;
  return CGUIWindow::OnAction(action);
}

void CGUIWindowStartup::OnDeinitWindow(int nextWindowID)
{
  CGUIWindow::OnDeinitWindow(nextWindowID);

  // let everyone know that the user interface is now ready for usage
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UI_READY);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

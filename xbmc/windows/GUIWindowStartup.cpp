/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "GUIWindowStartup.h"
#include "ServiceBroker.h"
#include "input/Key.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"

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

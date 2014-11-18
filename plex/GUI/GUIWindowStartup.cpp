/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "GUIWindowStartup.h"
#include "guilib/Key.h"
#include "PlexApplication.h"
#include "Client/MyPlex/MyPlexManager.h"
#include "dialogs/GUIDialogNumeric.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "Application.h"
#include "GUI/GUIDialogPlexUserSelect.h"

CGUIWindowStartup::CGUIWindowStartup(void)
    : CGUIWindow(WINDOW_STARTUP_ANIM, "Startup.xml")
{
}

CGUIWindowStartup::~CGUIWindowStartup(void)
{
}

void CGUIWindowStartup::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  
  if (g_plexApplication.myPlexManager->IsPinProtected())
  {
    CGUIDialogPlexUserSelect* dialog = (CGUIDialogPlexUserSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_PLEX_USER_SELECT);
    if (dialog)
    {
      while(true)
      {
        dialog->DoModal();
        if (dialog->DidAuth())
          break;
      }
    }
  }
}

bool CGUIWindowStartup::OnAction(const CAction &action)
{
  if (action.IsMouse())
    return true;
  return CGUIWindow::OnAction(action);
}

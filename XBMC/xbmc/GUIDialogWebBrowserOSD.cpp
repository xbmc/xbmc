/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIDialogWebBrowserOSD.h"
#include "GUIWindowSettingsCategory.h"

#ifdef WITH_LINKS_BROWSER

#define CONTROL_EXPERTMODE_BUTTON      501

CGUIDialogWebBrowserOSD::CGUIDialogWebBrowserOSD(void)
    : CGUIDialog(WINDOW_DIALOG_WEB_OSD, "WebBrowserOSD.xml")
{
  m_iLastControl = -1;
  LoadOnDemand(false);    // we are loaded by the web browser window.
}

CGUIDialogWebBrowserOSD::~CGUIDialogWebBrowserOSD(void)
{
}

bool CGUIDialogWebBrowserOSD::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {
      unsigned int iControl = message.GetSenderId();
      if (iControl == CONTROL_EXPERTMODE_BUTTON)
      {
        g_browserManager.SetExpertMode((g_browserManager.GetExpertMode()) ? FALSE : TRUE);
      }
      return true;
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);

      m_gWindowManager.m_bPointerNav = false;
      g_Mouse.SetInactive();
      SET_CONTROL_FOCUS(m_dwDefaultFocusControlID, 0);
      return true;
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogWebBrowserOSD::Render()
{
  CGUIDialog::Render();
}

#endif
/*
 *      Copyright (C) 2010 Team XBMC
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

#include "GUIDialogWebBrowserOSD.h"
#include "GUIEmbeddedBrowserWindowObserver.h"
#include "GUIWindowManager.h"
#include "GUIWebBrowserControl.h"
#include "GUIDialogKeyboard.h"
#include "Key.h"
#include "LocalizeStrings.h"
#include <cstdio>

#define ID_BUTTON_BACK        1
#define ID_BUTTON_FORWARD     2
#define ID_BUTTON_RELOAD      3
#define ID_BUTTON_STOP        4
#define ID_BUTTON_HOME        5
#define ID_BUTTON_ADDRESS_BAR 6
#define ID_BUTTON_GO          7

CGUIDialogWebBrowserOSD::CGUIDialogWebBrowserOSD(void)
  : CGUIDialog(WINDOW_DIALOG_WEB_BROWSER_OSD, "WebBrowserOSD.xml")
{}

bool CGUIDialogWebBrowserOSD::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == ID_BUTTON_BACK)
      {
        g_webBrowserObserver.Back();
        return true;
      }
      else if (iControl == ID_BUTTON_FORWARD)
      {
        g_webBrowserObserver.Forward();
        return true;
      }
      else if (iControl == ID_BUTTON_RELOAD)
      {
        g_webBrowserObserver.Reload();
        return true;
      }
      else if (iControl == ID_BUTTON_STOP)
      {
        g_webBrowserObserver.Stop();
        return true;
      }
      else if (iControl == ID_BUTTON_HOME)
      {
        g_webBrowserObserver.Home();
        return true;
      }
      else if (iControl == ID_BUTTON_GO)
      {
        g_webBrowserObserver.Go(m_url);
        return true;
      }
      else if (iControl == ID_BUTTON_ADDRESS_BAR)
      {
        CGUIDialogKeyboard::ShowAndGetInput(m_url, g_localizeStrings.Get(1051), false);
        g_webBrowserObserver.Go(m_url);
        return true;
      }
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogWebBrowserOSD::OnAction(const CAction &action)
{
  return CGUIDialog::OnAction(action);
}

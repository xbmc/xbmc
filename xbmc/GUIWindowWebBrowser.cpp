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

#include "GUIWindowWebBrowser.h"
#include "GUIWindowManager.h"
#include "Key.h"

CGUIWindowWebBrowser::CGUIWindowWebBrowser(void)
  : CGUIWindow(WINDOW_WEB_BROWSER, "WebBrowser.xml")
{}

bool CGUIWindowWebBrowser::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
  case ACTION_PREVIOUS_MENU:
  case ACTION_PARENT_DIR:
    g_windowManager.PreviousWindow();
    return true;
  default:
    return CGUIWindow::OnAction(action);
  }
}

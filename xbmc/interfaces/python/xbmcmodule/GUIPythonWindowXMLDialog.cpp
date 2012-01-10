/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "GUIPythonWindowXMLDialog.h"
#include "guilib/GUIWindowManager.h"
#include "Application.h"
#include "threads/SingleLock.h"

CGUIPythonWindowXMLDialog::CGUIPythonWindowXMLDialog(int id, CStdString strXML, CStdString strFallBackPath)
: CGUIPythonWindowXML(id,strXML,strFallBackPath)
{
  m_loadOnDemand = false;
}

CGUIPythonWindowXMLDialog::~CGUIPythonWindowXMLDialog(void)
{
}

bool CGUIPythonWindowXMLDialog::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_WINDOW_DEINIT)
  {
    CGUIWindow *pWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
    if (pWindow)
      g_windowManager.ShowOverlay(pWindow->GetOverlayState());
    return CGUIWindow::OnMessage(message);
  }
  return CGUIPythonWindowXML::OnMessage(message);
}

void CGUIPythonWindowXMLDialog::Show_Internal(bool show /* = true */)
{
  if (show)
  {
    g_windowManager.RouteToWindow(this);

    // active this dialog...
    CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0);
    OnMessage(msg);
    m_active = true;
  }
  else // hide
    Close();
}

void CGUIPythonWindowXMLDialog::OnDeinitWindow(int nextWindowID)
{
  g_windowManager.RemoveDialog(GetID());
  CGUIPythonWindowXML::OnDeinitWindow(nextWindowID);
}

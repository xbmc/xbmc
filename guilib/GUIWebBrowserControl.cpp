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

#include "GUIWebBrowserControl.h"
#include "Application.h"
#include "Key.h"
#include <cstdio>

CGUIWebBrowserControl::CGUIWebBrowserControl(DWORD dwParentID,
      DWORD dwControlId, float posX, float posY, float width, float height)
  : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  ControlType = GUICONTROL_WEB_BROWSER;
}

void CGUIWebBrowserControl::AllocResources()
{
  g_webBrowserObserver.SetParams(GetXPosition(), GetYPosition(), GetWidth(),
                        GetHeight());
  g_webBrowserObserver.init();
}

void CGUIWebBrowserControl::Render()
{
  g_webBrowserObserver.Render(GetXPosition(), GetYPosition(), GetWidth(), GetHeight());
  CGUIControl::Render();
}

bool CGUIWebBrowserControl::OnAction(const CAction &action)
{
  if (g_webBrowserObserver.keyboard(action))
    return true;
  return CGUIControl::OnAction(action);
}

EVENT_RESULT CGUIWebBrowserControl::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  EVENT_RESULT result = g_webBrowserObserver.mouseEvent(point, event);
  if (result != EVENT_RESULT_UNHANDLED)
    return result;
  return CGUIControl::OnMouseEvent(point, event);
}

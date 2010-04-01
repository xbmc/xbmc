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
#include "Key.h"

CGUIWebBrowserControl::CGUIWebBrowserControl(DWORD dwParentID,
      DWORD dwControlId, float posX, float posY, float width, float height)
  : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  m_texture = 0;
  m_observer = new CGUIEmbeddedBrowserWindowObserver(posX, posY, width, height);
  ControlType = GUICONTROL_WEB_BROWSER;
}

CGUIWebBrowserControl::~CGUIWebBrowserControl()
{}

void CGUIWebBrowserControl::AllocResources()
{
  try
  {
    m_observer->init();
  }
  catch (...)
  {
    //CLog::Log(LOGERROR, "Exception in CGUIWebBrowserControl::Render()");
  }
}

void CGUIWebBrowserControl::FreeResources()
{
  if (m_texture)
  {
    glDeleteTextures(1, &m_texture);
    m_texture = 0;
  }
  if (m_observer)
  {
    delete m_observer;
    m_observer = NULL;
  }
}

void CGUIWebBrowserControl::Render()
{
  m_observer->Render(GetXPosition(), GetYPosition(), GetWidth(), GetHeight());
  CGUIControl::Render();
}

EVENT_RESULT CGUIWebBrowserControl::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_LEFT_CLICK)
  {
    //TODO: implement handling of mouse events
    return EVENT_RESULT_HANDLED;
  }
  return CGUIControl::OnMouseEvent(point, event);
}

void CGUIWebBrowserControl::Back()
{
  //TODO: implement navigating back
}

void CGUIWebBrowserControl::Forward()
{
  //TODO: implement navigating forward
}

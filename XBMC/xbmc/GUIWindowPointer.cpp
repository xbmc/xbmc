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
#include "GUIWindowPointer.h"

//FIXME This should be somewere else. Probably were the include for Mouse.h is
#ifdef HAS_CWIID
#include "common/WiiRemote.h"
#endif


#define ID_POINTER 10

CGUIWindowPointer::CGUIWindowPointer(void)
    : CGUIWindow(105, "Pointer.xml")
{
  m_dwPointer = 0;
  m_loadOnDemand = false;
  m_needsScaling = false;
}

CGUIWindowPointer::~CGUIWindowPointer(void)
{}

void CGUIWindowPointer::Move(int x, int y)
{
  float posX = m_posX + x;
  float posY = m_posY + y;
  if (posX < 0) posX = 0;
  if (posY < 0) posY = 0;
  if (posX > g_graphicsContext.GetWidth()) posX = (float)g_graphicsContext.GetWidth();
  if (posY > g_graphicsContext.GetHeight()) posY = (float)g_graphicsContext.GetHeight();
  SetPosition(posX, posY);
}

void CGUIWindowPointer::SetPointer(DWORD dwPointer)
{
  if (m_dwPointer == dwPointer) return ;
  // set the new pointer visible
  CGUIControl *pControl = (CGUIControl *)GetControl(dwPointer);
  if (pControl)
  {
    pControl->SetVisible(true);
    // disable the old pointer
    pControl = (CGUIControl *)GetControl(m_dwPointer);
    if (pControl) pControl->SetVisible(false);
    // set pointer to the new one
    m_dwPointer = dwPointer;
  }
}

void CGUIWindowPointer::OnWindowLoaded()
{ // set all our pointer images invisible
  for (ivecControls i = m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl = *i;
    pControl->SetVisible(false);
  }
  CGUIWindow::OnWindowLoaded();
  DynamicResourceAlloc(false);
}

void CGUIWindowPointer::Render()
{
#ifdef HAS_CWIID
  // Depending if we prioritize mouse or wiiremote. If Mouse then the wiiremote wins if it have IR sources and the mouse is active but hasn't moved since last update
  // if we prioritize the wiiremote the outcome is the opposite, the mouse wins if it has moved since last update and the wiiremote is at most active with no IR sources
#ifndef WIIREMOTE_PRIORITIZE_MOUSE
  if (g_WiiRemote.HaveIRSources())
  {
    float x, y;
    x = g_graphicsContext.GetWidth()  * g_WiiRemote.GetMouseX();
    y = g_graphicsContext.GetHeight() * g_WiiRemote.GetMouseY();
       
    SetPosition(x, y);
    SetPointer(MOUSE_STATE_NORMAL);    //I am a bit uncertain if this should/could be anything else
  }
  else
  {
    CPoint location(g_Mouse.GetLocation());
    SetPosition(location.x, location.y);
    SetPointer(g_Mouse.GetState());
  }
#else
  if (!g_Mouse.HasMoved() && g_WiiRemote.HaveIRSources())
  {
    float x, y;
    x = g_graphicsContext.GetWidth()  * g_WiiRemote.GetMouseX();
    y = g_graphicsContext.GetHeight() * g_WiiRemote.GetMouseY();
    
    SetPosition(x, y);
    SetPointer(MOUSE_STATE_NORMAL);    
  }
  else
  {
    CPoint location(g_Mouse.GetLocation());
    SetPosition(location.x, location.y);
    SetPointer(g_Mouse.GetState());
  }
#endif

#else
  CPoint location(g_Mouse.GetLocation());
  SetPosition(location.x, location.y);
  SetPointer(g_Mouse.GetState());
#endif
  CGUIWindow::Render();
}


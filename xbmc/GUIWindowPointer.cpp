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
  CPoint location(g_Mouse.GetLocation());
  SetPosition(location.x, location.y);
  SetPointer(g_Mouse.GetState());
  CGUIWindow::Render();
}
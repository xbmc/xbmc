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

#include "GUIWindowPointer.h"
#include "input/MouseStat.h"
#include "windowing/WindowingFactory.h"
#include <climits>
#define ID_POINTER 10

CGUIWindowPointer::CGUIWindowPointer(void)
    : CGUIDialog(WINDOW_DIALOG_POINTER, "Pointer.xml")
{
  m_pointer = 0;
  m_loadType = LOAD_ON_GUI_INIT;
  m_needsScaling = false;
  m_active = false;
  m_renderOrder = INT_MAX - 1;
}

CGUIWindowPointer::~CGUIWindowPointer(void)
{
}

void CGUIWindowPointer::SetPointer(int pointer)
{
  if (m_pointer == pointer) return;
  // set the new pointer visible
  CGUIControl *pControl = (CGUIControl *)GetControl(pointer);
  if (pControl)
  {
    pControl->SetVisible(true);
    // disable the old pointer
    pControl = (CGUIControl *)GetControl(m_pointer);
    if (pControl) pControl->SetVisible(false);
    // set pointer to the new one
    m_pointer = pointer;
  }
}

void CGUIWindowPointer::UpdateVisibility()
{
  if(g_Windowing.HasCursor())
  {
    if (g_Mouse.IsActive())
      Show();
    else
      Close();
  }
}

void CGUIWindowPointer::OnWindowLoaded()
{ // set all our pointer images invisible
  for (iControls i = m_children.begin();i != m_children.end(); ++i)
  {
    CGUIControl* pControl = *i;
    pControl->SetVisible(false);
  }
  CGUIWindow::OnWindowLoaded();
  DynamicResourceAlloc(false);
  m_pointer = 0;
  m_renderOrder = INT_MAX - 1;
}

void CGUIWindowPointer::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  bool active = g_Mouse.IsActive();
  if (active != m_active)
  {
    MarkDirtyRegion();
    m_active = active;
  }
  SetPosition((float)g_Mouse.GetX(), (float)g_Mouse.GetY());
  SetPointer(g_Mouse.GetState());
  return CGUIWindow::Process(currentTime, dirtyregions);
}

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

#include "include.h"
#include "GUIControlGroupList.h"
#include "utils/GUIInfoManager.h"

#define TIME_TO_SCROLL 200;

CGUIControlGroupList::CGUIControlGroupList(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, float itemGap, DWORD pageControl, ORIENTATION orientation, bool useControlPositions)
: CGUIControlGroup(dwParentID, dwControlId, posX, posY, width, height)
{
  m_itemGap = itemGap;
  m_pageControl = pageControl;
  m_offset = 0;
  m_totalSize = 10;
  m_orientation = orientation;
  m_scrollOffset = 0;
  m_scrollSpeed = 0;
  m_useControlPositions = useControlPositions;
  ControlType = GUICONTROL_GROUPLIST;
}

CGUIControlGroupList::~CGUIControlGroupList(void)
{
}

void CGUIControlGroupList::Render()
{
  if (m_scrollSpeed != 0)
  {
    m_offset += m_scrollSpeed * (m_renderTime - m_scrollTime);
    if ((m_scrollSpeed < 0 && m_offset < m_scrollOffset) ||
        (m_scrollSpeed > 0 && m_offset > m_scrollOffset))
    {
      m_offset = m_scrollOffset;
      m_scrollSpeed = 0;
    }
  }
  m_scrollTime = m_renderTime;

  ValidateOffset();
  if (m_pageControl)
  {
    CGUIMessage message(GUI_MSG_LABEL_RESET, GetParentID(), m_pageControl, (DWORD)m_height, (DWORD)m_totalSize);
    SendWindowMessage(message);
    CGUIMessage message2(GUI_MSG_ITEM_SELECT, GetParentID(), m_pageControl, (DWORD)m_offset);
    SendWindowMessage(message2);
  }
  // we run through the controls, rendering as we go
  bool render(g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height));
  float pos = 0;
  float focusedPos = 0;
  CGUIControl *focusedControl = NULL;
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    // note we render all controls, even if they're offscreen, as then they'll be updated
    // with respect to animations
    CGUIControl *control = *it;
    control->UpdateVisibility();
    if (m_renderFocusedLast && control->HasFocus())
    {
      focusedControl = control;
      focusedPos = pos;
    }
    else
    {
      if (m_orientation == VERTICAL)
        g_graphicsContext.SetOrigin(m_posX, m_posY + pos - m_offset);
      else
        g_graphicsContext.SetOrigin(m_posX + pos - m_offset, m_posY);
      control->DoRender(m_renderTime);
    }
    if (control->IsVisible())
      pos += Size(control) + m_itemGap;
    g_graphicsContext.RestoreOrigin();
  }
  if (focusedControl)
  {
    if (m_orientation == VERTICAL)
      g_graphicsContext.SetOrigin(m_posX, m_posY + focusedPos - m_offset);
    else
      g_graphicsContext.SetOrigin(m_posX + focusedPos - m_offset, m_posY);
    focusedControl->DoRender(m_renderTime);
  }
  if (render) g_graphicsContext.RestoreClipRegion();
  CGUIControl::Render();
}

bool CGUIControlGroupList::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage() )
  {
  case GUI_MSG_FOCUSED:
    { // a control has been focused
      // scroll if we need to and update our page control
      ValidateOffset();
      float offset = 0;
      for (iControls it = m_children.begin(); it != m_children.end(); ++it)
      {
        CGUIControl *control = *it;
        if (!control->IsVisible())
          continue;
        if (control->HasID(message.GetControlId()))
        {
          if (offset < m_offset)
            ScrollTo(offset);
          else if (offset + Size(control) > m_offset + Size())
            ScrollTo(offset + Size(control) - Size());
          break;
        }
        offset += Size(control) + m_itemGap;
      }
    }
    break;
  case GUI_MSG_SETFOCUS:
    {
      // we've been asked to focus.  We focus the last control if it's on this page,
      // else we'll focus the first focusable control from our offset (after verifying it)
      ValidateOffset();
      // now check the focusControl's offset
      float offset = 0;
      for (iControls it = m_children.begin(); it != m_children.end(); ++it)
      {
        CGUIControl *control = *it;
        if (!control->IsVisible())
          continue;
        if (control->HasID(m_focusedControl))
        {
          if (offset >= m_offset && offset + Size(control) <= m_offset + Size())
            return CGUIControlGroup::OnMessage(message);
          break;
        }
        offset += Size(control) + m_itemGap;
      }
      // find the first control on this page
      offset = 0;
      for (iControls it = m_children.begin(); it != m_children.end(); ++it)
      {
        CGUIControl *control = *it;
        if (!control->IsVisible())
          continue;
        if (control->CanFocus() && offset >= m_offset && offset + Size(control) <= m_offset + Size())
        {
          m_focusedControl = control->GetID();
          break;
        }
        offset += Size(control) + m_itemGap;
      }
    }
    break;
  case GUI_MSG_PAGE_CHANGE:
    {
      if (message.GetSenderId() == m_pageControl)
      { // it's from our page control
        ScrollTo((float)message.GetParam1());
        return true;
      }
    }
    break;
  }
  return CGUIControlGroup::OnMessage(message);
}

void CGUIControlGroupList::ValidateOffset()
{
  // calculate how many items we have on this page
  m_totalSize = 0;
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    if (!control->IsVisible()) continue;
    m_totalSize += Size(control) + m_itemGap;
  }
  if (m_totalSize > 0) m_totalSize -= m_itemGap;
  // check our m_offset range
  if (m_offset > m_totalSize - Size())
    m_offset = m_totalSize - Size();
  if (m_offset < 0) m_offset = 0;
}

void CGUIControlGroupList::AddControl(CGUIControl *control, int position /*= -1*/)
{
  // NOTE: We override control navigation here, but we don't override the <onleft> etc. builtins
  //       if specified.
  if (position < 0 || position > (int)m_children.size()) // add at the end
    position = (int)m_children.size();

  if (control)
  { // set the navigation of items so that they form a list
    DWORD beforeID = (m_orientation == VERTICAL) ? GetControlIdUp() : GetControlIdLeft();
    DWORD afterID = (m_orientation == VERTICAL) ? GetControlIdDown() : GetControlIdRight();
    if (m_children.size())
    {
      // we're inserting at the given position, so grab the items above and below and alter
      // their navigation accordingly
      CGUIControl *before = NULL;
      CGUIControl *after = NULL;
      if (position == 0)
      { // inserting at the beginning
        after = m_children[0];
        if (afterID == GetID()) // we're wrapping around bottom->top, so we have to update the last item
          before = m_children[m_children.size() - 1];
        if (beforeID == GetID())   // we're wrapping around top->bottom
          beforeID = m_children[m_children.size() - 1]->GetID();
        afterID = after->GetID();
      }
      else if (position == (int)m_children.size())
      { // inserting at the end
        before = m_children[m_children.size() - 1];
        if (beforeID == GetID())   // we're wrapping around top->bottom, so we have to update the first item
          after = m_children[0];
        if (afterID == GetID()) // we're wrapping around bottom->top
          afterID = m_children[0]->GetID();
        beforeID = before->GetID();
      }
      else
      { // inserting somewhere in the middle
        before = m_children[position - 1];
        after = m_children[position];
        beforeID = before->GetID();
        afterID = after->GetID();
      }
      if (m_orientation == VERTICAL)
      {
        if (before) // update the DOWN action to point to us
          before->SetNavigation(before->GetControlIdUp(), control->GetID(), GetControlIdLeft(), GetControlIdRight());
        if (after) // update the UP action to point to us
          after->SetNavigation(control->GetID(), after->GetControlIdDown(), GetControlIdLeft(), GetControlIdRight());
      }
      else
      {
        if (before) // update the RIGHT action to point to us
          before->SetNavigation(GetControlIdUp(), GetControlIdDown(), before->GetControlIdLeft(), control->GetID());
        if (after) // update the LEFT action to point to us
          after->SetNavigation(GetControlIdUp(), GetControlIdDown(), control->GetID(), after->GetControlIdRight());
      }
    }
    // now the control's nav
    if (m_orientation == VERTICAL)
      control->SetNavigation(beforeID, afterID, GetControlIdLeft(), GetControlIdRight());
    else
      control->SetNavigation(GetControlIdUp(), GetControlIdDown(), beforeID, afterID);

    if (!m_useControlPositions)
      control->SetPosition(0,0);
    CGUIControlGroup::AddControl(control, position);
  }
}

void CGUIControlGroupList::ClearAll()
{
  CGUIControlGroup::ClearAll();
  m_offset = 0;
}

inline float CGUIControlGroupList::Size(const CGUIControl *control) const
{
  return (m_orientation == VERTICAL) ? control->GetYPosition() + control->GetHeight() : control->GetXPosition() + control->GetWidth();
}

inline float CGUIControlGroupList::Size() const
{
  return (m_orientation == VERTICAL) ? m_height : m_width;
}

void CGUIControlGroupList::ScrollTo(float offset)
{
  m_scrollOffset = offset;
  m_scrollSpeed = (m_scrollOffset - m_offset) / TIME_TO_SCROLL;
}

bool CGUIControlGroupList::CanFocusFromPoint(const CPoint &point, CGUIControl **control, CPoint &controlPoint) const
{
  if (!CGUIControl::CanFocus()) return false;
  float pos = 0;
  CPoint controlCoords(point);
  m_transform.InverseTransformPosition(controlCoords.x, controlCoords.y);
  for (ciControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    const CGUIControl *child = *it;
    if (child->IsVisible())
    {
      if (pos + Size(child) > m_offset && pos < m_offset + Size())
      { // we're on screen
        float offsetX = m_orientation == VERTICAL ? m_posX : m_posX + pos - m_offset;
        float offsetY = m_orientation == VERTICAL ? m_posY + pos - m_offset : m_posY;
        if (child->CanFocusFromPoint(controlCoords - CPoint(offsetX, offsetY), control, controlPoint))
          return true;
      }
      pos += Size(child) + m_itemGap;
    }
  }
  *control = NULL;
  return false;
}

void CGUIControlGroupList::UnfocusFromPoint(const CPoint &point)
{
  float pos = 0;
  CPoint controlCoords(point);
  m_transform.InverseTransformPosition(controlCoords.x, controlCoords.y);
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *child = *it;
    if (child->IsVisible())
    {
      if (pos + Size(child) > m_offset && pos < m_offset + Size())
      { // we're on screen
        CPoint offset = (m_orientation == VERTICAL) ? CPoint(m_posX, m_posY + pos - m_offset) : CPoint(m_posX + pos - m_offset, m_posY);
        child->UnfocusFromPoint(controlCoords - offset);
      }
      pos += Size(child) + m_itemGap;
    }
  }
  CGUIControl::UnfocusFromPoint(point);
}

bool CGUIControlGroupList::GetCondition(int condition, int data) const
{
  switch (condition)
  {
  case CONTAINER_HAS_NEXT:
    return (m_totalSize >= Size() && m_offset < m_totalSize - Size());
  case CONTAINER_HAS_PREVIOUS:
    return (m_offset > 0);
  default:
    return false;
  }
}


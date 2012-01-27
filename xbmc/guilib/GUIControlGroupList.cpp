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

#include "GUIControlGroupList.h"
#include "Key.h"
#include "GUIInfoManager.h"
#include "GUIControlProfiler.h"
#include "GUIFont.h" // for XBFONT_* definitions

CGUIControlGroupList::CGUIControlGroupList(int parentID, int controlID, float posX, float posY, float width, float height, float itemGap, int pageControl, ORIENTATION orientation, bool useControlPositions, uint32_t alignment, const CScroller& scroller)
: CGUIControlGroup(parentID, controlID, posX, posY, width, height)
, m_scroller(scroller)
{
  m_itemGap = itemGap;
  m_pageControl = pageControl;
  m_totalSize = 0;
  m_orientation = orientation;
  m_alignment = alignment;
  m_useControlPositions = useControlPositions;
  ControlType = GUICONTROL_GROUPLIST;
  m_minSize = 0;
}

CGUIControlGroupList::~CGUIControlGroupList(void)
{
}

void CGUIControlGroupList::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_scroller.Update(currentTime))
    MarkDirtyRegion();

  // first we update visibility of all our items, to ensure our size and
  // alignment computations are correct.
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    GUIPROFILER_VISIBILITY_BEGIN(control);
    control->UpdateVisibility();
    GUIPROFILER_VISIBILITY_END(control);
  }

  ValidateOffset();
  if (m_pageControl)
  {
    CGUIMessage message(GUI_MSG_LABEL_RESET, GetParentID(), m_pageControl, (int)m_height, (int)m_totalSize);
    SendWindowMessage(message);
    CGUIMessage message2(GUI_MSG_ITEM_SELECT, GetParentID(), m_pageControl, (int)m_scroller.GetValue());
    SendWindowMessage(message2);
  }
  // we run through the controls, rendering as we go
  float pos = GetAlignOffset();
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    // note we render all controls, even if they're offscreen, as then they'll be updated
    // with respect to animations
    CGUIControl *control = *it;
    if (m_orientation == VERTICAL)
      g_graphicsContext.SetOrigin(m_posX, m_posY + pos - m_scroller.GetValue());
    else
      g_graphicsContext.SetOrigin(m_posX + pos - m_scroller.GetValue(), m_posY);
    control->DoProcess(currentTime, dirtyregions);

    if (control->IsVisible())
      pos += Size(control) + m_itemGap;
    g_graphicsContext.RestoreOrigin();
  }
  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIControlGroupList::Render()
{
  // we run through the controls, rendering as we go
  bool render(g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height));
  float pos = GetAlignOffset();
  float focusedPos = 0;
  CGUIControl *focusedControl = NULL;
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    // note we render all controls, even if they're offscreen, as then they'll be updated
    // with respect to animations
    CGUIControl *control = *it;
    if (m_renderFocusedLast && control->HasFocus())
    {
      focusedControl = control;
      focusedPos = pos;
    }
    else
    {
      if (m_orientation == VERTICAL)
        g_graphicsContext.SetOrigin(m_posX, m_posY + pos - m_scroller.GetValue());
      else
        g_graphicsContext.SetOrigin(m_posX + pos - m_scroller.GetValue(), m_posY);
      control->DoRender();
    }
    if (control->IsVisible())
      pos += Size(control) + m_itemGap;
    g_graphicsContext.RestoreOrigin();
  }
  if (focusedControl)
  {
    if (m_orientation == VERTICAL)
      g_graphicsContext.SetOrigin(m_posX, m_posY + focusedPos - m_scroller.GetValue());
    else
      g_graphicsContext.SetOrigin(m_posX + focusedPos - m_scroller.GetValue(), m_posY);
    focusedControl->DoRender();
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
          // find out whether this is the first or last control
          if (IsFirstFocusableControl(control))
            ScrollTo(0);
          else if (IsLastFocusableControl(control))
            ScrollTo(m_totalSize - Size());
          else if (offset < m_scroller.GetValue())
            ScrollTo(offset);
          else if (offset + Size(control) > m_scroller.GetValue() + Size())
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
          if (offset >= m_scroller.GetValue() && offset + Size(control) <= m_scroller.GetValue() + Size())
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
        if (control->CanFocus() && offset >= m_scroller.GetValue() && offset + Size(control) <= m_scroller.GetValue() + Size())
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
  m_totalSize = GetTotalSize();
  // check our m_offset range
  if (m_scroller.GetValue() > m_totalSize - Size())
    m_scroller.SetValue(m_totalSize - Size());
  if (m_scroller.GetValue() < 0) m_scroller.SetValue(0);
}

void CGUIControlGroupList::AddControl(CGUIControl *control, int position /*= -1*/)
{
  // NOTE: We override control navigation here, but we don't override the <onleft> etc. builtins
  //       if specified.
  if (position < 0 || position > (int)m_children.size()) // add at the end
    position = (int)m_children.size();

  if (control)
  { // set the navigation of items so that they form a list
    int beforeID = (m_orientation == VERTICAL) ? GetControlIdUp() : GetControlIdLeft();
    int afterID = (m_orientation == VERTICAL) ? GetControlIdDown() : GetControlIdRight();
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
          before->SetNavigation(before->GetControlIdUp(), control->GetID(), GetControlIdLeft(), GetControlIdRight(), GetControlIdBack());
        if (after) // update the UP action to point to us
          after->SetNavigation(control->GetID(), after->GetControlIdDown(), GetControlIdLeft(), GetControlIdRight(), GetControlIdBack());
      }
      else
      {
        if (before) // update the RIGHT action to point to us
          before->SetNavigation(GetControlIdUp(), GetControlIdDown(), before->GetControlIdLeft(), control->GetID(), GetControlIdBack());
        if (after) // update the LEFT action to point to us
          after->SetNavigation(GetControlIdUp(), GetControlIdDown(), control->GetID(), after->GetControlIdRight(), GetControlIdBack());
      }
    }
    // now the control's nav
    CGUIAction empty;
    if (m_orientation == VERTICAL)
    {
      control->SetNavigation(beforeID, afterID, GetControlIdLeft(), GetControlIdRight(), GetControlIdBack());
      control->SetNavigationActions(empty, empty, m_actionLeft, m_actionRight, empty, false);
    }
    else
    {
      control->SetNavigation(GetControlIdUp(), GetControlIdDown(), beforeID, afterID, GetControlIdBack());
      control->SetNavigationActions(m_actionUp, m_actionDown, empty, empty, empty, false);
    }

    if (!m_useControlPositions)
      control->SetPosition(0,0);
    CGUIControlGroup::AddControl(control, position);
    m_totalSize = GetTotalSize();
  }
}

void CGUIControlGroupList::ClearAll()
{
  m_totalSize = 0;
  CGUIControlGroup::ClearAll();
  m_scroller.SetValue(0);
}

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

float CGUIControlGroupList::GetWidth() const
{
  if (m_orientation == HORIZONTAL)
    return CLAMP(m_totalSize, m_minSize, m_width);
  return CGUIControlGroup::GetWidth();
}

float CGUIControlGroupList::GetHeight() const
{
  if (m_orientation == VERTICAL)
    return CLAMP(m_totalSize, m_minSize, m_height);
  return CGUIControlGroup::GetHeight();
}

void CGUIControlGroupList::SetMinSize(float minWidth, float minHeight)
{
  if (m_orientation == VERTICAL)
    m_minSize = minHeight;
  else
    m_minSize = minWidth;
}

float CGUIControlGroupList::Size(const CGUIControl *control) const
{
  return (m_orientation == VERTICAL) ? control->GetYPosition() + control->GetHeight() : control->GetXPosition() + control->GetWidth();
}

inline float CGUIControlGroupList::Size() const
{
  return (m_orientation == VERTICAL) ? m_height : m_width;
}

void CGUIControlGroupList::ScrollTo(float offset)
{
  m_scroller.ScrollTo(offset);
  if (m_scroller.IsScrolling())
    SetInvalid();
}

EVENT_RESULT CGUIControlGroupList::SendMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  // transform our position into child coordinates
  CPoint childPoint(point);
  m_transform.InverseTransformPosition(childPoint.x, childPoint.y);
  if (CGUIControl::CanFocus())
  {
    float pos = 0;
    float alignOffset = GetAlignOffset();
    for (ciControls i = m_children.begin(); i != m_children.end(); ++i)
    {
      CGUIControl *child = *i;
      if (child->IsVisible())
      {
        if (pos + Size(child) > m_scroller.GetValue() && pos < m_scroller.GetValue() + Size())
        { // we're on screen
          float offsetX = m_orientation == VERTICAL ? m_posX : m_posX + alignOffset + pos - m_scroller.GetValue();
          float offsetY = m_orientation == VERTICAL ? m_posY + alignOffset + pos - m_scroller.GetValue() : m_posY;
          EVENT_RESULT ret = child->SendMouseEvent(childPoint - CPoint(offsetX, offsetY), event);
          if (ret)
          { // we've handled the action, and/or have focused an item
            return ret;
          }
        }
        pos += Size(child) + m_itemGap;
      }
    }
    // none of our children want the event, but we may want it.
    EVENT_RESULT ret;
    if (HitTest(childPoint) && (ret = OnMouseEvent(childPoint, event)))
      return ret;
  }
  m_focusedControl = 0;
  return EVENT_RESULT_UNHANDLED;
}

void CGUIControlGroupList::UnfocusFromPoint(const CPoint &point)
{
  float pos = 0;
  CPoint controlCoords(point);
  m_transform.InverseTransformPosition(controlCoords.x, controlCoords.y);
  float alignOffset = GetAlignOffset();
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *child = *it;
    if (child->IsVisible())
    {
      if (pos + Size(child) > m_scroller.GetValue() && pos < m_scroller.GetValue() + Size())
      { // we're on screen
        CPoint offset = (m_orientation == VERTICAL) ? CPoint(m_posX, m_posY + alignOffset + pos - m_scroller.GetValue()) : CPoint(m_posX + alignOffset + pos - m_scroller.GetValue(), m_posY);
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
    return (m_totalSize >= Size() && m_scroller.GetValue() < m_totalSize - Size());
  case CONTAINER_HAS_PREVIOUS:
    return (m_scroller.GetValue() > 0);
  default:
    return false;
  }
}

bool CGUIControlGroupList::IsFirstFocusableControl(const CGUIControl *control) const
{
  for (ciControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *child = *it;
    if (child->IsVisible() && child->CanFocus())
    { // found first focusable
      return child == control;
    }
  }
  return false;
}

bool CGUIControlGroupList::IsLastFocusableControl(const CGUIControl *control) const
{
  for (crControls it = m_children.rbegin(); it != m_children.rend(); ++it)
  {
    CGUIControl *child = *it;
    if (child->IsVisible() && child->CanFocus())
    { // found first focusable
      return child == control;
    }
  }
  return false;
}

float CGUIControlGroupList::GetAlignOffset() const
{
  if (m_totalSize < Size())
  {
    if (m_alignment & XBFONT_RIGHT)
      return Size() - m_totalSize;
    if (m_alignment & XBFONT_CENTER_X)
      return (Size() - m_totalSize)*0.5f;
  }
  return 0.0f;
}

EVENT_RESULT CGUIControlGroupList::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_WHEEL_UP || event.m_id == ACTION_MOUSE_WHEEL_DOWN)
  {
    // find the current control and move to the next or previous
    float offset = 0;
    for (ciControls it = m_children.begin(); it != m_children.end(); ++it)
    {
      CGUIControl *control = *it;
      if (!control->IsVisible()) continue;
      float nextOffset = offset + Size(control) + m_itemGap;
      if (event.m_id == ACTION_MOUSE_WHEEL_DOWN && nextOffset > m_scroller.GetValue() && m_scroller.GetValue() < m_totalSize - Size()) // past our current offset
      {
        ScrollTo(nextOffset);
        return EVENT_RESULT_HANDLED;
      }
      else if (event.m_id == ACTION_MOUSE_WHEEL_UP && nextOffset >= m_scroller.GetValue() && m_scroller.GetValue() > 0) // at least at our current offset
      {
        ScrollTo(offset);
        return EVENT_RESULT_HANDLED;
      }
      offset = nextOffset;
    }
  }
  return EVENT_RESULT_UNHANDLED;
}

float CGUIControlGroupList::GetTotalSize() const
{
  float totalSize = 0;
  for (ciControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    if (!control->IsVisible()) continue;
    totalSize += Size(control) + m_itemGap;
  }
  if (totalSize > 0) totalSize -= m_itemGap;
  return totalSize;
}

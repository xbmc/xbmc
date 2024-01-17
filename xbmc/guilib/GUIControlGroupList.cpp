/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIControlGroupList.h"

#include "GUIAction.h"
#include "GUIControlProfiler.h"
#include "GUIFont.h" // for XBFONT_* definitions
#include "GUIMessage.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/mouse/MouseEvent.h"
#include "utils/StringUtils.h"

using namespace KODI;

CGUIControlGroupList::CGUIControlGroupList(int parentID, int controlID, float posX, float posY, float width, float height, float itemGap, int pageControl, ORIENTATION orientation, bool useControlPositions, uint32_t alignment, const CScroller& scroller)
: CGUIControlGroup(parentID, controlID, posX, posY, width, height)
, m_scroller(scroller)
{
  m_itemGap = itemGap;
  m_pageControl = pageControl;
  m_focusedPosition = 0;
  m_totalSize = 0;
  m_orientation = orientation;
  m_alignment = alignment;
  m_lastScrollerValue = -1;
  m_useControlPositions = useControlPositions;
  ControlType = GUICONTROL_GROUPLIST;
  m_minSize = 0;
}

CGUIControlGroupList::~CGUIControlGroupList(void) = default;

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
    control->UpdateVisibility(nullptr);
    GUIPROFILER_VISIBILITY_END(control);
  }

  // visibility status of some of the list items may have changed. Thus, the group list size
  // may now be different and the scroller needs to be updated
  int previousTotalSize = m_totalSize;
  ValidateOffset(); // m_totalSize is updated here
  bool sizeChanged = previousTotalSize != m_totalSize;

  if (m_pageControl && (m_lastScrollerValue != m_scroller.GetValue() || sizeChanged))
  {
    CGUIMessage message(GUI_MSG_LABEL_RESET, GetParentID(), m_pageControl, (int)Size(), (int)m_totalSize);
    SendWindowMessage(message);
    CGUIMessage message2(GUI_MSG_ITEM_SELECT, GetParentID(), m_pageControl, (int)m_scroller.GetValue());
    SendWindowMessage(message2);
    m_lastScrollerValue = static_cast<int>(m_scroller.GetValue());
  }
  // we run through the controls, rendering as we go
  int index = 0;
  float pos = GetAlignOffset();
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    // note we render all controls, even if they're offscreen, as then they'll be updated
    // with respect to animations
    CGUIControl *control = *it;
    if (m_orientation == VERTICAL)
      CServiceBroker::GetWinSystem()->GetGfxContext().SetOrigin(m_posX, m_posY + pos - m_scroller.GetValue());
    else
      CServiceBroker::GetWinSystem()->GetGfxContext().SetOrigin(m_posX + pos - m_scroller.GetValue(), m_posY);
    control->DoProcess(currentTime, dirtyregions);

    if (control->IsVisible())
    {
      if (IsControlOnScreen(pos, control))
      {
        if (control->HasFocus())
          m_focusedPosition = index;
        index++;
      }

      pos += Size(control) + m_itemGap;
    }
    CServiceBroker::GetWinSystem()->GetGfxContext().RestoreOrigin();
  }
  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIControlGroupList::Render()
{
  // we run through the controls, rendering as we go
  bool render(CServiceBroker::GetWinSystem()->GetGfxContext().SetClipRegion(m_posX, m_posY, m_width, m_height));
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
        CServiceBroker::GetWinSystem()->GetGfxContext().SetOrigin(m_posX, m_posY + pos - m_scroller.GetValue());
      else
        CServiceBroker::GetWinSystem()->GetGfxContext().SetOrigin(m_posX + pos - m_scroller.GetValue(), m_posY);
      control->DoRender();
    }
    if (control->IsVisible())
      pos += Size(control) + m_itemGap;
    CServiceBroker::GetWinSystem()->GetGfxContext().RestoreOrigin();
  }
  if (focusedControl)
  {
    if (m_orientation == VERTICAL)
      CServiceBroker::GetWinSystem()->GetGfxContext().SetOrigin(m_posX, m_posY + focusedPos - m_scroller.GetValue());
    else
      CServiceBroker::GetWinSystem()->GetGfxContext().SetOrigin(m_posX + focusedPos - m_scroller.GetValue(), m_posY);
    focusedControl->DoRender();
  }
  if (render) CServiceBroker::GetWinSystem()->GetGfxContext().RestoreClipRegion();
  CGUIControl::Render();
}

bool CGUIControlGroupList::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    case ACTION_PAGE_UP:
      ScrollPages(-1.f);
      return true;

    case ACTION_PAGE_DOWN:
      ScrollPages(1.f);
      return true;

    case ACTION_FIRST_PAGE:
      MoveTo(GetFirstFocusableControl(), 0.f);
      return true;

    case ACTION_LAST_PAGE:
      MoveTo(GetLastFocusableControl(), m_totalSize - Size());
      return true;
  }
  return CGUIControlGroup::OnAction(action);
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
        if (control->GetControl(message.GetControlId()))
        {
          // find out whether this is the first or last control
          if (control == GetFirstFocusableControl())
            ScrollTo(0);
          else if (control == GetLastFocusableControl())
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
        if (control->GetControl(m_focusedControl))
        {
          if (IsControlOnScreen(offset, control))
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
        if (control->CanFocus() && IsControlOnScreen(offset, control))
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
  // calculate item gap. this needs to be done
  // before fetching the total size
  CalculateItemGap();
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
    CGUIAction beforeAction = GetAction((m_orientation == VERTICAL) ? ACTION_MOVE_UP : ACTION_MOVE_LEFT);
    CGUIAction afterAction = GetAction((m_orientation == VERTICAL) ? ACTION_MOVE_DOWN : ACTION_MOVE_RIGHT);
    if (m_children.size())
    {
      // we're inserting at the given position, so grab the items above and below and alter
      // their navigation accordingly
      CGUIControl *before = NULL;
      CGUIControl *after = NULL;
      if (position == 0)
      { // inserting at the beginning
        after = m_children[0];
        if (!afterAction.HasActionsMeetingCondition() || afterAction.GetNavigation() == GetID()) // we're wrapping around bottom->top, so we have to update the last item
          before = m_children[m_children.size() - 1];
        if (!beforeAction.HasActionsMeetingCondition() || beforeAction.GetNavigation() == GetID())   // we're wrapping around top->bottom
          beforeAction = CGUIAction(m_children[m_children.size() - 1]->GetID());
        afterAction = CGUIAction(after->GetID());
      }
      else if (position == (int)m_children.size())
      { // inserting at the end
        before = m_children[m_children.size() - 1];
        if (!beforeAction.HasActionsMeetingCondition() || beforeAction.GetNavigation() == GetID())   // we're wrapping around top->bottom, so we have to update the first item
          after = m_children[0];
        if (!afterAction.HasActionsMeetingCondition() || afterAction.GetNavigation() == GetID()) // we're wrapping around bottom->top
          afterAction = CGUIAction(m_children[0]->GetID());
        beforeAction = CGUIAction(before->GetID());
      }
      else
      { // inserting somewhere in the middle
        before = m_children[position - 1];
        after = m_children[position];
        beforeAction = CGUIAction(before->GetID());
        afterAction = CGUIAction(after->GetID());
      }
      if (m_orientation == VERTICAL)
      {
        if (before) // update the DOWN action to point to us
          before->SetAction(ACTION_MOVE_DOWN, CGUIAction(control->GetID()));
        if (after) // update the UP action to point to us
          after->SetAction(ACTION_MOVE_UP, CGUIAction(control->GetID()));
      }
      else
      {
        if (before) // update the RIGHT action to point to us
          before->SetAction(ACTION_MOVE_RIGHT, CGUIAction(control->GetID()));
        if (after) // update the LEFT action to point to us
          after->SetAction(ACTION_MOVE_LEFT, CGUIAction(control->GetID()));
      }
    }
    // now the control's nav
    // set navigation path on orientation axis
    // and try to apply other nav actions from grouplist
    // don't override them if child have already defined actions
    if (m_orientation == VERTICAL)
    {
      control->SetAction(ACTION_MOVE_UP, beforeAction);
      control->SetAction(ACTION_MOVE_DOWN, afterAction);
      control->SetAction(ACTION_MOVE_LEFT, GetAction(ACTION_MOVE_LEFT), false);
      control->SetAction(ACTION_MOVE_RIGHT, GetAction(ACTION_MOVE_RIGHT), false);
    }
    else
    {
      control->SetAction(ACTION_MOVE_LEFT, beforeAction);
      control->SetAction(ACTION_MOVE_RIGHT, afterAction);
      control->SetAction(ACTION_MOVE_UP, GetAction(ACTION_MOVE_UP), false);
      control->SetAction(ACTION_MOVE_DOWN, GetAction(ACTION_MOVE_DOWN), false);
    }
    control->SetAction(ACTION_NAV_BACK, GetAction(ACTION_NAV_BACK), false);

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

void CGUIControlGroupList::SetInvalid()
{
  CGUIControl::SetInvalid();
  // Force a message to the scrollbar
  m_lastScrollerValue = -1;
}

void CGUIControlGroupList::ScrollTo(float offset)
{
  m_scroller.ScrollTo(offset);
  if (m_scroller.IsScrolling())
    SetInvalid();
  MarkDirtyRegion();
}

EVENT_RESULT CGUIControlGroupList::SendMouseEvent(const CPoint& point,
                                                  const MOUSE::CMouseEvent& event)
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
        if (IsControlOnScreen(pos, child))
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
      if (IsControlOnScreen(pos, child))
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
  case CONTAINER_POSITION:
    return (m_focusedPosition == data);
  default:
    return false;
  }
}

std::string CGUIControlGroupList::GetLabel(int info) const
{
  switch (info)
  {
  case CONTAINER_CURRENT_ITEM:
    return std::to_string(GetSelectedItem());
  case CONTAINER_NUM_ITEMS:
    return std::to_string(GetNumItems());
  case CONTAINER_POSITION:
    return std::to_string(m_focusedPosition);
  default:
    break;
  }
  return "";
}

int CGUIControlGroupList::GetNumItems() const
{
  return std::count_if(m_children.begin(), m_children.end(), [&](const CGUIControl *child) {
    return (child->IsVisible() && child->CanFocus());
  });
}

int CGUIControlGroupList::GetSelectedItem() const
{
  int index = 1;
  for (const auto& child : m_children)
  {
    if (child->IsVisible() && child->CanFocus())
    {
      if (child->HasFocus())
        return index;
      index++;
    }
  }
  return -1;
}

bool CGUIControlGroupList::IsControlOnScreen(float pos, const CGUIControl *control) const
{
  return (pos >= m_scroller.GetValue() && pos + Size(control) <= m_scroller.GetValue() + Size());
}

void CGUIControlGroupList::CalculateItemGap()
{
  if (m_alignment & XBFONT_JUSTIFIED)
  {
    int itemsCount = 0;
    float itemsSize = 0;
    for (const auto& child : m_children)
    {
      if (child->IsVisible())
      {
        itemsSize += Size(child);
        itemsCount++;
      }
    }

    if (itemsCount > 0)
      m_itemGap = (Size() - itemsSize) / itemsCount;
  }
}

float CGUIControlGroupList::GetAlignOffset() const
{
  if (m_totalSize < Size())
  {
    if (m_alignment & XBFONT_RIGHT)
      return Size() - m_totalSize;
    if (m_alignment & (XBFONT_CENTER_X | XBFONT_JUSTIFIED))
      return (Size() - m_totalSize)*0.5f;
  }
  return 0.0f;
}

EVENT_RESULT CGUIControlGroupList::OnMouseEvent(const CPoint& point,
                                                const MOUSE::CMouseEvent& event)
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
  else if (event.m_id == ACTION_GESTURE_BEGIN)
  { // grab exclusive access
    CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, GetID(), GetParentID());
    SendWindowMessage(msg);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_GESTURE_END || event.m_id == ACTION_GESTURE_ABORT)
  { // release exclusive access
    CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, 0, GetParentID());
    SendWindowMessage(msg);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_GESTURE_PAN)
  { // do the drag and validate our offset (corrects for end of scroll)
    m_scroller.SetValue(CLAMP(m_scroller.GetValue() - ((m_orientation == HORIZONTAL) ? event.m_offsetX : event.m_offsetY), 0, m_totalSize - Size()));
    SetInvalid();
    return EVENT_RESULT_HANDLED;
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

float CGUIControlGroupList::GetControlOffset(const CGUIControl* control) const
{
  bool found{false};
  float offset{0.f};
  for (CGUIControl* child : m_children)
  {
    if (child->IsVisible())
    {
      if (child == control)
      {
        found = true;
        break;
      }
      offset += Size(child) + m_itemGap;
    }
  }
  return (found ? offset : -1.f);
}

SGUIControlAndOffset CGUIControlGroupList::GetFocusableControlAt(float target, int direction) const
{
  float offset{0.f};
  CGUIControl* lastFocusable{nullptr};
  float lastFocusableOffset{0.f};

  for (CGUIControl* child : m_children)
  {
    if (child->IsVisible())
    {
      if (child->CanFocus())
      {
        // The target is at the beginning or inside a focusable control > perfect match
        if (offset <= target && offset + Size(child) > target)
          return SGUIControlAndOffset{child, offset};
        // The control at the target position was not focusable
        // or the target was before the first focusable control.
        // Since the children are always iterated from first to last and their positions increase,
        // this control is the next best match when moving down (position increasing).
        // When moving up, the next best match is the focusable control before this one.
        else if (offset >= target)
        {
          if (direction > 0.f || !lastFocusable)
            return SGUIControlAndOffset{child, offset};
          else
            return SGUIControlAndOffset{lastFocusable, lastFocusableOffset};
        }
        lastFocusable = child;
        lastFocusableOffset = offset;
      }
      offset += Size(child) + m_itemGap;
    }
  }
  // No visible focusable controls or the target is beyond the last focusable control
  return SGUIControlAndOffset{lastFocusable, lastFocusableOffset};
}

void CGUIControlGroupList::ScrollPages(float pages)
{
  ValidateOffset();

  float currOffset = m_scroller.GetValue();
  float newOffset = currOffset + pages * Size();

  if (newOffset < 0.f)
    newOffset = 0.f;
  else if (newOffset > m_totalSize - Size())
    newOffset = m_totalSize - Size();

  CGUIControl* focusedControl = GetFocusedControl();
  if (focusedControl)
  {
    // Using the middle of the focused control as origin helps deal with the alignment thrown off
    // by controls of different sizes. The expected target control can be missed by a few pixels otherwise.
    float origin = GetControlOffset(focusedControl) + Size(focusedControl) / 2;
    SGUIControlAndOffset newFocusedControl = GetFocusableControlAt(origin + pages * Size(), pages);

    if (newFocusedControl.control)
    {
      CGUIMessage message(GUI_MSG_LOSTFOCUS, GetID(), focusedControl->GetID(),
                          newFocusedControl.control->GetID());
      focusedControl->OnMessage(message);

      CGUIMessage message2(GUI_MSG_SETFOCUS, GetID(), newFocusedControl.control->GetID());
      newFocusedControl.control->OnMessage(message2);

      // Adjust the new view offset so that the new focused control is fully visible
      if (newOffset > newFocusedControl.offset)
        newOffset = newFocusedControl.offset;
      else if (newOffset + Size() < newFocusedControl.offset + Size(newFocusedControl.control))
        newOffset = newFocusedControl.offset + Size(newFocusedControl.control) - Size();
    }
  }
  // The GUI_MSG_SETFOCUS message only makes the selection visible
  // Restore the relative position of the selection in the view
  ScrollTo(newOffset);
}

void CGUIControlGroupList::MoveTo(CGUIControl* control, float offset)
{
  CGUIControl* focusedControl = GetFocusedControl();

  if (focusedControl && control)
  {
    CGUIMessage message(GUI_MSG_LOSTFOCUS, GetID(), focusedControl->GetID(), control->GetID());
    focusedControl->OnMessage(message);

    CGUIMessage message2(GUI_MSG_SETFOCUS, GetID(), control->GetID());
    control->OnMessage(message2);
  }
  ScrollTo(offset);
}

CGUIControl* CGUIControlGroupList::GetFirstFocusableControl() const
{
  for (ciControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl* child = *it;
    if (child->CanFocus())
      return child;
  }
  return nullptr;
}

CGUIControl* CGUIControlGroupList::GetLastFocusableControl() const
{
  for (crControls it = m_children.rbegin(); it != m_children.rend(); ++it)
  {
    CGUIControl* child = *it;
    if (child->CanFocus())
      return child;
  }
  return nullptr;
}

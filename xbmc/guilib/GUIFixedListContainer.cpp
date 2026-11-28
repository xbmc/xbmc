/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIFixedListContainer.h"

#include "GUIListItemLayout.h"
#include "GUIMessage.h"
#include "ServiceBroker.h"
#include "input/InputManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/mouse/MouseEvent.h"

#include <algorithm>
#include <cmath>
#include <functional>

namespace
{
// Speed in rows per second the list scrolls at when the pointer is at its edge
constexpr float HOVER_SCROLL_MAX_SPEED = 10.0f;
// Rows at either end of the list in which the pointer scrolls it
constexpr float HOVER_SCROLL_EDGE = 3.0f;
// A longer frame doesn't scroll any further
constexpr unsigned int HOVER_SCROLL_MAX_FRAME_TIME = 100;
} // namespace

CGUIFixedListContainer::CGUIFixedListContainer(int parentID,
                                               int controlID,
                                               float posX,
                                               float posY,
                                               float width,
                                               float height,
                                               ORIENTATION orientation,
                                               const CScroller& scroller,
                                               int preloadItems,
                                               int fixedPosition,
                                               int startCursorRange,
                                               int endCursorRange,
                                               FixedListAlignY alignY)
  : CGUIBaseContainer(
        parentID, controlID, posX, posY, width, height, orientation, scroller, preloadItems)
{
  ControlType = GUICONTAINER_FIXEDLIST;
  m_type = VIEW_TYPE_LIST;
  m_fixedCursor = fixedPosition;
  m_startCursorRange = startCursorRange;
  m_endCursorRange = endCursorRange;
  m_alignY = alignY;
  SetCursor(m_fixedCursor);
}

CGUIFixedListContainer::~CGUIFixedListContainer(void) = default;

bool CGUIFixedListContainer::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
  case ACTION_PAGE_UP:
    {
      SelectItem(std::max(GetSelectedItem() - GetPageSize(), 0));
      return true;
    }
    break;
  case ACTION_PAGE_DOWN:
    {
      SelectItem(std::min(GetSelectedItem() + GetPageSize(), static_cast<int>(m_items.size()) - 1));
      return true;
    }
    break;
    // smooth scrolling (for analog controls)
  case ACTION_SCROLL_UP:
    {
      m_analogScrollCount += action.GetAmount() * action.GetAmount();
      bool handled = false;
      while (m_analogScrollCount > 0.4f)
      {
        handled = true;
        m_analogScrollCount -= 0.4f;
        Scroll(-1);
      }
      return handled;
    }
    break;
  case ACTION_SCROLL_DOWN:
    {
      m_analogScrollCount += action.GetAmount() * action.GetAmount();
      bool handled = false;
      while (m_analogScrollCount > 0.4f)
      {
        handled = true;
        m_analogScrollCount -= 0.4f;
        Scroll(1);
      }
      return handled;
    }
    break;
  }
  return CGUIBaseContainer::OnAction(action);
}

bool CGUIFixedListContainer::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() && message.GetMessage() == GUI_MSG_PAGE_CHANGE &&
      message.GetSenderId() == m_pageControl && IsVisible())
  {
    const int maxOffset = std::max(static_cast<int>(GetRows()) - GetPageSize(), 0);
    const int pageOffset = std::min(std::max(message.GetParam1(), 0), maxOffset);
    const int currentPageOffset =
        m_lastPageControlOffset.value_or(std::min(std::max(GetOffset(), 0), maxOffset));
    const int targetItem = GetSelectedItem() + pageOffset - currentPageOffset;
    if (pageOffset != currentPageOffset)
      StartPageChangeTimer();
    SelectItem(std::min(std::max(targetItem, 0), static_cast<int>(m_items.size()) - 1));
    m_lastPageControlOffset = pageOffset;
    return true;
  }

  return CGUIBaseContainer::OnMessage(message);
}

void CGUIFixedListContainer::Process(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  if (m_hoverScrolling)
    UpdateHoverScrolling(currentTime);
  CGUIBaseContainer::Process(currentTime, dirtyregions);
}

EVENT_RESULT CGUIFixedListContainer::OnMouseEvent(const CPoint& point,
                                                  const KODI::MOUSE::CMouseEvent& event)
{
  // Only a move of the pointer goes on with the scrolling
  if (event.m_id != ACTION_MOUSE_MOVE)
    m_hoverScrolling = false;
  return CGUIBaseContainer::OnMouseEvent(point, event);
}

bool CGUIFixedListContainer::MoveUp(bool wrapAround)
{
  int item = GetSelectedItem();
  if (item > 0)
    SelectItem(item - 1);
  else if (wrapAround)
  {
    SelectItem((int)m_items.size() - 1);
    SetContainerMoving(-1);
  }
  else
    return false;
  return true;
}

bool CGUIFixedListContainer::MoveDown(bool wrapAround)
{
  int item = GetSelectedItem();
  if (item < (int)m_items.size() - 1)
    SelectItem(item + 1);
  else if (wrapAround)
  { // move first item in list
    SelectItem(0);
    SetContainerMoving(1);
  }
  else
    return false;
  return true;
}

void CGUIFixedListContainer::Scroll(int amount)
{
  // increase or decrease the offset within [-minCursor, m_items.size() - maxCursor]
  int minCursor, maxCursor;
  GetCursorRange(minCursor, maxCursor);
  const int nextCursor = GetCursor() + amount;
  int offset = GetOffset() + amount;
  if (offset < -minCursor)
  {
    offset = -minCursor;
    SetCursor(nextCursor < minCursor ? minCursor : nextCursor);
  }
  if (offset > (int)m_items.size() - 1 - maxCursor)
  {
    offset = m_items.size() - 1 - maxCursor;
    SetCursor(nextCursor > maxCursor ? maxCursor : nextCursor);
  }
  ScrollToOffset(offset);
}

bool CGUIFixedListContainer::GetOffsetRange(int &minOffset, int &maxOffset) const
{
  GetCursorRange(minOffset, maxOffset);
  minOffset = -minOffset;
  maxOffset = m_items.size() - maxOffset - 1;
  return true;
}

void CGUIFixedListContainer::ValidateOffset()
{
  if (!m_layout) return;
  // ensure our fixed cursor position is valid
  if (m_fixedCursor >= m_itemsPerPage)
    m_fixedCursor = m_itemsPerPage - 1;
  if (m_fixedCursor < 0)
    m_fixedCursor = 0;
  // compute our minimum and maximum cursor positions
  int minCursor, maxCursor;
  GetCursorRange(minCursor, maxCursor);
  // assure our cursor is between these limits
  SetCursor(std::max(GetCursor(), minCursor));
  SetCursor(std::min(GetCursor(), maxCursor));
  int minOffset, maxOffset;
  GetOffsetRange(minOffset, maxOffset);
  // and finally ensure our offset is valid
  // don't validate offset if we are scrolling in case the tween image exceed <0, 1> range
  if (GetOffset() > maxOffset || (!m_scroller.IsScrolling() && m_scroller.GetValue() > maxOffset * m_layout->Size(m_orientation)))
  {
    SetOffset(std::max(-minCursor, maxOffset));
    m_scroller.SetValue(GetOffset() * m_layout->Size(m_orientation));
  }
  if (GetOffset() < minOffset || (!m_scroller.IsScrolling() && m_scroller.GetValue() < minOffset * m_layout->Size(m_orientation)))
  {
    SetOffset(minOffset);
    m_scroller.SetValue(GetOffset() * m_layout->Size(m_orientation));
  }
}

int CGUIFixedListContainer::GetCursorFromPoint(const CPoint &point, CPoint *itemPoint) const
{
  if (!m_focusedLayout || !m_layout)
    return -1;
  int minCursor, maxCursor;
  GetCursorRange(minCursor, maxCursor);
  // see if the point is either side of our focus range
  float start = (minCursor + 0.2f) * m_layout->Size(m_orientation);
  float end = (maxCursor - 0.2f) * m_layout->Size(m_orientation) + m_focusedLayout->Size(m_orientation);
  float pos = (m_orientation == VERTICAL) ? point.y : point.x;
  if (pos >= start && pos <= end)
  { // select the appropriate item
    pos -= minCursor * m_layout->Size(m_orientation);
    for (int row = minCursor; row <= maxCursor; row++)
    {
      const CGUIListItemLayout *layout = (row == GetCursor()) ? m_focusedLayout : m_layout;
      if (pos < layout->Size(m_orientation))
      {
        if (!InsideLayout(layout, point))
          return -1;
        return row;
      }
      pos -= layout->Size(m_orientation);
    }
  }
  return -1;
}

float CGUIFixedListContainer::GetHoverScrollSpeed(const CPoint& point) const
{
  if (!m_focusedLayout || !m_layout)
    return 0.0f;

  const float sizeOfItem = m_layout->Size(m_orientation);
  const float edge = HOVER_SCROLL_EDGE * sizeOfItem;
  const float length = (m_orientation == VERTICAL) ? m_height : m_width;
  int minCursor, maxCursor;
  GetCursorRange(minCursor, maxCursor);
  // The cursor can't leave its range, so the list scrolls while the pointer is outside of it.
  // A range that reaches an end of the list leaves no row there to scroll it, so the rows
  // at that end of the container scroll it instead.
  const float start = (minCursor == 0) ? edge : (minCursor + 0.2f) * sizeOfItem;
  const float end = (maxCursor >= m_itemsPerPage - 1)
                        ? length - edge
                        : (maxCursor - 0.2f) * sizeOfItem + m_focusedLayout->Size(m_orientation);
  const float pos = (m_orientation == VERTICAL) ? point.y : point.x;

  int minOffset, maxOffset;
  GetOffsetRange(minOffset, maxOffset);
  float distance; // into the scrolling area, in rows
  float direction;
  if (pos < start && GetOffset() > minOffset)
  {
    distance = (start - pos) / sizeOfItem;
    direction = -1.0f;
  }
  else if (pos > end && GetOffset() < maxOffset)
  {
    distance = (pos - end) / sizeOfItem;
    direction = 1.0f;
  }
  else
    return 0.0f;

  distance = std::min(distance, 1.0f);
  return direction * distance * distance * HOVER_SCROLL_MAX_SPEED;
}

void CGUIFixedListContainer::UpdateHoverScrolling(unsigned int currentTime)
{
  // The pointer is where it was seen last, as long as it is in use and over the list
  const float speed = (HasFocus() && CServiceBroker::GetInputManager().IsMouseActive())
                          ? GetHoverScrollSpeed(m_hoverPoint)
                          : 0.0f;
  if (speed == 0.0f)
  {
    m_hoverScrolling = false;
    return;
  }

  if (m_hoverScrollTime != 0)
  {
    const unsigned int elapsed =
        std::min(currentTime - m_hoverScrollTime, HOVER_SCROLL_MAX_FRAME_TIME);
    m_analogScrollCount += std::abs(speed) * static_cast<float>(elapsed) / 1000.0f;
    for (; m_analogScrollCount >= 1.0f; m_analogScrollCount -= 1.0f)
      ScrollToOffset(GetOffset() + (speed < 0.0f ? -1 : 1));
  }
  m_hoverScrollTime = currentTime;
}

bool CGUIFixedListContainer::SelectItemFromPoint(const CPoint& point)
{
  if (!m_focusedLayout || !m_layout)
    return false;

  MarkDirtyRegion();

  const int cursor = GetCursorFromPoint(point);
  if (GetHoverScrollSpeed(point) != 0.0f)
  { // the list scrolls while the pointer stays here, see Process()
    if (!InsideLayout(m_layout, point))
    {
      m_hoverScrolling = false;
      return false;
    }
    if (!m_hoverScrolling)
    {
      m_analogScrollCount = 0.0f;
      m_hoverScrollTime = 0;
    }
    m_hoverScrolling = true;
    m_hoverPoint = point;
    // A row inside of the cursor range stays selected as the list scrolls under the pointer
    if (cursor >= 0)
      SetCursor(cursor);
    return true;
  }

  m_hoverScrolling = false;
  // Select the appropriate item
  if (cursor < 0)
    return false;
  // Calling SelectItem() here will focus the item and scroll, which isn't really what we're after
  SetCursor(cursor);
  return true;
}

bool CGUIFixedListContainer::SelectItemFromClick(const CPoint& point)
{
  if (!m_focusedLayout || !m_layout)
    return false;

  // Find the row at the point, it can be outside of the cursor range. The row after the last one
  // that fits is only partly visible.
  float pos = (m_orientation == VERTICAL) ? point.y : point.x;
  const CGUIListItemLayout* layout = nullptr;
  int row = 0;
  for (; row <= m_itemsPerPage; ++row)
  {
    layout = (row == GetCursor()) ? m_focusedLayout : m_layout;
    if (pos < layout->Size(m_orientation))
      break;
    pos -= layout->Size(m_orientation);
  }
  const int item = CorrectOffset(GetOffset(), row);
  if (row > m_itemsPerPage || item < 0 || item >= static_cast<int>(m_items.size()) ||
      !InsideLayout(layout, point))
    return false;

  // The cursor has to stay in its range, so an item outside of it is scrolled into it
  int minCursor, maxCursor;
  GetCursorRange(minCursor, maxCursor);
  const int cursor = std::clamp(row, minCursor, maxCursor);
  SetCursor(cursor);
  if (cursor != row)
    ScrollToOffset(item - cursor);
  return true;
}

void CGUIFixedListContainer::SelectItem(int item)
{
  // Check that GetOffset() is valid
  ValidateOffset();
  // only select an item if it's in a valid range
  if (item >= 0 && item < (int)m_items.size())
  {
    m_lastPageControlOffset.reset();

    // Select the item requested - we first set the cursor position
    // which may be different at either end of the list, then the offset
    int minCursor, maxCursor;
    GetCursorRange(minCursor, maxCursor);

    int cursor;
    if ((int)m_items.size() - 1 - item <= maxCursor - m_fixedCursor)
      cursor = std::max(m_fixedCursor, maxCursor + item - (int)m_items.size() + 1);
    else if (item <= m_fixedCursor - minCursor)
      cursor = std::min(m_fixedCursor, minCursor + item);
    else
      cursor = m_fixedCursor;
    if (cursor != GetCursor())
      SetContainerMoving(cursor - GetCursor());
    SetCursor(cursor);
    ScrollToOffset(item - GetCursor());
    MarkDirtyRegion();
  }
}

void CGUIFixedListContainer::UpdatePageControl(int offset)
{
  const int maxOffset = std::max(static_cast<int>(GetRows()) - GetPageSize(), 0);
  const int pageOffset = std::min(std::max(offset, 0), maxOffset);
  if (m_pageControl && m_lastPageControlOffset != pageOffset)
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, pageOffset);
    SendWindowMessage(msg);
    m_lastPageControlOffset = pageOffset;
  }
}

bool CGUIFixedListContainer::HasPreviousPage() const
{
  return (GetOffset() > 0);
}

bool CGUIFixedListContainer::HasNextPage() const
{
  return (GetOffset() < static_cast<int>(m_items.size()) - GetPageSize() &&
          static_cast<int>(m_items.size()) >= GetPageSize());
}

int CGUIFixedListContainer::GetCurrentPage() const
{
  const int pageSize = GetPageSize();
  int offset = CorrectOffset(GetOffset(), GetCursor());
  if (offset + pageSize - GetCursor() >= static_cast<int>(GetRows())) // last page
    return (GetRows() + pageSize - 1) / pageSize;
  return offset / pageSize + 1;
}

void CGUIFixedListContainer::GetCursorRange(int &minCursor, int &maxCursor) const
{
  minCursor = std::max(m_fixedCursor - m_startCursorRange, 0);
  // The last row of the container is only partly visible sometimes, but the cursor can be there
  maxCursor = std::min(m_fixedCursor + m_endCursorRange, std::max(m_itemsPerPage - 1, 0));

  if (m_items.empty())
  {
    minCursor = m_fixedCursor;
    maxCursor = m_fixedCursor;
    return;
  }

  std::function<void(int& minCursor, int& maxCursor, int fixedCursor)> fn;

  switch (m_alignY)
  {
    case FixedListAlignY::CENTER:
      fn = [](int& minCursor, int& maxCursor, int fixedCursor)
      {
        if (maxCursor - fixedCursor > fixedCursor - minCursor)
          maxCursor--;
        else
          minCursor++;
      };
      break;
    case FixedListAlignY::TOP:
      fn = [](int& minCursor, int& maxCursor, int fixedCursor) { maxCursor--; };
      break;
    case FixedListAlignY::BOTTOM:
      fn = [](int& minCursor, int& maxCursor, int fixedCursor) { minCursor++; };
      break;
    default:
      // unknown value: nothing can be done, exit to avoid an infinite loop.
      return;
  }

  while (maxCursor - minCursor > static_cast<int>(m_items.size()) - 1)
    fn(minCursor, maxCursor, m_fixedCursor);
}

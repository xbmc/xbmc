/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "GUIFixedListContainer.h"
#include "input/Key.h"

CGUIFixedListContainer::CGUIFixedListContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems, int fixedPosition, int cursorRange)
    : CGUIBaseContainer(parentID, controlID, posX, posY, width, height, orientation, scroller, preloadItems)
{
  ControlType = GUICONTAINER_FIXEDLIST;
  m_type = VIEW_TYPE_LIST;
  m_fixedCursor = fixedPosition;
  m_cursorRange = std::max(0, cursorRange);
  SetCursor(m_fixedCursor);
}

CGUIFixedListContainer::~CGUIFixedListContainer(void)
{
}

bool CGUIFixedListContainer::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
  case ACTION_PAGE_UP:
    {
      Scroll(-m_itemsPerPage);
      return true;
    }
    break;
  case ACTION_PAGE_DOWN:
    {
      Scroll(m_itemsPerPage);
      return true;
    }
    break;
    // smooth scrolling (for analog controls)
  case ACTION_SCROLL_UP:
    {
      m_analogScrollCount += action.GetAmount() * action.GetAmount();
      bool handled = false;
      while (m_analogScrollCount > 0.4)
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
      while (m_analogScrollCount > 0.4)
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
  int offset = GetOffset() + amount;
  if (offset < -minCursor)
  {
    offset = -minCursor;
    SetCursor(minCursor);
  }
  if (offset > (int)m_items.size() - 1 - maxCursor)
  {
    offset = m_items.size() - 1 - maxCursor;
    SetCursor(maxCursor);
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

bool CGUIFixedListContainer::SelectItemFromPoint(const CPoint &point)
{
  if (!m_focusedLayout || !m_layout)
    return false;

  const float mouse_scroll_speed = 0.25f;
  const float mouse_max_amount = 1.5f;
  float sizeOfItem = m_layout->Size(m_orientation);
  int minCursor, maxCursor;
  GetCursorRange(minCursor, maxCursor);
  // see if the point is either side of our focus range
  float start = (minCursor + 0.2f) * sizeOfItem;
  float end = (maxCursor - 0.2f) * sizeOfItem + m_focusedLayout->Size(m_orientation);
  float pos = (m_orientation == VERTICAL) ? point.y : point.x;
  if (pos < start && GetOffset() > -minCursor)
  { // scroll backward
    if (!InsideLayout(m_layout, point))
      return false;
    float amount = std::min((start - pos) / sizeOfItem, mouse_max_amount);
    m_analogScrollCount += amount * amount * mouse_scroll_speed;
    if (m_analogScrollCount > 1)
    {
      ScrollToOffset(GetOffset() - 1);
      m_analogScrollCount = 0;
    }
    return true;
  }
  else if (pos > end && GetOffset() + maxCursor < (int)m_items.size() - 1)
  {
    if (!InsideLayout(m_layout, point))
      return false;
    // scroll forward
    float amount = std::min((pos - end) / sizeOfItem, mouse_max_amount);
    m_analogScrollCount += amount * amount * mouse_scroll_speed;
    if (m_analogScrollCount > 1)
    {
      ScrollToOffset(GetOffset() + 1);
      m_analogScrollCount = 0;
    }
    return true;
  }
  else
  { // select the appropriate item
    int cursor = GetCursorFromPoint(point);
    if (cursor < 0)
      return false;
    // calling SelectItem() here will focus the item and scroll, which isn't really what we're after
    SetCursor(cursor);
    return true;
  }
}

void CGUIFixedListContainer::SelectItem(int item)
{
  // Check that GetOffset() is valid
  ValidateOffset();
  // only select an item if it's in a valid range
  if (item >= 0 && item < (int)m_items.size())
  {
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
  }
}

bool CGUIFixedListContainer::HasPreviousPage() const
{
  return (GetOffset() > 0);
}

bool CGUIFixedListContainer::HasNextPage() const
{
  return (GetOffset() < (int)m_items.size() - m_itemsPerPage && (int)m_items.size() >= m_itemsPerPage);
}

int CGUIFixedListContainer::GetCurrentPage() const
{
  int offset = CorrectOffset(GetOffset(), GetCursor());
  if (offset + m_itemsPerPage - GetCursor() >= (int)GetRows())  // last page
    return (GetRows() + m_itemsPerPage - 1) / m_itemsPerPage;
  return offset / m_itemsPerPage + 1;
}

void CGUIFixedListContainer::GetCursorRange(int &minCursor, int &maxCursor) const
{
  minCursor = std::max(m_fixedCursor - m_cursorRange, 0);
  maxCursor = std::min(m_fixedCursor + m_cursorRange, m_itemsPerPage);

  if (!m_items.size())
  {
    minCursor = m_fixedCursor;
    maxCursor = m_fixedCursor;
    return;
  }

  while (maxCursor - minCursor > (int)m_items.size() - 1)
  {
    if (maxCursor - m_fixedCursor > m_fixedCursor - minCursor)
      maxCursor--;
    else
      minCursor++;
  }
}


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

#include "GUIPanelContainer.h"
#include "GUIInfoManager.h"
#include "input/Key.h"

#include <cassert>

using namespace std;

CGUIPanelContainer::CGUIPanelContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems)
    : CGUIBaseContainer(parentID, controlID, posX, posY, width, height, orientation, scroller, preloadItems)
{
  ControlType = GUICONTAINER_PANEL;
  m_type = VIEW_TYPE_ICON;
  m_itemsPerRow = 1;
}

CGUIPanelContainer::~CGUIPanelContainer(void)
{
}

void CGUIPanelContainer::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  ValidateOffset();

  if (m_bInvalidated)
    UpdateLayout();

  if (!m_layout || !m_focusedLayout) return;

  UpdateScrollOffset(currentTime);

  int offset = (int)(m_scroller.GetValue() / m_layout->Size(m_orientation));

  int cacheBefore, cacheAfter;
  GetCacheOffsets(cacheBefore, cacheAfter);

  // Free memory not used on screen at the moment, do this first so there's more memory for the new items.
  FreeMemory(CorrectOffset(offset - cacheBefore, 0), CorrectOffset(offset + cacheAfter + m_itemsPerPage + 1, 0));

  CPoint origin = CPoint(m_posX, m_posY) + m_renderOffset;
  float pos = (m_orientation == VERTICAL) ? origin.y : origin.x;
  float end = (m_orientation == VERTICAL) ? m_posY + m_height : m_posX + m_width;
  pos += (offset - cacheBefore) * m_layout->Size(m_orientation) - m_scroller.GetValue();
  end += cacheAfter * m_layout->Size(m_orientation);

  int current = (offset - cacheBefore) * m_itemsPerRow;
  int col = 0;
  while (pos < end && m_items.size())
  {
    if (current >= (int)m_items.size())
      break;
    if (current >= 0)
    {
      CGUIListItemPtr item = m_items[current];
      bool focused = (current == GetOffset() * m_itemsPerRow + GetCursor()) && m_bHasFocus;

      if (m_orientation == VERTICAL)
        ProcessItem(origin.x + col * m_layout->Size(HORIZONTAL), pos, item, focused, currentTime, dirtyregions);
      else
        ProcessItem(pos, origin.y + col * m_layout->Size(VERTICAL), item, focused, currentTime, dirtyregions);
    }
    // increment our position
    if (col < m_itemsPerRow - 1)
      col++;
    else
    {
      pos += m_layout->Size(m_orientation);
      col = 0;
    }
    current++;
  }

  // when we are scrolling up, offset will become lower (integer division, see offset calc)
  // to have same behaviour when scrolling down, we need to set page control to offset+1
  UpdatePageControl(offset + (m_scroller.IsScrollingDown() ? 1 : 0));

  CGUIControl::Process(currentTime, dirtyregions);
}


void CGUIPanelContainer::Render()
{
  if (!m_layout || !m_focusedLayout) return;

  int offset = (int)(m_scroller.GetValue() / m_layout->Size(m_orientation));

  int cacheBefore, cacheAfter;
  GetCacheOffsets(cacheBefore, cacheAfter);

  // Free memory not used on screen at the moment, do this first so there's more memory for the new items.
  FreeMemory(CorrectOffset(offset - cacheBefore, 0), CorrectOffset(offset + cacheAfter + m_itemsPerPage + 1, 0));

  if (g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height))
  {
    CPoint origin = CPoint(m_posX, m_posY) + m_renderOffset;
    float pos = (m_orientation == VERTICAL) ? origin.y : origin.x;
    float end = (m_orientation == VERTICAL) ? m_posY + m_height : m_posX + m_width;
    pos += (offset - cacheBefore) * m_layout->Size(m_orientation) - m_scroller.GetValue();
    end += cacheAfter * m_layout->Size(m_orientation);

    float focusedPos = 0;
    int focusedCol = 0;
    CGUIListItemPtr focusedItem;
    int current = (offset - cacheBefore) * m_itemsPerRow;
    int col = 0;
    while (pos < end && m_items.size())
    {
      if (current >= (int)m_items.size())
        break;
      if (current >= 0)
      {
        CGUIListItemPtr item = m_items[current];
        bool focused = (current == GetOffset() * m_itemsPerRow + GetCursor()) && m_bHasFocus;
        // render our item
        if (focused)
        {
          focusedPos = pos;
          focusedCol = col;
          focusedItem = item;
        }
        else
        {
          if (m_orientation == VERTICAL)
            RenderItem(origin.x + col * m_layout->Size(HORIZONTAL), pos, item.get(), false);
          else
            RenderItem(pos, origin.y + col * m_layout->Size(VERTICAL), item.get(), false);
        }
      }
      // increment our position
      if (col < m_itemsPerRow - 1)
        col++;
      else
      {
        pos += m_layout->Size(m_orientation);
        col = 0;
      }
      current++;
    }
    // and render the focused item last (for overlapping purposes)
    if (focusedItem)
    {
      if (m_orientation == VERTICAL)
        RenderItem(origin.x + focusedCol * m_layout->Size(HORIZONTAL), focusedPos, focusedItem.get(), true);
      else
        RenderItem(focusedPos, origin.y + focusedCol * m_layout->Size(VERTICAL), focusedItem.get(), true);
    }

    g_graphicsContext.RestoreClipRegion();
  }
  CGUIControl::Render();
}

bool CGUIPanelContainer::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
  case ACTION_PAGE_UP:
    {
      if (GetOffset() == 0)
      { // already on the first page, so move to the first item
        SetCursor(0);
      }
      else
      { // scroll up to the previous page
        Scroll( -m_itemsPerPage);
      }
      return true;
    }
    break;
  case ACTION_PAGE_DOWN:
    {
      if ((GetOffset() + m_itemsPerPage) * m_itemsPerRow >= (int)m_items.size() || (int)m_items.size() < m_itemsPerPage)
      { // already at the last page, so move to the last item.
        SetCursor(m_items.size() - GetOffset() * m_itemsPerRow - 1);
      }
      else
      { // scroll down to the next page
        Scroll(m_itemsPerPage);
      }
      return true;
    }
    break;
    // smooth scrolling (for analog controls)
  case ACTION_SCROLL_UP:
    {
      m_analogScrollCount += action.GetAmount() * action.GetAmount();
      bool handled = false;
      while (m_analogScrollCount > AnalogScrollSpeed())
      {
        handled = true;
        m_analogScrollCount -= AnalogScrollSpeed();
        if (GetOffset() > 0)// && GetCursor() <= m_itemsPerPage * m_itemsPerRow / 2)
        {
          Scroll(-1);
        }
        else if (GetCursor() > 0)
        {
          SetCursor(GetCursor() - 1);
        }
      }
      return handled;
    }
    break;
  case ACTION_SCROLL_DOWN:
    {
      m_analogScrollCount += action.GetAmount() * action.GetAmount();
      bool handled = false;
      while (m_analogScrollCount > AnalogScrollSpeed())
      {
        handled = true;
        m_analogScrollCount -= AnalogScrollSpeed();
        if ((GetOffset() + m_itemsPerPage) * m_itemsPerRow < (int)m_items.size())// && GetCursor() >= m_itemsPerPage * m_itemsPerRow / 2)
        {
          Scroll(1);
        }
        else if (GetCursor() < m_itemsPerPage * m_itemsPerRow - 1 && GetOffset() * m_itemsPerRow + GetCursor() < (int)m_items.size() - 1)
        {
          SetCursor(GetCursor() + 1);
        }
      }
      return handled;
    }
    break;
  }
  return CGUIBaseContainer::OnAction(action);
}

bool CGUIPanelContainer::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      SetCursor(0);
      // fall through to base class
    }
  }
  return CGUIBaseContainer::OnMessage(message);
}

void CGUIPanelContainer::OnLeft()
{
  CGUIAction action = GetNavigateAction(ACTION_MOVE_LEFT);
  bool wrapAround = action.GetNavigation() == GetID() || !action.HasActionsMeetingCondition();
  if (m_orientation == VERTICAL && MoveLeft(wrapAround))
    return;
  if (m_orientation == HORIZONTAL && MoveUp(wrapAround))
    return;
  CGUIControl::OnLeft();
}

void CGUIPanelContainer::OnRight()
{
  CGUIAction action = GetNavigateAction(ACTION_MOVE_RIGHT);
  bool wrapAround = action.GetNavigation() == GetID() || !action.HasActionsMeetingCondition();
  if (m_orientation == VERTICAL && MoveRight(wrapAround))
    return;
  if (m_orientation == HORIZONTAL && MoveDown(wrapAround))
    return;
  return CGUIControl::OnRight();
}

void CGUIPanelContainer::OnUp()
{
  CGUIAction action = GetNavigateAction(ACTION_MOVE_UP);
  bool wrapAround = action.GetNavigation() == GetID() || !action.HasActionsMeetingCondition();
  if (m_orientation == VERTICAL && MoveUp(wrapAround))
    return;
  if (m_orientation == HORIZONTAL && MoveLeft(wrapAround))
    return;
  CGUIControl::OnUp();
}

void CGUIPanelContainer::OnDown()
{
  CGUIAction action = GetNavigateAction(ACTION_MOVE_DOWN);
  bool wrapAround = action.GetNavigation() == GetID() || !action.HasActionsMeetingCondition();
  if (m_orientation == VERTICAL && MoveDown(wrapAround))
    return;
  if (m_orientation == HORIZONTAL && MoveRight(wrapAround))
    return;
  return CGUIControl::OnDown();
}

bool CGUIPanelContainer::MoveDown(bool wrapAround)
{
  if (GetCursor() + m_itemsPerRow < m_itemsPerPage * m_itemsPerRow && (GetOffset() + 1 + GetCursor() / m_itemsPerRow) * m_itemsPerRow < (int)m_items.size())
  { // move to last item if necessary
    if ((GetOffset() + 1)*m_itemsPerRow + GetCursor() >= (int)m_items.size())
      SetCursor((int)m_items.size() - 1 - GetOffset()*m_itemsPerRow);
    else
      SetCursor(GetCursor() + m_itemsPerRow);
  }
  else if ((GetOffset() + 1 + GetCursor() / m_itemsPerRow) * m_itemsPerRow < (int)m_items.size())
  { // we scroll to the next row, and move to last item if necessary
    if ((GetOffset() + 1)*m_itemsPerRow + GetCursor() >= (int)m_items.size())
      SetCursor((int)m_items.size() - 1 - (GetOffset() + 1)*m_itemsPerRow);
    ScrollToOffset(GetOffset() + 1);
  }
  else if (wrapAround)
  { // move first item in list
    SetCursor(GetCursor() % m_itemsPerRow);
    ScrollToOffset(0);
    SetContainerMoving(1);
  }
  else
    return false;
  return true;
}

bool CGUIPanelContainer::MoveUp(bool wrapAround)
{
  if (GetCursor() >= m_itemsPerRow)
    SetCursor(GetCursor() - m_itemsPerRow);
  else if (GetOffset() > 0)
    ScrollToOffset(GetOffset() - 1);
  else if (wrapAround)
  { // move last item in list in this column
    SetCursor((GetCursor() % m_itemsPerRow) + (m_itemsPerPage - 1) * m_itemsPerRow);
    int offset = max((int)GetRows() - m_itemsPerPage, 0);
    // should check here whether cursor is actually allowed here, and reduce accordingly
    if (offset * m_itemsPerRow + GetCursor() >= (int)m_items.size())
      SetCursor((int)m_items.size() - offset * m_itemsPerRow - 1);
    ScrollToOffset(offset);
    SetContainerMoving(-1);
  }
  else
    return false;
  return true;
}

bool CGUIPanelContainer::MoveLeft(bool wrapAround)
{
  int col = GetCursor() % m_itemsPerRow;
  if (col > 0)
    SetCursor(GetCursor() - 1);
  else if (wrapAround)
  { // wrap around
    SetCursor(GetCursor() + m_itemsPerRow - 1);
    if (GetOffset() * m_itemsPerRow + GetCursor() >= (int)m_items.size())
      SetCursor((int)m_items.size() - GetOffset() * m_itemsPerRow - 1);
  }
  else
    return false;
  return true;
}

bool CGUIPanelContainer::MoveRight(bool wrapAround)
{
  int col = GetCursor() % m_itemsPerRow;
  if (col + 1 < m_itemsPerRow && GetOffset() * m_itemsPerRow + GetCursor() + 1 < (int)m_items.size())
    SetCursor(GetCursor() + 1);
  else if (wrapAround) // move first item in row
    SetCursor(GetCursor() - col);
  else
    return false;
  return true;
}

// scrolls the said amount
void CGUIPanelContainer::Scroll(int amount)
{
  // increase or decrease the offset
  int offset = GetOffset() + amount;
  if (offset > ((int)GetRows() - m_itemsPerPage) * m_itemsPerRow)
  {
    offset = ((int)GetRows() - m_itemsPerPage) * m_itemsPerRow;
  }
  if (offset < 0) offset = 0;
  ScrollToOffset(offset);
}

void CGUIPanelContainer::ValidateOffset()
{
  if (!m_layout) return;
  // first thing is we check the range of our offset
  // don't validate offset if we are scrolling in case the tween image exceed <0, 1> range
  if (GetOffset() > (int)GetRows() - m_itemsPerPage || (!m_scroller.IsScrolling() && m_scroller.GetValue() > ((int)GetRows() - m_itemsPerPage) * m_layout->Size(m_orientation)))
  {
    SetOffset(std::max(0, (int)GetRows() - m_itemsPerPage));
    m_scroller.SetValue(GetOffset() * m_layout->Size(m_orientation));
  }
  if (GetOffset() < 0 || (!m_scroller.IsScrolling() && m_scroller.GetValue() < 0))
  {
    SetOffset(0);
    m_scroller.SetValue(0);
  }
}

void CGUIPanelContainer::SetCursor(int cursor)
{
  // +1 to ensure we're OK if we have a half item
  if (cursor > (m_itemsPerPage + 1)*m_itemsPerRow - 1) cursor = (m_itemsPerPage + 1)*m_itemsPerRow - 1;
  if (cursor < 0) cursor = 0;
  if (!m_wasReset)
    SetContainerMoving(cursor - GetCursor());
  CGUIBaseContainer::SetCursor(cursor);
}

void CGUIPanelContainer::CalculateLayout()
{
  GetCurrentLayouts();

  if (!m_layout || !m_focusedLayout) return;
  // calculate the number of items to display
  if (m_orientation == HORIZONTAL)
  {
    m_itemsPerRow = (int)(m_height / m_layout->Size(VERTICAL));
    m_itemsPerPage = (int)(m_width / m_layout->Size(HORIZONTAL));
  }
  else
  {
    m_itemsPerRow = (int)(m_width / m_layout->Size(HORIZONTAL));
    m_itemsPerPage = (int)(m_height / m_layout->Size(VERTICAL));
  }
  if (m_itemsPerRow < 1) m_itemsPerRow = 1;
  if (m_itemsPerPage < 1) m_itemsPerPage = 1;

  // ensure that the scroll offset is a multiple of our size
  m_scroller.SetValue(GetOffset() * m_layout->Size(m_orientation));
}

unsigned int CGUIPanelContainer::GetRows() const
{
  assert(m_itemsPerRow > 0);
  return (m_items.size() + m_itemsPerRow - 1) / m_itemsPerRow;
}

float CGUIPanelContainer::AnalogScrollSpeed() const
{
  return 10.0f / m_itemsPerPage;
}

int CGUIPanelContainer::CorrectOffset(int offset, int cursor) const
{
  return offset * m_itemsPerRow + cursor;
}

int CGUIPanelContainer::GetCursorFromPoint(const CPoint &point, CPoint *itemPoint) const
{
  if (!m_layout)
    return -1;

  float sizeX = m_orientation == VERTICAL ? m_layout->Size(HORIZONTAL) : m_layout->Size(VERTICAL);
  float sizeY = m_orientation == VERTICAL ? m_layout->Size(VERTICAL) : m_layout->Size(HORIZONTAL);

  float posY = m_orientation == VERTICAL ? point.y : point.x;
  for (int y = 0; y < m_itemsPerPage + 1; y++) // +1 to ensure if we have a half item we can select it
  {
    float posX = m_orientation == VERTICAL ? point.x : point.y;
    for (int x = 0; x < m_itemsPerRow; x++)
    {
      int item = x + y * m_itemsPerRow;
      if (posX < sizeX && posY < sizeY && item + GetOffset() < (int)m_items.size())
      { // found
        return item;
      }
      posX -= sizeX;
    }
    posY -= sizeY;
  }
  return -1;
}

bool CGUIPanelContainer::SelectItemFromPoint(const CPoint &point)
{
  int cursor = GetCursorFromPoint(point);
  if (cursor < 0)
    return false;
  SetCursor(cursor);
  return true;
}

bool CGUIPanelContainer::GetCondition(int condition, int data) const
{ // probably only works vertically atm...
  int row = GetCursor() / m_itemsPerRow;
  int col = GetCursor() % m_itemsPerRow;
  if (m_orientation == HORIZONTAL)
    swap(row, col);
  switch (condition)
  {
  case CONTAINER_ROW:
    return (row == data);
  case CONTAINER_COLUMN:
    return (col == data);
  default:
    return CGUIBaseContainer::GetCondition(condition, data);
  }
}

void CGUIPanelContainer::SelectItem(int item)
{
  // Check that our offset is valid
  ValidateOffset();
  // only select an item if it's in a valid range
  if (item >= 0 && item < (int)m_items.size())
  {
    // Select the item requested
    if (item >= GetOffset() * m_itemsPerRow && item < (GetOffset() + m_itemsPerPage) * m_itemsPerRow)
    { // the item is on the current page, so don't change it.
      SetCursor(item - GetOffset() * m_itemsPerRow);
    }
    else if (item < GetOffset() * m_itemsPerRow)
    { // item is on a previous page - make it the first item on the page
      SetCursor(item % m_itemsPerRow);
      ScrollToOffset((item - GetCursor()) / m_itemsPerRow);
    }
    else // (item >= GetOffset()+m_itemsPerPage)
    { // item is on a later page - make it the last row on the page
      SetCursor(item % m_itemsPerRow + m_itemsPerRow * (m_itemsPerPage - 1));
      ScrollToOffset((item - GetCursor()) / m_itemsPerRow);
    }
  }
}

bool CGUIPanelContainer::HasPreviousPage() const
{
  return (GetOffset() > 0);
}

bool CGUIPanelContainer::HasNextPage() const
{
  return (GetOffset() != (int)GetRows() - m_itemsPerPage && (int)GetRows() > m_itemsPerPage);
}


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

#include "system.h"
#include "GUIListContainer.h"
#include "GUIListItem.h"
#include "Key.h"
#include "utils/StringUtils.h"

CGUIListContainer::CGUIListContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems)
    : CGUIBaseContainer(parentID, controlID, posX, posY, width, height, orientation, scroller, preloadItems)
{
  ControlType = GUICONTAINER_LIST;
  m_type = VIEW_TYPE_LIST;
}

CGUIListContainer::~CGUIListContainer(void)
{
}

bool CGUIListContainer::OnAction(const CAction &action)
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
      if (GetOffset() == (int)m_items.size() - m_itemsPerPage || (int)m_items.size() < m_itemsPerPage)
      { // already at the last page, so move to the last item.
        SetCursor(m_items.size() - GetOffset() - 1);
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
      while (m_analogScrollCount > 0.4)
      {
        handled = true;
        m_analogScrollCount -= 0.4f;
        if (GetOffset() > 0 && GetCursor() <= m_itemsPerPage / 2)
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
      while (m_analogScrollCount > 0.4)
      {
        handled = true;
        m_analogScrollCount -= 0.4f;
        if (GetOffset() + m_itemsPerPage < (int)m_items.size() && GetCursor() >= m_itemsPerPage / 2)
        {
          Scroll(1);
        }
        else if (GetCursor() < m_itemsPerPage - 1 && GetOffset() + GetCursor() < (int)m_items.size() - 1)
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

bool CGUIListContainer::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      SetCursor(0);
    }
    else if (message.GetMessage() == GUI_MSG_SETFOCUS)
    {
      if (message.GetParam1()) // subfocus item is specified, so set the offset appropriately
      {
        int item = std::min(GetOffset() + (int)message.GetParam1() - 1, (int)m_items.size() - 1);
        SelectItem(item);
      }
    }
  }
  return CGUIBaseContainer::OnMessage(message);
}

bool CGUIListContainer::MoveUp(bool wrapAround)
{
  if (GetCursor() > 0)
  {
    SetCursor(GetCursor() - 1);
  }
  else if (GetCursor() == 0 && GetOffset())
  {
    ScrollToOffset(GetOffset() - 1);
  }
  else if (wrapAround)
  {
    if (m_items.size() > 0)
    { // move 2 last item in list, and set our container moving up
      int offset = m_items.size() - m_itemsPerPage;
      if (offset < 0) offset = 0;
      SetCursor(m_items.size() - offset - 1);
      ScrollToOffset(offset);
      SetContainerMoving(-1);
    }
  }
  else
    return false;
  return true;
}

bool CGUIListContainer::MoveDown(bool wrapAround)
{
  if (GetOffset() + GetCursor() + 1 < (int)m_items.size())
  {
    if (GetCursor() + 1 < m_itemsPerPage)
    {
      SetCursor(GetCursor() + 1);
    }
    else
    {
      ScrollToOffset(GetOffset() + 1);
    }
  }
  else if(wrapAround)
  { // move first item in list, and set our container moving in the "down" direction
    SetCursor(0);
    ScrollToOffset(0);
    SetContainerMoving(1);
  }
  else
    return false;
  return true;
}

// scrolls the said amount
void CGUIListContainer::Scroll(int amount)
{
  // increase or decrease the offset
  int offset = GetOffset() + amount;
  if (offset > (int)m_items.size() - m_itemsPerPage)
  {
    offset = m_items.size() - m_itemsPerPage;
  }
  if (offset < 0) offset = 0;
  ScrollToOffset(offset);
}

void CGUIListContainer::ValidateOffset()
{
  if (!m_layout) return;
  // first thing is we check the range of our offset
  // don't validate offset if we are scrolling in case the tween image exceed <0, 1> range
  int minOffset, maxOffset;
  GetOffsetRange(minOffset, maxOffset);
  if (GetOffset() > maxOffset || (!m_scroller.IsScrolling() && m_scroller.GetValue() > maxOffset * m_layout->Size(m_orientation)))
  {
    SetOffset(std::max(0, maxOffset));
    m_scroller.SetValue(GetOffset() * m_layout->Size(m_orientation));
  }
  if (GetOffset() < 0 || (!m_scroller.IsScrolling() && m_scroller.GetValue() < 0))
  {
    SetOffset(0);
    m_scroller.SetValue(0);
  }
}

void CGUIListContainer::SetCursor(int cursor)
{
  if (cursor > m_itemsPerPage - 1) cursor = m_itemsPerPage - 1;
  if (cursor < 0) cursor = 0;
  if (!m_wasReset)
    SetContainerMoving(cursor - GetCursor());
  CGUIBaseContainer::SetCursor(cursor);
}

void CGUIListContainer::SelectItem(int item)
{
  // Check that our offset is valid
  ValidateOffset();
  // only select an item if it's in a valid range
  if (item >= 0 && item < (int)m_items.size())
  {
    // Select the item requested
    if (item >= GetOffset() && item < GetOffset() + m_itemsPerPage)
    { // the item is on the current page, so don't change it.
      SetCursor(item - GetOffset());
    }
    else if (item < GetOffset())
    { // item is on a previous page - make it the first item on the page
      SetCursor(0);
      ScrollToOffset(item);
    }
    else // (item >= GetOffset()+m_itemsPerPage)
    { // item is on a later page - make it the last item on the page
      SetCursor(m_itemsPerPage - 1);
      ScrollToOffset(item - GetCursor());
    }
  }
}

int CGUIListContainer::GetCursorFromPoint(const CPoint &point, CPoint *itemPoint) const
{
  if (!m_focusedLayout || !m_layout)
    return -1;

  int row = 0;
  float pos = (m_orientation == VERTICAL) ? point.y : point.x;
  while (row < m_itemsPerPage + 1)  // 1 more to ensure we get the (possible) half item at the end.
  {
    const CGUIListItemLayout *layout = (row == GetCursor()) ? m_focusedLayout : m_layout;
    if (pos < layout->Size(m_orientation) && row + GetOffset() < (int)m_items.size())
    { // found correct "row" -> check horizontal
      if (!InsideLayout(layout, point))
        return -1;

      if (itemPoint)
        *itemPoint = m_orientation == VERTICAL ? CPoint(point.x, pos) : CPoint(pos, point.y);
      return row;
    }
    row++;
    pos -= layout->Size(m_orientation);
  }
  return -1;
}

bool CGUIListContainer::SelectItemFromPoint(const CPoint &point)
{
  CPoint itemPoint;
  int row = GetCursorFromPoint(point, &itemPoint);
  if (row < 0)
    return false;

  SetCursor(row);
  CGUIListItemLayout *focusedLayout = GetFocusedLayout();
  if (focusedLayout)
    focusedLayout->SelectItemFromPoint(itemPoint);
  return true;
}

//#ifdef GUILIB_PYTHON_COMPATIBILITY
CGUIListContainer::CGUIListContainer(int parentID, int controlID, float posX, float posY, float width, float height,
                                 const CLabelInfo& labelInfo, const CLabelInfo& labelInfo2,
                                 const CTextureInfo& textureButton, const CTextureInfo& textureButtonFocus,
                                 float textureHeight, float itemWidth, float itemHeight, float spaceBetweenItems)
: CGUIBaseContainer(parentID, controlID, posX, posY, width, height, VERTICAL, 200, 0)
{
  CGUIListItemLayout layout;
  layout.CreateListControlLayouts(width, textureHeight + spaceBetweenItems, false, labelInfo, labelInfo2, textureButton, textureButtonFocus, textureHeight, itemWidth, itemHeight, "", "");
  m_layouts.push_back(layout);
  std::string condition = StringUtils::Format("control.hasfocus(%i)", controlID);
  std::string condition2 = "!" + condition;
  CGUIListItemLayout focusLayout;
  focusLayout.CreateListControlLayouts(width, textureHeight + spaceBetweenItems, true, labelInfo, labelInfo2, textureButton, textureButtonFocus, textureHeight, itemWidth, itemHeight, condition2, condition);
  m_focusedLayouts.push_back(focusLayout);
  m_height = floor(m_height / (textureHeight + spaceBetweenItems)) * (textureHeight + spaceBetweenItems);
  ControlType = GUICONTAINER_LIST;
}
//#endif

bool CGUIListContainer::HasNextPage() const
{
  return (GetOffset() != (int)m_items.size() - m_itemsPerPage && (int)m_items.size() >= m_itemsPerPage);
}

bool CGUIListContainer::HasPreviousPage() const
{
  return (GetOffset() > 0);
}

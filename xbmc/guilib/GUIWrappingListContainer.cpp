/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "GUIWrappingListContainer.h"
#include "FileItem.h"
#include "Key.h"
#include "utils/log.h"

CGUIWrappingListContainer::CGUIWrappingListContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems, int fixedPosition)
    : CGUIBaseContainer(parentID, controlID, posX, posY, width, height, orientation, scroller, preloadItems)
{
  SetCursor(fixedPosition);
  ControlType = GUICONTAINER_WRAPLIST;
  m_type = VIEW_TYPE_LIST;
  m_extraItems = 0;
}

CGUIWrappingListContainer::~CGUIWrappingListContainer(void)
{
}

void CGUIWrappingListContainer::UpdatePageControl(int offset)
{
  if (m_pageControl)
  { // tell our pagecontrol (scrollbar or whatever) to update (offset it by our cursor position)
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, GetNumItems() ? CorrectOffset(offset, GetCursor()) % GetNumItems() : 0);
    SendWindowMessage(msg);
  }
}

bool CGUIWrappingListContainer::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
  case ACTION_PAGE_UP:
    Scroll(-m_itemsPerPage);
    return true;
  case ACTION_PAGE_DOWN:
    Scroll(m_itemsPerPage);
    return true;
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

bool CGUIWrappingListContainer::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_PAGE_CHANGE)
    {
      if (message.GetSenderId() == m_pageControl && IsVisible())
      { // offset by our cursor position
        message.SetParam1(message.GetParam1() - GetCursor());
      }
    }
  }
  return CGUIBaseContainer::OnMessage(message);
}

bool CGUIWrappingListContainer::MoveUp(bool wrapAround)
{
  Scroll(-1);
  return true;
}

bool CGUIWrappingListContainer::MoveDown(bool wrapAround)
{
  Scroll(+1);
  return true;
}

// scrolls the said amount
void CGUIWrappingListContainer::Scroll(int amount)
{
  ScrollToOffset(GetOffset() + amount);
}

bool CGUIWrappingListContainer::GetOffsetRange(int &minOffset, int &maxOffset) const
{
  return false;
}

void CGUIWrappingListContainer::ValidateOffset()
{
  // our minimal amount of items - we need to take into acount extra items to display wrapped items when scrolling
  int minItems = m_itemsPerPage + ScrollCorrectionRange() + GetCacheCount() / 2;
  if (minItems <= m_items.Size())
    return;

  // no need to check the range here, but we need to check we have
  // more items than slots.
  ResetExtraItems();
  if (m_items.Size())
  {
    int numItems = m_items.Size();
    while (m_items.Size() < minItems)
    {
      // add additional copies of items, as we require extras at render time
      for (int i = 0; i < numItems; i++)
      {
        m_items.Add(CFileItemPtr(new CFileItem(*(m_items[i]))));
        m_extraItems++;
      }
    }
  }
}

int CGUIWrappingListContainer::CorrectOffset(int offset, int cursor) const
{
  if (m_items.Size())
  {
    int correctOffset = (offset + cursor) % (int)m_items.Size();
    if (correctOffset < 0) correctOffset += m_items.Size();
    return correctOffset;
  }
  return 0;
}

int CGUIWrappingListContainer::GetSelectedItem() const
{
  if (m_items.Size() > (int)m_extraItems)
  {
    int numItems = (int)(m_items.Size() - m_extraItems);
    int correctOffset = (GetOffset() + GetCursor()) % numItems;
    if (correctOffset < 0) correctOffset += numItems;
    return correctOffset;
  }
  return 0;
}

bool CGUIWrappingListContainer::SelectItemFromPoint(const CPoint &point)
{
  if (!m_focusedLayout || !m_layout)
    return false;

  const float mouse_scroll_speed = 0.05f;
  const float mouse_max_amount = 1.0f;   // max speed: 1 item per frame
  float sizeOfItem = m_layout->Size(m_orientation);
  // see if the point is either side of our focused item
  float start = GetCursor() * sizeOfItem;
  float end = start + m_focusedLayout->Size(m_orientation);
  float pos = (m_orientation == VERTICAL) ? point.y : point.x;
  if (pos < start - 0.5f * sizeOfItem)
  { // scroll backward
    if (!InsideLayout(m_layout, point))
      return false;
    float amount = std::min((start - pos) / sizeOfItem, mouse_max_amount);
    m_analogScrollCount += amount * amount * mouse_scroll_speed;
    if (m_analogScrollCount > 1)
    {
      Scroll(-1);
      m_analogScrollCount-=1.0f;
    }
    return true;
  }
  else if (pos > end + 0.5f * sizeOfItem)
  { // scroll forward
    if (!InsideLayout(m_layout, point))
      return false;

    float amount = std::min((pos - end) / sizeOfItem, mouse_max_amount);
    m_analogScrollCount += amount * amount * mouse_scroll_speed;
    if (m_analogScrollCount > 1)
    {
      Scroll(1);
      m_analogScrollCount-=1.0f;
    }
    return true;
  }
  return InsideLayout(m_focusedLayout, point);
}

void CGUIWrappingListContainer::SelectItem(int item)
{
  if (item >= 0 && item < m_items.Size())
    ScrollToOffset(item - GetCursor());
}

void CGUIWrappingListContainer::ResetExtraItems()
{
  // delete any extra items
  if (m_extraItems)
    m_items.RemoveRange(m_items.Size() - m_extraItems, m_items.Size());
  m_extraItems = 0;
}

void CGUIWrappingListContainer::Reset()
{
  ResetExtraItems();
  CGUIBaseContainer::Reset();
}

int CGUIWrappingListContainer::GetCurrentPage() const
{
  int offset = CorrectOffset(GetOffset(), GetCursor());
  if (offset + m_itemsPerPage - GetCursor() >= (int)GetRows())  // last page
    return (GetRows() + m_itemsPerPage - 1) / m_itemsPerPage;
  return offset / m_itemsPerPage + 1;
}

void CGUIWrappingListContainer::SetPageControlRange()
{
  if (m_pageControl)
  {
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, GetNumItems());
    SendWindowMessage(msg);
  }
}

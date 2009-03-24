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
#include "GUIWrappingListContainer.h"
#include "FileItem.h"

CGUIWrappingListContainer::CGUIWrappingListContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime, int fixedPosition)
    : CGUIBaseContainer(dwParentID, dwControlId, posX, posY, width, height, orientation, scrollTime)
{
  m_cursor = fixedPosition;
  ControlType = GUICONTAINER_WRAPLIST;
  m_type = VIEW_TYPE_LIST;
  m_extraItems = 0;
}

CGUIWrappingListContainer::~CGUIWrappingListContainer(void)
{
}

void CGUIWrappingListContainer::Render()
{
  if (!IsVisible()) return;

  ValidateOffset();

  if (m_bInvalidated)
    UpdateLayout();

  if (!m_layout || !m_focusedLayout) return;

  UpdateScrollOffset();

  int offset = (int)floorf(m_scrollOffset / m_layout->Size(m_orientation));
  // Free memory not used on scre  if (m_scrollSpeed)
  if ((int)m_items.size() > m_itemsPerPage)
    FreeMemory(CorrectOffset(offset, 0), CorrectOffset(offset, m_itemsPerPage + 1));

  g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height);
  float posX = m_posX;
  float posY = m_posY;
  if (m_orientation == VERTICAL)
    posY += (offset * m_layout->Size(m_orientation) - m_scrollOffset);
  else
    posX += (offset * m_layout->Size(m_orientation) - m_scrollOffset);;

  float focusedPosX = 0;
  float focusedPosY = 0;
  CGUIListItemPtr focusedItem;
  int current = offset;
  while (posX < m_posX + m_width && posY < m_posY + m_height && m_items.size())
  {
    CGUIListItemPtr item = m_items[CorrectOffset(current, 0)];
    bool focused = (current == m_offset + m_cursor) && m_bHasFocus;
    // render our item
    if (focused)
    {
      focusedPosX = posX;
      focusedPosY = posY;
      focusedItem = item;
    }
    else
      RenderItem(posX, posY, item.get(), focused);

    // increment our position
    if (m_orientation == VERTICAL)
      posY += focused ? m_focusedLayout->Size(m_orientation) : m_layout->Size(m_orientation);
    else
      posX += focused ? m_focusedLayout->Size(m_orientation) : m_layout->Size(m_orientation);

    current++;
  }
  // render focused item last so it can overlap other items
  if (focusedItem)
    RenderItem(focusedPosX, focusedPosY, focusedItem.get(), true);

  g_graphicsContext.RestoreClipRegion();

  if (m_pageControl)
  { // tell our pagecontrol (scrollbar or whatever) to update (offset it by our cursor position)
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, CorrectOffset(offset, m_cursor));
    SendWindowMessage(msg);
  }
  CGUIBaseContainer::Render();
}

bool CGUIWrappingListContainer::OnAction(const CAction &action)
{
  switch (action.wID)
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
      m_analogScrollCount += action.fAmount1 * action.fAmount1;
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
      m_analogScrollCount += action.fAmount1 * action.fAmount1;
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
    if (message.GetMessage() == GUI_MSG_ITEM_SELECT)
    {
      SelectItem(message.GetParam1());
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_PAGE_CHANGE)
    {
      if (message.GetSenderId() == m_pageControl && IsVisible())
      { // offset by our cursor position
        message.SetParam1(message.GetParam1() - m_cursor);
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
  ScrollToOffset(m_offset + amount);
}

void CGUIWrappingListContainer::ValidateOffset()
{
  if (m_itemsPerPage <= (int)m_items.size())
    return;

  // no need to check the range here, but we need to check we have
  // more items than slots.
  ResetExtraItems();
  if (m_items.size())
  {
    unsigned int numItems = m_items.size();
    while (m_items.size() < (unsigned int)m_itemsPerPage)
    {
      // add additional copies of items, as we require extras at render time
      for (unsigned int i = 0; i < numItems; i++)
      {
        if (m_items[i]->IsFileItem())
          m_items.push_back(CFileItemPtr(new CFileItem(*(CFileItem *)m_items[i].get())));
        else
          m_items.push_back(CGUIListItemPtr(new CGUIListItem(*m_items[i])));
        m_extraItems++;
      }
    }
  }
}

int CGUIWrappingListContainer::CorrectOffset(int offset, int cursor) const
{
  if (m_items.size())
  {
    int correctOffset = (offset + cursor) % (int)m_items.size();
    if (correctOffset < 0) correctOffset += m_items.size();
    return correctOffset;
  }
  return 0;
}

int CGUIWrappingListContainer::GetSelectedItem() const
{
  if (m_items.size() > m_extraItems)
  {
    int numItems = (int)(m_items.size() - m_extraItems);
    int correctOffset = (m_offset + m_cursor) % numItems;
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
  float start = m_cursor * sizeOfItem;
  float end = start + m_focusedLayout->Size(m_orientation);
  float pos = (m_orientation == VERTICAL) ? point.y : point.x;
  if (pos < start - 0.5f * sizeOfItem)
  { // scroll backward
    if (!InsideLayout(m_layout, point))
      return false;
    float amount = std::min((start - pos) / sizeOfItem, mouse_max_amount);
    m_analogScrollCount += amount * amount * mouse_scroll_speed;
    CLog::Log(LOGERROR, "%s: Speed %f", __FUNCTION__, amount);
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
  if (item >= 0 && item < (int)m_items.size())
    ScrollToOffset(item - m_cursor);
}

void CGUIWrappingListContainer::ResetExtraItems()
{
  // delete any extra items
  if (m_extraItems)
    m_items.erase(m_items.begin() + m_items.size() - m_extraItems, m_items.end());
  m_extraItems = 0;
}

void CGUIWrappingListContainer::Reset()
{
  ResetExtraItems();
  CGUIBaseContainer::Reset();
}

int CGUIWrappingListContainer::GetCurrentPage() const
{
  int offset = CorrectOffset(m_offset, m_cursor);
  if (offset + m_itemsPerPage - m_cursor >= (int)GetRows())  // last page
    return (GetRows() + m_itemsPerPage - 1) / m_itemsPerPage;
  return offset / m_itemsPerPage + 1;
}

void CGUIWrappingListContainer::SetPageControlRange()
{
  if (m_pageControl)
  {
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, m_items.size() + m_itemsPerPage - 1);
    SendWindowMessage(msg);
  }
}

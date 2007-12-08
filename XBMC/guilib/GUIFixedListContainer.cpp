#include "include.h"
#include "GUIFixedListContainer.h"
#include "GUIListItem.h"
#include "../xbmc/utils/GUIInfoManager.h"

CGUIFixedListContainer::CGUIFixedListContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime, int fixedPosition)
    : CGUIBaseContainer(dwParentID, dwControlId, posX, posY, width, height, orientation, scrollTime)
{
  ControlType = GUICONTAINER_FIXEDLIST;
  m_type = VIEW_TYPE_LIST;
  m_cursor = fixedPosition;
}

CGUIFixedListContainer::~CGUIFixedListContainer(void)
{
}

void CGUIFixedListContainer::Render()
{
  ValidateOffset();

  if (m_bInvalidated)
    UpdateLayout();

  if (!m_focusedLayout || !m_layout) return;

  m_scrollOffset += m_scrollSpeed * (m_renderTime - m_scrollLastTime);
  if ((m_scrollSpeed < 0 && m_scrollOffset < m_offset * m_layout->Size(m_orientation)) ||
      (m_scrollSpeed > 0 && m_scrollOffset > m_offset * m_layout->Size(m_orientation)))
  {
    m_scrollOffset = m_offset * m_layout->Size(m_orientation);
    m_scrollSpeed = 0;
  }
  m_scrollLastTime = m_renderTime;

  int offset = (int)(m_scrollOffset / m_layout->Size(m_orientation));
  // Free memory not used on screen at the moment, do this first so there's more memory for the new items.
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
  CGUIListItem *focusedItem = NULL;
  int current = offset;
  while (posX < m_posX + m_width && posY < m_posY + m_height && m_items.size())
  {
    if (current >= (int)m_items.size())
      break;
    bool focused = (current == m_offset + m_cursor);
    if (current >= 0)
    {
      CGUIListItem *item = m_items[current];
      // render our item
      if (focused)
      {
        focusedPosX = posX;
        focusedPosY = posY;
        focusedItem = item;
      }
      else
        RenderItem(posX, posY, item, focused);
    }

    // increment our position
    if (m_orientation == VERTICAL)
      posY += focused ? m_focusedLayout->Size(m_orientation) : m_layout->Size(m_orientation);
    else
      posX += focused ? m_focusedLayout->Size(m_orientation) : m_layout->Size(m_orientation);

    current++;
  }
  // and render the focused item last (for overlapping purposes)
  if (focusedItem)
    RenderItem(focusedPosX, focusedPosY, focusedItem, true);

  g_graphicsContext.RestoreClipRegion();

  if (m_pageControl)
  { // tell our pagecontrol (scrollbar or whatever) to update
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, offset);
    SendWindowMessage(msg);
  }
  CGUIBaseContainer::Render();
}

bool CGUIFixedListContainer::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_PAGE_UP:
    {
      if (m_offset + m_cursor < m_itemsPerPage)
      { // already on the first page, so move to the first item
        Scroll(m_offset + m_cursor - m_itemsPerPage);
      }
      else
      { // scroll up to the previous page
        Scroll(-m_itemsPerPage);
      }
      return true;
    }
    break;
  case ACTION_PAGE_DOWN:
    {
      if (m_offset + m_cursor >= (int)m_items.size() || (int)m_items.size() < m_itemsPerPage)
      { // already at the last page, so move to the last item.
        Scroll(m_items.size() - 1 - m_offset + m_cursor);
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
      m_analogScrollCount += action.fAmount1 * action.fAmount1;
      bool handled = false;
      while (m_analogScrollCount > 0.4)
      {
        handled = true;
        m_analogScrollCount -= 0.4f;
        if (m_offset > -m_cursor)
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
        if (m_offset < (int)m_items.size() - 1)
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
  if (message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_ITEM_SELECT)
    {
      SelectItem(message.GetParam1());
      return true;
    }
  }
  return CGUIBaseContainer::OnMessage(message);
}

bool CGUIFixedListContainer::MoveUp(DWORD control)
{
  if (m_offset > -m_cursor)
    ScrollToOffset(m_offset - 1);
  else if ( control == 0 || control == GetID() )
  {
    if (m_items.size() > 0)
    { // move 2 last item in list
      int offset = m_items.size() - m_cursor - 1;
      if (offset < -m_cursor) offset = -m_cursor;
      ScrollToOffset(offset);
      g_infoManager.SetContainerMoving(GetID(), -1);
    }
  }
  else
    return false;
  return true;
}

bool CGUIFixedListContainer::MoveDown(DWORD control)
{
  if (m_offset + m_cursor + 1 < (int)m_items.size())
  {
    ScrollToOffset(m_offset + 1);
  }
  else if( control == 0 || control == GetID() )
  { // move first item in list
    ScrollToOffset(-m_cursor);
    g_infoManager.SetContainerMoving(GetID(), 1);
  }
  else
    return false;
  return true;
}

// scrolls the said amount
void CGUIFixedListContainer::Scroll(int amount)
{
  // increase or decrease the offset
  int offset = m_offset + amount;
  if (offset >= (int)m_items.size() - m_cursor)
  {
    offset = m_items.size() - m_cursor - 1;
  }
  if (offset < -m_cursor) offset = -m_cursor;
  ScrollToOffset(offset);
}

void CGUIFixedListContainer::ValidateOffset()
{ // first thing is we check the range of m_offset
  if (!m_layout) return;
  if (m_offset > (int)m_items.size() - m_cursor)
  {
    m_offset = m_items.size() - m_cursor;
    m_scrollOffset = m_offset * m_layout->Size(m_orientation);
  }
  if (m_offset < -m_cursor)
  {
    m_offset = -m_cursor;
    m_scrollOffset = m_offset * m_layout->Size(m_orientation);
  }
}

bool CGUIFixedListContainer::SelectItemFromPoint(const CPoint &point)
{
  if (!m_focusedLayout || !m_layout)
    return false;

  const float mouse_scroll_speed = 0.5f;
  // see if the point is either side of our focused item
  float start = m_cursor * m_layout->Size(m_orientation);
  float end = start + m_focusedLayout->Size(m_orientation);
  float pos = (m_orientation == VERTICAL) ? point.y : point.x;
  if (pos < start)
  { // scroll backward
    if (!InsideLayout(m_layout, point))
      return false;
    float amount = (start - pos) / m_layout->Size(m_orientation);
    m_analogScrollCount += amount * amount * mouse_scroll_speed;
    if (m_analogScrollCount > 1)
    {
      Scroll(-1);
      m_analogScrollCount-=1.0f;
    }
    return true;
  }
  else if (pos > end)
  {
    if (!InsideLayout(m_layout, point))
      return false;
    // scroll forward
    float amount = (pos - end) / m_layout->Size(m_orientation);
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

void CGUIFixedListContainer::SelectItem(int item)
{
  // Check that m_offset is valid
  ValidateOffset();
  // only select an item if it's in a valid range
  if (item >= 0 && item < (int)m_items.size())
  {
    // Select the item requested
    ScrollToOffset(item - m_cursor);
  }
}


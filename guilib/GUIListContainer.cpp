#include "include.h"
#include "GUIListContainer.h"
#include "GUIListItem.h"

//#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
#include "../xbmc/utils/GUIInfoManager.h"
//#endif

CGUIListContainer::CGUIListContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime)
    : CGUIBaseContainer(dwParentID, dwControlId, posX, posY, width, height, orientation, scrollTime)
{
  ControlType = GUICONTAINER_LIST;
  m_type = VIEW_TYPE_LIST;
//#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  m_spinControl = NULL;
//#endif
  m_wasReset = false;
}

CGUIListContainer::~CGUIListContainer(void)
{
}

void CGUIListContainer::Render()
{
  ValidateOffset();

  if (m_bInvalidated)
    UpdateLayout();

  m_scrollOffset += m_scrollSpeed * (m_renderTime - m_scrollLastTime);
  if ((m_scrollSpeed < 0 && m_scrollOffset < m_offset * m_layout.Size(m_orientation)) ||
      (m_scrollSpeed > 0 && m_scrollOffset > m_offset * m_layout.Size(m_orientation)))
  {
    m_scrollOffset = m_offset * m_layout.Size(m_orientation);
    m_scrollSpeed = 0;
  }
  m_scrollLastTime = m_renderTime;

  int offset = (int)(m_scrollOffset / m_layout.Size(m_orientation));
  // Free memory not used on screen at the moment, do this first so there's more memory for the new items.
  FreeMemory(CorrectOffset(offset, 0), CorrectOffset(offset, m_itemsPerPage + 1));

  g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height);
  float posX = m_posX;
  float posY = m_posY;
  // we offset our draw position to take into account scrolling and whether or not our focused
  // item is offscreen "above" the list.
  float drawOffset = offset * m_layout.Size(m_orientation) - m_scrollOffset;
  if (offset > m_offset + m_cursor)
    drawOffset += m_focusedLayout.Size(m_orientation) - m_layout.Size(m_orientation);

  if (m_orientation == VERTICAL)
    posY += drawOffset;
  else
    posX += drawOffset;

  float focusedPosX = 0;
  float focusedPosY = 0;
  CGUIListItem *focusedItem = NULL;
  int current = offset;
  while (posX < m_posX + m_width && posY < m_posY + m_height && m_items.size())
  {
    if (current >= (int)m_items.size())
      break;
    CGUIListItem *item = m_items[current];
    bool focused = (current == m_offset + m_cursor);
    // render our item
    if (focused)
    {
      focusedPosX = posX;
      focusedPosY = posY;
      focusedItem = item;
    }
    else
      RenderItem(posX, posY, item, focused);

    // increment our position
    if (m_orientation == VERTICAL)
      posY += focused ? m_focusedLayout.Size(m_orientation) : m_layout.Size(m_orientation);
    else
      posX += focused ? m_focusedLayout.Size(m_orientation) : m_layout.Size(m_orientation);

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

  m_wasReset = false;
  CGUIBaseContainer::Render();
}

bool CGUIListContainer::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_PAGE_UP:
    {
      if (m_offset == 0)
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
      if (m_offset == (int)m_items.size() - m_itemsPerPage || (int)m_items.size() < m_itemsPerPage)
      { // already at the last page, so move to the last item.
        SetCursor(m_items.size() - m_offset - 1);
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
        if (m_offset > 0 && m_cursor <= m_itemsPerPage / 2)
        {
          Scroll(-1);
        }
        else if (m_cursor > 0)
        {
          SetCursor(m_cursor - 1);
        }
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
        if (m_offset + m_itemsPerPage < (int)m_items.size() && m_cursor >= m_itemsPerPage / 2)
        {
          Scroll(1);
        }
        else if (m_cursor < m_itemsPerPage - 1 && m_offset + m_cursor < (int)m_items.size() - 1)
        {
          SetCursor(m_cursor + 1);
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
      m_wasReset = true;
      SetCursor(0);
    }
    else if (message.GetMessage() == GUI_MSG_ITEM_SELECT)
    {
      // Check that m_offset is valid
      ValidateOffset();
      // only select an item if it's in a valid range
      if (message.GetParam1() >= 0 && message.GetParam1() < m_items.size())
      {
        // Select the item requested
        int item = message.GetParam1();
        if (item >= m_offset && item < m_offset + m_itemsPerPage)
        { // the item is on the current page, so don't change it.
          SetCursor(item - m_offset);
        }
        else if (item < m_offset)
        { // item is on a previous page - make it the first item on the page
          SetCursor(0);
          ScrollToOffset(item);
        }
        else // (item >= m_offset+m_itemsPerPage)
        { // item is on a later page - make it the last item on the page
          SetCursor(m_itemsPerPage - 1);
          ScrollToOffset(item - m_cursor);
        }
      }
      return true;
    }
  }
  return CGUIBaseContainer::OnMessage(message);
}

bool CGUIListContainer::MoveUp(DWORD control)
{
  if (m_cursor > 0)
  {
    SetCursor(m_cursor - 1);
  }
  else if (m_cursor == 0 && m_offset)
  {
    ScrollToOffset(m_offset - 1);
  }
  else if ( control == 0 || control == GetID() )
  {
    if (m_items.size() > 0)
    { // move 2 last item in list, and set our container moving up
      int offset = m_items.size() - m_itemsPerPage;
      if (offset < 0) offset = 0;
      SetCursor(m_items.size() - offset - 1);
      ScrollToOffset(offset);
      g_infoManager.SetContainerMoving(GetID(), -1);
    }
  }
  else
    return false;
  return true;
}

bool CGUIListContainer::MoveDown(DWORD control)
{
  if (m_offset + m_cursor + 1 < (int)m_items.size())
  {
    if (m_cursor + 1 < m_itemsPerPage)
    {
      SetCursor(m_cursor + 1);
    }
    else
    {
      ScrollToOffset(m_offset + 1);
    }
  }
  else if( control == 0 || control == GetID() )
  { // move first item in list, and set our container moving in the "down" direction
    SetCursor(0);
    ScrollToOffset(0);
    g_infoManager.SetContainerMoving(GetID(), 1);
  }
  else
    return false;
  return true;
}

// scrolls the said amount
void CGUIListContainer::Scroll(int amount)
{
  // increase or decrease the offset
  int offset = m_offset + amount;
  if (offset > (int)m_items.size() - m_itemsPerPage)
  {
    offset = m_items.size() - m_itemsPerPage;
  }
  if (offset < 0) offset = 0;
  ScrollToOffset(offset);
}

void CGUIListContainer::ValidateOffset()
{ // first thing is we check the range of m_offset
  if (m_offset > (int)m_items.size() - m_itemsPerPage)
  {
    m_offset = m_items.size() - m_itemsPerPage;
    m_scrollOffset = m_offset * m_layout.Size(m_orientation);
  }
  if (m_offset < 0)
  {
    m_offset = 0;
    m_scrollOffset = 0;
  }
}

void CGUIListContainer::SetCursor(int cursor)
{
  if (cursor > m_itemsPerPage - 1) cursor = m_itemsPerPage - 1;
  if (cursor < 0) cursor = 0;
  if (!m_wasReset)
    g_infoManager.SetContainerMoving(GetID(), cursor - m_cursor);
  m_cursor = cursor;
}

//#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
CGUIListContainer::CGUIListContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height,
                                 const CLabelInfo& labelInfo, const CLabelInfo& labelInfo2,
                                 const CImage& textureButton, const CImage& textureButtonFocus,
                                 float textureHeight, float itemWidth, float itemHeight, float spaceBetweenItems, CGUIControl *pSpin)
: CGUIBaseContainer(dwParentID, dwControlId, posX, posY, width, height, VERTICAL, 200) 
{
  m_layout.CreateListControlLayouts(width, textureHeight + spaceBetweenItems, false, labelInfo, labelInfo2, textureButton, textureButtonFocus, textureHeight, itemWidth, itemHeight, 0, 0);
  CStdString condition;
  condition.Format("control.hasfocus(%i)", dwControlId);
  CStdString condition2 = "!" + condition;
  m_focusedLayout.CreateListControlLayouts(width, textureHeight + spaceBetweenItems, true, labelInfo, labelInfo2, textureButton, textureButtonFocus, textureHeight, itemWidth, itemHeight, g_infoManager.TranslateString(condition2), g_infoManager.TranslateString(condition));
  m_height = floor(m_height / (textureHeight + spaceBetweenItems)) * (textureHeight + spaceBetweenItems);
  m_spinControl = pSpin;
  ControlType = GUICONTAINER_LIST;
}
//#endif

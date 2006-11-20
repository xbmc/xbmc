#include "include.h"
#include "GUIListContainer.h"
#include "GUIListItem.h"

#define SCROLL_TIME 200

CGUIListContainer::CGUIListContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  m_cursor = 0;
  m_offset = 0;
  m_scrollOffset = 0;
  m_scrollSpeed = 0;
  m_scrollTime = 0;
  m_itemsPerPage = 10;
  m_pageControl = 0;
  m_renderTime = 0;
  m_orientation = orientation;
  ControlType = GUICONTAINER_LIST;
//#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  m_spinControl = NULL;
//#endif
}

CGUIListContainer::~CGUIListContainer(void)
{
}

void CGUIListContainer::Render()
{
  if (!IsVisible()) return;

  ValidateOffset();

  if (m_bInvalidated)
    UpdateLayout();

  m_scrollOffset += m_scrollSpeed * (m_renderTime - m_scrollTime);
  if ((m_scrollSpeed < 0 && m_scrollOffset < m_offset * m_layout.Size(m_orientation)) ||
      (m_scrollSpeed > 0 && m_scrollOffset > m_offset * m_layout.Size(m_orientation)))
  {
    m_scrollOffset = m_offset * m_layout.Size(m_orientation);
    m_scrollSpeed = 0;
  }
  m_scrollTime = m_renderTime;

  int offset = (int)(m_scrollOffset / m_layout.Size(m_orientation));
  // Free memory not used on screen at the moment, do this first so there's more memory for the new items.
  if (offset < 30000)
  {
    for (int i = 0; i < offset && i < (int)m_items.size(); ++i)
    {
      CGUIListItem *item = m_items[i];
      if (item)
        item->FreeMemory();
    }
  }
  for (int i = offset + m_itemsPerPage; i < (int)m_items.size(); ++i)
  {
    CGUIListItem *item = m_items[i];
    if (item)
      item->FreeMemory();
  }

  g_graphicsContext.SetViewPort(m_posX, m_posY, m_width, m_height);
  float posX = m_posX;
  float posY = m_posY;
  if (m_orientation == VERTICAL)
    posY += (offset * m_layout.Size(m_orientation) - m_scrollOffset);
  else
    posX += (offset * m_layout.Size(m_orientation) - m_scrollOffset);;

  int current = offset;
  while (posX < m_posX + m_width && posY < m_posY + m_height)
  {
    if (current >= (int)m_items.size())
      break;
    CGUIListItem *item = m_items[current];
    bool focused = (current == m_offset + m_cursor) && m_bHasFocus;
    // render our item
    RenderItem(posX, posY, item, focused);

    // increment our position
    if (m_orientation == VERTICAL)
      posY += focused ? m_focusedLayout.Size(m_orientation) : m_layout.Size(m_orientation);
    else
      posX += focused ? m_focusedLayout.Size(m_orientation) : m_layout.Size(m_orientation);

    current++;
  }
  g_graphicsContext.RemoveGroupTransform();
  g_graphicsContext.RestoreViewPort();

  if (m_pageControl)
  { // tell our pagecontrol (scrollbar or whatever) to update
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, offset);
    SendWindowMessage(msg);
  }
  CGUIControl::Render();
}

void CGUIListContainer::RenderItem(float posX, float posY, CGUIListItem *item, bool focused)
{
  // set the origin
  g_graphicsContext.SetControlTransform(TransformMatrix::CreateTranslation(posX, posY));

  static CGUIListItem *lastItem = NULL;
  // TODO: Reset focused/unfocused item
  if (focused)
  {
    if (!item->GetFocusedLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(m_focusedLayout);
      item->SetFocusedLayout(layout);
    }
    if (item != lastItem)
      item->GetFocusedLayout()->ResetScrolling();
    if (item->GetFocusedLayout())
      item->GetFocusedLayout()->Render(item);
    lastItem = item;
  }
  else
  {
    if (!item->GetLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(m_layout);
      item->SetLayout(layout);
    }
    if (item->GetLayout())
      item->GetLayout()->Render(item);
  }
}

bool CGUIListContainer::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_PAGE_UP:
    {
      if (m_offset == 0)
      { // already on the first page, so move to the first item
        m_cursor = 0;
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
        m_cursor = m_items.size() - m_offset - 1;
        if (m_cursor < 0) m_cursor = 0;
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
      m_scrollOffset += action.fAmount1 * action.fAmount1;
      bool handled = false;
      while (m_scrollOffset > 0.4)
      {
        handled = true;
        m_scrollOffset -= 0.4f;
        if (m_offset > 0 && m_cursor <= m_itemsPerPage / 2)
        {
          Scroll(-1);
        }
        else if (m_cursor > 0)
        {
          m_cursor--;
        }
      }
      return handled;
    }
    break;
  case ACTION_SCROLL_DOWN:
    {
      m_scrollOffset += action.fAmount1 * action.fAmount1;
      bool handled = false;
      while (m_scrollOffset > 0.4)
      {
        handled = true;
        m_scrollOffset -= 0.4f;
        if (m_offset + m_itemsPerPage < (int)m_items.size() && m_cursor >= m_itemsPerPage / 2)
        {
          Scroll(1);
        }
        else if (m_cursor < m_itemsPerPage - 1 && m_offset + m_cursor < (int)m_items.size() - 1)
        {
          m_cursor++;
        }
      }
      return handled;
    }
    break;

  case ACTION_MOVE_LEFT:
  case ACTION_MOVE_RIGHT:
  case ACTION_MOVE_DOWN:
  case ACTION_MOVE_UP:
    { // use base class implementation
      return CGUIControl::OnAction(action);
    }
    break;

  default:
    if (action.wID)
    { // Don't know what to do, so send to our parent window.
      CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID(), action.wID);
      return g_graphicsContext.SendMessage(msg);
    }
  }
  return false;
}

bool CGUIListContainer::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_ADD)
    {
      CGUIListItem* item = (CGUIListItem*)message.GetLPVOID();
      m_items.push_back(item);
      if (m_pageControl)
      {
        CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, m_items.size());
        SendWindowMessage(msg);
      }
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_cursor = 0;
      m_items.clear();
      if (m_pageControl)
      {
        CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, m_items.size());
        SendWindowMessage(msg);
      }
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_ITEM_SELECTED)
    {
      message.SetParam1(m_offset + m_cursor);
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_ITEM_SELECT)
    {
      // Check that m_offset is valid
      ValidateOffset();
      // only select an item if it's in a valid range
      if (message.GetParam1() >= 0 && message.GetParam1() < (int)m_items.size())
      {
        // Select the item requested
        int item = message.GetParam1();
        if (item >= m_offset && item < m_offset + m_itemsPerPage)
        { // the item is on the current page, so don't change it.
          m_cursor = item - m_offset;
        }
        else if (item < m_offset)
        { // item is on a previous page - make it the first item on the page
          m_cursor = 0;
          m_offset = item;
          ScrollToOffset();
        }
        else // (item >= m_offset+m_itemsPerPage)
        { // item is on a later page - make it the last item on the page
          m_cursor = m_itemsPerPage - 1;
          m_offset = item - m_cursor;
          ScrollToOffset();
        }
      }
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_PAGE_CHANGE)
    {
      if (message.GetSenderId() == m_pageControl)
      { // update our page
        m_offset = message.GetParam1();
        ScrollToOffset();
        return true;
      }
    }
  }
  return CGUIControl::OnMessage(message);
}

void CGUIListContainer::OnUp()
{
  if (m_orientation == VERTICAL && MoveUp(m_dwControlUp))
    return;
  CGUIControl::OnUp();
}

void CGUIListContainer::OnDown()
{
  if (m_orientation == VERTICAL && MoveDown(m_dwControlDown))
    return;
  CGUIControl::OnDown();
}

void CGUIListContainer::OnLeft()
{
  if (m_orientation == HORIZONTAL && MoveUp(m_dwControlLeft))
    return;
  CGUIControl::OnLeft();
}

void CGUIListContainer::OnRight()
{
  if (m_orientation == HORIZONTAL && MoveDown(m_dwControlRight))
    return;
  CGUIControl::OnRight();
}

bool CGUIListContainer::MoveUp(DWORD control)
{
  if (m_cursor > 0)
    m_cursor--;
  else if (m_cursor == 0 && m_offset)
  {
    m_offset--;
    ScrollToOffset();
  }
  else if ( control == 0 || control == GetID() )
  {
    if (m_items.size() > 0)
    { // move 2 last item in list
      m_offset = m_items.size() - m_itemsPerPage;
      if (m_offset < 0) m_offset = 0;
      m_cursor = m_items.size() - m_offset - 1;
      ScrollToOffset();
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
      m_cursor++;
    else
    {
      m_offset++;
      ScrollToOffset();
    }
  }
  else if( m_dwControlDown == 0 || m_dwControlDown == GetID() )
  { // move first item in list
    m_offset = 0;
    m_cursor = 0;
    ScrollToOffset();
  }
  else
    return false;
  return true;
}

// scrolls the said amount
void CGUIListContainer::Scroll(int amount)
{
  // increase or decrease the offset
  m_offset += amount;
  if (m_offset > (int)m_items.size() - m_itemsPerPage)
  {
    m_offset = m_items.size() - m_itemsPerPage;
  }
  if (m_offset < 0) m_offset = 0;
  ScrollToOffset();
}

int CGUIListContainer::GetSelectedItem() const
{
  return m_cursor + m_offset;
}

bool CGUIListContainer::SelectItemFromPoint(float posX, float posY)
{
  int row = 0;
  float pos = (m_orientation == VERTICAL) ? posY : posX;
  while (row < m_itemsPerPage)
  {
    float size = (row == m_offset && m_bHasFocus) ? m_focusedLayout.Size(m_orientation) : m_layout.Size(m_orientation);
    if (pos < Size() && row + m_offset < (int)m_items.size())
    { // found
      m_cursor = row;
      return true;
    }
    row++;
    pos -= size;
  }
  return false;
}

bool CGUIListContainer::OnMouseOver()
{
  // select the item under the pointer
  if (SelectItemFromPoint(g_Mouse.posX - m_posX, g_Mouse.posY - m_posY))
    return CGUIControl::OnMouseOver();
  return false;
}

bool CGUIListContainer::OnMouseClick(DWORD dwButton)
{
  if (SelectItemFromPoint(g_Mouse.posX - m_posX, g_Mouse.posY - m_posY))
  { // send click message to window
    SEND_CLICK_MESSAGE(GetID(), GetParentID(), ACTION_MOUSE_CLICK + dwButton);
    return true;
  }
  return false;
}

bool CGUIListContainer::OnMouseDoubleClick(DWORD dwButton)
{
  if (SelectItemFromPoint(g_Mouse.posX - m_posX, g_Mouse.posY - m_posY))
  { // send double click message to window
    SEND_CLICK_MESSAGE(GetID(), GetParentID(), ACTION_MOUSE_DOUBLE_CLICK + dwButton);
    return true;
  }
  return false;
}

bool CGUIListContainer::OnMouseWheel()
{
  Scroll(-g_Mouse.cWheel);
  return true;
}

CStdString CGUIListContainer::GetDescription() const
{
  CStdString strLabel;
  int item = m_offset + m_cursor;
  if (item >= 0 && item < (int)m_items.size())
  {
    CGUIListItem *pItem = m_items[item];
    if (pItem->m_bIsFolder)
      strLabel.Format("[%s]", pItem->GetLabel().c_str());
    else
      strLabel = pItem->GetLabel();
  }
  return strLabel;
}

void CGUIListContainer::SetFocus(bool bOnOff)
{
  if (bOnOff != HasFocus())
    Update();
  CGUIControl::SetFocus(bOnOff);
}

void CGUIListContainer::SaveStates(vector<CControlState> &states)
{
  states.push_back(CControlState(GetID(), m_offset + m_cursor));
}

void CGUIListContainer::SetPageControl(DWORD id)
{
  m_pageControl = id;
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

void CGUIListContainer::UpdateEffectState(DWORD currentTime)
{
  CGUIControl::UpdateEffectState(currentTime);
  m_renderTime = currentTime;
}

void CGUIListContainer::Animate(DWORD currentTime)
{
  TransformMatrix transform;
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    anim.Animate(currentTime, HasRendered());
    // Update the control states (such as visibility)
    UpdateStates(anim.type, anim.currentProcess, anim.currentState);
    // and render the animation effect
    anim.RenderAnimation(transform);
  }
  g_graphicsContext.AddGroupTransform(transform);
}

void CGUIListContainer::UpdateLayout()
{
  // calculate the number of items to display
  if (HasFocus())
    m_itemsPerPage = (int)((Size() - m_focusedLayout.Size(m_orientation)) / m_layout.Size(m_orientation)) + 1;
  else
    m_itemsPerPage = (int)(Size() / m_layout.Size(m_orientation));
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, m_items.size());
  SendWindowMessage(msg);
}

inline float CGUIListContainer::Size() const
{
  return (m_orientation == HORIZONTAL) ? m_width : m_height;
}

void CGUIListContainer::ScrollToOffset()
{
  m_scrollSpeed = (m_offset * m_layout.Size(m_orientation) - m_scrollOffset) / SCROLL_TIME;
}

void CGUIListContainer::LoadLayout(TiXmlElement *layout)
{
  TiXmlElement *itemElement = layout->FirstChildElement("itemlayout");
  if (itemElement)
  { // we have a new item layout
    m_layout.LoadLayout(itemElement, false);
  }
  itemElement = layout->FirstChildElement("focusedlayout");
  if (itemElement)
  { // we have a new item layout
    m_focusedLayout.LoadLayout(itemElement, true);
  }
}

//#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
CGUIListContainer::CGUIListContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height,
                                 const CLabelInfo& labelInfo, const CLabelInfo& labelInfo2,
                                 const CImage& textureButton, const CImage& textureButtonFocus,
                                 float textureHeight, float itemWidth, float itemHeight, float spaceBetweenItems, CGUIControl *pSpin)
: CGUIControl(dwParentID, dwControlId, posX, posY, width, height) 
{
  m_layout.CreateListControlLayouts(width, textureHeight + spaceBetweenItems, false, labelInfo, labelInfo2, textureButton, textureHeight, itemWidth, itemHeight);
  m_focusedLayout.CreateListControlLayouts(width, textureHeight + spaceBetweenItems, true, labelInfo, labelInfo2, textureButtonFocus, textureHeight, itemWidth, itemHeight);
  m_height = floor(m_height / (textureHeight + spaceBetweenItems)) * (textureHeight + spaceBetweenItems);
  m_spinControl = pSpin;
  m_cursor = 0;
  m_offset = 0;
  m_scrollOffset = 0;
  m_scrollSpeed = 0;
  m_scrollTime = 0;
  m_itemsPerPage = 10;
  m_pageControl = 0;
  m_renderTime = 0;
  m_orientation = VERTICAL;
  ControlType = GUICONTAINER_LIST;
}
//#endif

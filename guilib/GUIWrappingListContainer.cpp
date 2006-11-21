#include "include.h"
#include "GUIWrappingListContainer.h"
#include "GUIListItem.h"

#define SCROLL_TIME 200

CGUIWrappingListContainer::CGUIWrappingListContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation, int fixedPosition)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  m_cursor = fixedPosition;
  m_offset = 0;
  m_scrollOffset = 0;
  m_scrollSpeed = 0;
  m_scrollTime = 0;
  m_itemsPerPage = 10;
  m_pageControl = 0;
  m_renderTime = 0;
  m_orientation = orientation;
  ControlType = GUICONTAINER_WRAPPINGLIST;
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
  FreeMemory(CorrectOffset(offset), CorrectOffset(offset + m_itemsPerPage));

  g_graphicsContext.SetViewPort(m_posX, m_posY, m_width, m_height);
  float posX = m_posX;
  float posY = m_posY;
  if (m_orientation == VERTICAL)
    posY += (offset * m_layout.Size(m_orientation) - m_scrollOffset);
  else
    posX += (offset * m_layout.Size(m_orientation) - m_scrollOffset);;

  int current = offset;
  while (posX < m_posX + m_width && posY < m_posY + m_height && m_items.size())
  {
    CGUIListItem *item = m_items[CorrectOffset(current)];
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
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, CorrectOffset(offset));
    SendWindowMessage(msg);
  }
  CGUIControl::Render();
}

void CGUIWrappingListContainer::RenderItem(float posX, float posY, CGUIListItem *item, bool focused)
{
  // set the origin
  g_graphicsContext.SetControlTransform(TransformMatrix::CreateTranslation(posX, posY));

  static CGUIListItem *lastItem = NULL;
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
      m_scrollOffset += action.fAmount1 * action.fAmount1;
      bool handled = false;
      while (m_scrollOffset > 0.4)
      {
        handled = true;
        m_scrollOffset -= 0.4f;
        Scroll(-1);
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
        Scroll(1);
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

bool CGUIWrappingListContainer::OnMessage(CGUIMessage& message)
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
      message.SetParam1(CorrectOffset(m_offset + m_cursor));
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_ITEM_SELECT)
    {
      // Check that m_offset is valid
      ValidateOffset();
      // only select an item if it's in a valid range
      int item = message.GetParam1();
      if (item >= 0 && item < (int)m_items.size())
      { // Select the item requested
        ScrollToOffset(item - m_cursor);
      }
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_PAGE_CHANGE)
    {
      if (message.GetSenderId() == m_pageControl)
      { // update our page
        ScrollToOffset(message.GetParam1());
        return true;
      }
    }
  }
  return CGUIControl::OnMessage(message);
}

void CGUIWrappingListContainer::OnUp()
{
  if (m_orientation == VERTICAL && MoveUp(m_dwControlUp))
    return;
  CGUIControl::OnUp();
}

void CGUIWrappingListContainer::OnDown()
{
  if (m_orientation == VERTICAL && MoveDown(m_dwControlDown))
    return;
  CGUIControl::OnDown();
}

void CGUIWrappingListContainer::OnLeft()
{
  if (m_orientation == HORIZONTAL && MoveUp(m_dwControlLeft))
    return;
  CGUIControl::OnLeft();
}

void CGUIWrappingListContainer::OnRight()
{
  if (m_orientation == HORIZONTAL && MoveDown(m_dwControlRight))
    return;
  CGUIControl::OnRight();
}

bool CGUIWrappingListContainer::MoveUp(DWORD control)
{
  Scroll(-1);
  return true;
}

bool CGUIWrappingListContainer::MoveDown(DWORD control)
{
  Scroll(+1);
  return true;
}

// scrolls the said amount
void CGUIWrappingListContainer::Scroll(int amount)
{
  ScrollToOffset(m_offset + amount);
}

int CGUIWrappingListContainer::GetSelectedItem() const
{
  return CorrectOffset(m_cursor + m_offset);
}

bool CGUIWrappingListContainer::SelectItemFromPoint(float posX, float posY)
{
  int row = 0;
  float pos = (m_orientation == VERTICAL) ? posY : posX;
  while (row < m_itemsPerPage)
  {
    float size = (row == m_offset && m_bHasFocus) ? m_focusedLayout.Size(m_orientation) : m_layout.Size(m_orientation);
    if (pos < Size() && row + m_offset < (int)m_items.size())
    { // found
      MoveToItem(row);
      return true;
    }
    row++;
    pos -= size;
  }
  return false;
}

bool CGUIWrappingListContainer::OnMouseOver()
{
  // select the item under the pointer
  if (SelectItemFromPoint(g_Mouse.posX - m_posX, g_Mouse.posY - m_posY))
    return CGUIControl::OnMouseOver();
  return false;
}

bool CGUIWrappingListContainer::OnMouseClick(DWORD dwButton)
{
  if (SelectItemFromPoint(g_Mouse.posX - m_posX, g_Mouse.posY - m_posY))
  { // send click message to window
    SEND_CLICK_MESSAGE(GetID(), GetParentID(), ACTION_MOUSE_CLICK + dwButton);
    return true;
  }
  return false;
}

bool CGUIWrappingListContainer::OnMouseDoubleClick(DWORD dwButton)
{
  if (SelectItemFromPoint(g_Mouse.posX - m_posX, g_Mouse.posY - m_posY))
  { // send double click message to window
    SEND_CLICK_MESSAGE(GetID(), GetParentID(), ACTION_MOUSE_DOUBLE_CLICK + dwButton);
    return true;
  }
  return false;
}

bool CGUIWrappingListContainer::OnMouseWheel()
{
  Scroll(-g_Mouse.cWheel);
  return true;
}

CStdString CGUIWrappingListContainer::GetDescription() const
{
  CStdString strLabel;
  int item = CorrectOffset(m_offset + m_cursor);
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

void CGUIWrappingListContainer::SetFocus(bool bOnOff)
{
  if (bOnOff != HasFocus())
    Update();
  CGUIControl::SetFocus(bOnOff);
}

void CGUIWrappingListContainer::SaveStates(vector<CControlState> &states)
{
  states.push_back(CControlState(GetID(), CorrectOffset(m_offset + m_cursor)));
}

void CGUIWrappingListContainer::SetPageControl(DWORD id)
{
  m_pageControl = id;
}

void CGUIWrappingListContainer::ValidateOffset()
{
  // no need to check the range here
}

void CGUIWrappingListContainer::UpdateEffectState(DWORD currentTime)
{
  CGUIControl::UpdateEffectState(currentTime);
  m_renderTime = currentTime;
}

void CGUIWrappingListContainer::Animate(DWORD currentTime)
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

void CGUIWrappingListContainer::UpdateLayout()
{
  // calculate the number of items to display
  if (HasFocus())
    m_itemsPerPage = (int)((Size() - m_focusedLayout.Size(m_orientation)) / m_layout.Size(m_orientation)) + 1;
  else
    m_itemsPerPage = (int)(Size() / m_layout.Size(m_orientation));
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, m_items.size());
  SendWindowMessage(msg);
}

inline float CGUIWrappingListContainer::Size() const
{
  return (m_orientation == HORIZONTAL) ? m_width : m_height;
}

void CGUIWrappingListContainer::ScrollToOffset(int offset)
{
  m_scrollSpeed = (offset * m_layout.Size(m_orientation) - m_scrollOffset) / SCROLL_TIME;
  m_offset = offset;
}

int CGUIWrappingListContainer::CorrectOffset(int offset) const
{
  if (m_items.size())
  {
    int correctOffset = offset % (int)m_items.size();
    if (correctOffset < 0) correctOffset += m_items.size();
    return correctOffset;
  }
  return 0;
}

void CGUIWrappingListContainer::LoadLayout(TiXmlElement *layout)
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

void CGUIWrappingListContainer::MoveToItem(int row)
{
  // TODO: Implement this...
  ScrollToOffset(row - m_cursor);
}

void CGUIWrappingListContainer::FreeMemory(int keepStart, int keepEnd)
{
  if (keepStart < keepEnd)
  { // remove before keepStart and after keepEnd
    for (int i = 0; i < keepStart && i < (int)m_items.size(); ++i)
      m_items[i]->FreeMemory();
    for (int i = keepEnd + 1; i < (int)m_items.size(); ++i)
      m_items[i]->FreeMemory();
  }
  else
  { // wrapping
    for (int i = keepEnd + 1; i < keepStart && i < (int)m_items.size(); ++i)
      m_items[i]->FreeMemory();
  }
}


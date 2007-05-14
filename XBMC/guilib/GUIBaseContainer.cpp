#include "include.h"
#include "GUIBaseContainer.h"
#include "GUIListItem.h"

CGUIBaseContainer::CGUIBaseContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  m_cursor = 0;
  m_offset = 0;
  m_scrollOffset = 0;
  m_scrollSpeed = 0;
  m_scrollLastTime = 0;
  m_scrollTime = scrollTime ? scrollTime : 1;
  m_itemsPerPage = 10;
  m_pageControl = 0;
  m_renderTime = 0;
  m_orientation = orientation;
  m_analogScrollCount = 0;
  m_lastItem = NULL;
}

CGUIBaseContainer::~CGUIBaseContainer(void)
{
}

void CGUIBaseContainer::Render()
{
  g_graphicsContext.RemoveGroupTransform();
  if (IsVisible())
    CGUIControl::Render();
}

void CGUIBaseContainer::RenderItem(float posX, float posY, CGUIListItem *item, bool focused)
{
  // set the origin
  g_graphicsContext.AddGroupTransform(TransformMatrix::CreateTranslation(posX, posY));

  if (m_bInvalidated)
    item->SetInvalid();
  if (focused)
  {
    if (!item->GetFocusedLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(m_focusedLayout);
      item->SetFocusedLayout(layout);
    }
    if (item->GetFocusedLayout())
    {
      if (item != m_lastItem || !HasFocus())
        item->GetFocusedLayout()->ResetScrolling();
      if (item != m_lastItem)
        item->GetFocusedLayout()->QueueAnimation(ANIM_TYPE_FOCUS);
      item->GetFocusedLayout()->Render(item, m_dwParentID, m_renderTime);
    }
    m_lastItem = item;
  }
  else
  {
    if (!item->GetLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(m_layout);
      item->SetLayout(layout);
    }
    if (item->GetLayout())
      item->GetLayout()->Render(item, m_dwParentID);
  }
  g_graphicsContext.RemoveGroupTransform();
}

bool CGUIBaseContainer::OnAction(const CAction &action)
{
  switch (action.wID)
  {
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
      return SendWindowMessage(msg);
    }
  }
  return false;
}

bool CGUIBaseContainer::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_ADD)
    {
      CGUIListItem* item = (CGUIListItem*)message.GetLPVOID();
      m_items.push_back(item);
      if (m_pageControl)
      {
        CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, GetRows());
        SendWindowMessage(msg);
      }
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_items.clear();
      if (m_pageControl)
      {
        CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, GetRows());
        SendWindowMessage(msg);
      }
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_ITEM_SELECTED)
    {
      message.SetParam1(CorrectOffset(m_offset, m_cursor));
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
    else if (message.GetMessage() == GUI_MSG_REFRESH_LIST)
    { // update our list contents
      for (unsigned int i = 0; i < m_items.size(); ++i)
        m_items[i]->SetInvalid();
    }
  }
  return CGUIControl::OnMessage(message);
}

void CGUIBaseContainer::OnUp()
{
  if (m_orientation == VERTICAL && MoveUp(m_dwControlUp))
    return;
  CGUIControl::OnUp();
}

void CGUIBaseContainer::OnDown()
{
  if (m_orientation == VERTICAL && MoveDown(m_dwControlDown))
    return;
  CGUIControl::OnDown();
}

void CGUIBaseContainer::OnLeft()
{
  if (m_orientation == HORIZONTAL && MoveUp(m_dwControlLeft))
    return;
  CGUIControl::OnLeft();
}

void CGUIBaseContainer::OnRight()
{
  if (m_orientation == HORIZONTAL && MoveDown(m_dwControlRight))
    return;
  CGUIControl::OnRight();
}

bool CGUIBaseContainer::MoveUp(DWORD control)
{
  return true;
}

bool CGUIBaseContainer::MoveDown(DWORD control)
{
  return true;
}

// scrolls the said amount
void CGUIBaseContainer::Scroll(int amount)
{
  ScrollToOffset(m_offset + amount);
}

int CGUIBaseContainer::GetSelectedItem() const
{
  return CorrectOffset(m_cursor, m_offset);
}

bool CGUIBaseContainer::SelectItemFromPoint(const CPoint &point)
{
  int row = 0;
  float pos = (m_orientation == VERTICAL) ? point.y : point.x;
  while (row < m_itemsPerPage)
  {
    float size = (row == m_cursor) ? m_focusedLayout.Size(m_orientation) : m_layout.Size(m_orientation);
    if (pos < size && row + m_offset < (int)m_items.size())
    { // found
      MoveToItem(row);
      return true;
    }
    row++;
    pos -= size;
  }
  return false;
}

bool CGUIBaseContainer::OnMouseOver(const CPoint &point)
{
  // select the item under the pointer
  if (SelectItemFromPoint(point - CPoint(m_posX, m_posY)))
    return CGUIControl::OnMouseOver(point);
  return false;
}

bool CGUIBaseContainer::OnMouseClick(DWORD dwButton, const CPoint &point)
{
  if (SelectItemFromPoint(point - CPoint(m_posX, m_posY)))
  { // send click message to window
    SEND_CLICK_MESSAGE(GetID(), GetParentID(), ACTION_MOUSE_CLICK + dwButton);
    return true;
  }
  return false;
}

bool CGUIBaseContainer::OnMouseDoubleClick(DWORD dwButton, const CPoint &point)
{
  if (SelectItemFromPoint(point - CPoint(m_posX, m_posY)))
  { // send double click message to window
    SEND_CLICK_MESSAGE(GetID(), GetParentID(), ACTION_MOUSE_DOUBLE_CLICK + dwButton);
    return true;
  }
  return false;
}

bool CGUIBaseContainer::OnMouseWheel(char wheel, const CPoint &point)
{
  Scroll(-wheel);
  return true;
}

CStdString CGUIBaseContainer::GetDescription() const
{
  CStdString strLabel;
  int item = CorrectOffset(m_offset, m_cursor);
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

void CGUIBaseContainer::SetFocus(bool bOnOff)
{
  if (bOnOff != HasFocus())
  {
    Update();
    m_lastItem = NULL;
  }
  CGUIControl::SetFocus(bOnOff);
}

void CGUIBaseContainer::SaveStates(vector<CControlState> &states)
{
  states.push_back(CControlState(GetID(), CorrectOffset(m_offset, m_cursor)));
}

void CGUIBaseContainer::SetPageControl(DWORD id)
{
  m_pageControl = id;
}

void CGUIBaseContainer::ValidateOffset()
{
}

void CGUIBaseContainer::UpdateEffectState(DWORD currentTime)
{
  CGUIControl::UpdateEffectState(currentTime);
  m_renderTime = currentTime;
}

void CGUIBaseContainer::Animate(DWORD currentTime)
{
  GUIVISIBLE visible = m_visible;
  TransformMatrix transform;
  for (unsigned int i = 0; i < m_animations.size(); i++)
  {
    CAnimation &anim = m_animations[i];
    anim.Animate(currentTime, HasRendered() || visible == DELAYED);
    // Update the control states (such as visibility)
    UpdateStates(anim.GetType(), anim.GetProcess(), anim.GetState());
    // and render the animation effect
    anim.RenderAnimation(transform);
  }
  g_graphicsContext.AddGroupTransform(transform);
}

void CGUIBaseContainer::AllocResources()
{
  CalculateLayout();
}

void CGUIBaseContainer::UpdateLayout()
{
  CalculateLayout();
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, GetRows());
  SendWindowMessage(msg);
}

void CGUIBaseContainer::CalculateLayout()
{
  // calculate the number of items to display
  m_itemsPerPage = (int)((Size() - m_focusedLayout.Size(m_orientation)) / m_layout.Size(m_orientation)) + 1;
}

unsigned int CGUIBaseContainer::GetRows() const
{
  return m_items.size();
}

inline float CGUIBaseContainer::Size() const
{
  return (m_orientation == HORIZONTAL) ? m_width : m_height;
}

#define MAX_SCROLL_AMOUNT 0.4f

void CGUIBaseContainer::ScrollToOffset(int offset)
{
  float size = m_layout.Size(m_orientation);
  int range = m_itemsPerPage / 4;
  if (range <= 0) range = 1;
  if (offset * size < m_scrollOffset &&  m_scrollOffset - offset * size > size * range)
  { // scrolling up, and we're jumping more than 0.5 of a screen
    m_scrollOffset = (offset + range) * size;
  }
  if (offset * size > m_scrollOffset && offset * size - m_scrollOffset > size * range)
  { // scrolling down, and we're jumping more than 0.5 of a screen
    m_scrollOffset = (offset - range) * size;
  }
  m_scrollSpeed = (offset * size - m_scrollOffset) / m_scrollTime;
  m_offset = offset;
}

int CGUIBaseContainer::CorrectOffset(int offset, int cursor) const
{
  return offset + cursor;
}

void CGUIBaseContainer::LoadLayout(TiXmlElement *layout)
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

void CGUIBaseContainer::SetType(VIEW_TYPE type, const CStdString &label)
{
  m_type = type;
  m_label = label;
}

void CGUIBaseContainer::MoveToItem(int item)
{
  m_cursor = item;
}

void CGUIBaseContainer::FreeMemory(int keepStart, int keepEnd)
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

#ifdef _DEBUG
void CGUIBaseContainer::DumpTextureUse()
{
  CLog::Log(LOGDEBUG, __FUNCTION__" for container %i", GetID());
  for (unsigned int i = 0; i < m_items.size(); ++i)
  {
    CGUIListItem *item = m_items[i];
    if (item->GetFocusedLayout()) item->GetFocusedLayout()->DumpTextureUse();
    if (item->GetLayout()) item->GetLayout()->DumpTextureUse();
  }
}
#endif
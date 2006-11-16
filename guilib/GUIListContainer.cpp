#include "include.h"
#include "GUIListContainer.h"
#include "GUIListItem.h"
#include "../xbmc/utils/GUIInfoManager.h"

CGUIListContainer::CGUIListContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  m_cursor = 0;
  m_offset = 0;
  m_scrollOffset = 0;
  m_itemsPerPage = 10;
  m_itemHeight = 10;
  m_focusedHeight = 10;
  m_pageControl = 0;
  m_renderTime = 0;
  ControlType = GUICONTAINER_LIST;
}

CGUIListContainer::~CGUIListContainer(void)
{
  for (iControls it = m_controls.begin(); it != m_controls.end(); ++it)
    delete *it;
  for (iControls it = m_focusedControls.begin(); it != m_focusedControls.end(); ++it)
    delete *it;
}

void CGUIListContainer::Render()
{
  if (!IsVisible()) return;

  ValidateOffset();

  if (m_bInvalidated)
    UpdateLayout();

  // Free memory not used on screen at the moment, do this first so there's more memory for the new items.
  if (m_offset < 30000)
  {
    for (int i = 0; i < m_offset; ++i)
    {
      CGUIListItem *item = m_items[i];
      if (item)
        item->FreeMemory();
    }
  }
  for (int i = m_offset + m_itemsPerPage; i < (int)m_items.size(); ++i)
  {
    CGUIListItem *item = m_items[i];
    if (item)
      item->FreeMemory();
  }

  float posY = m_posY;
  for (int i = 0; i < m_itemsPerPage; i++)
  {
    if (i + m_offset < (int)m_items.size())
    {
      CGUIListItem *item = m_items[i + m_offset];

      bool focused = (i == m_cursor) && m_bHasFocus;
      // render our item
      RenderItem(m_posX, posY, item, focused);

      // increment our position
      posY += focused ? m_focusedHeight : m_itemHeight;
    }
  }
  g_graphicsContext.RemoveGroupTransform();

  if (m_pageControl)
  { // tell our pagecontrol (scrollbar or whatever) to update
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, m_offset);
    SendWindowMessage(msg);
  }
  CGUIControl::Render();
}

void CGUIListContainer::RenderItem(float posX, float posY, CGUIListItem *item, bool focused)
{
  // set the origin
  g_graphicsContext.AddGroupTransform(TransformMatrix::CreateTranslation(posX, posY));

  // set our item
  g_infoManager.SetListItem(item);

  // render our items
  if (focused)
  {
    for (iControls it = m_focusedControls.begin(); it != m_focusedControls.end(); ++it)
    {
      CGUIControl *control = *it;
      control->UpdateEffectState(m_renderTime);
      control->Render();
    }
  }
  else
  {
    for (iControls it = m_controls.begin(); it != m_controls.end(); ++it)
    {
      CGUIControl *control = *it;
      control->UpdateEffectState(m_renderTime);
      control->Render();
    }
  }
  g_infoManager.SetListItem(NULL);

  // restore origin
  g_graphicsContext.RemoveGroupTransform();
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
      if (m_offset > (int)m_items.size() - m_itemsPerPage)
        m_offset = m_items.size() - m_itemsPerPage;
      if (m_offset < 0) m_offset = 0;
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
        }
        else // (item >= m_offset+m_itemsPerPage)
        { // item is on a later page - make it the last item on the page
          m_cursor = m_itemsPerPage - 1;
          m_offset = item - m_cursor;
        }
      }
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_PAGE_CHANGE)
    {
      if (message.GetSenderId() == m_pageControl)
      { // update our page
        m_offset = message.GetParam1();
        return true;
      }
    }
  }
  return CGUIControl::OnMessage(message);
}

void CGUIListContainer::PreAllocResources()
{
  for (iControls it = m_controls.begin(); it != m_controls.end(); ++it)
    (*it)->PreAllocResources();
  for (iControls it = m_focusedControls.begin(); it != m_focusedControls.end(); ++it)
    (*it)->PreAllocResources();
}

void CGUIListContainer::AllocResources()
{
  for (iControls it = m_controls.begin(); it != m_controls.end(); ++it)
    (*it)->AllocResources();
  for (iControls it = m_focusedControls.begin(); it != m_focusedControls.end(); ++it)
    (*it)->AllocResources();
}

void CGUIListContainer::FreeResources()
{
  for (iControls it = m_controls.begin(); it != m_controls.end(); ++it)
    (*it)->FreeResources();
  for (iControls it = m_focusedControls.begin(); it != m_focusedControls.end(); ++it)
    (*it)->FreeResources();
}

void CGUIListContainer::DynamicResourceAlloc(bool bOnOff)
{
  for (iControls it = m_controls.begin(); it != m_controls.end(); ++it)
    (*it)->DynamicResourceAlloc(bOnOff);
  for (iControls it = m_focusedControls.begin(); it != m_focusedControls.end(); ++it)
    (*it)->DynamicResourceAlloc(bOnOff);
}

void CGUIListContainer::SetPulseOnSelect(bool pulse)
{
  for (iControls it = m_controls.begin(); it != m_controls.end(); ++it)
    (*it)->SetPulseOnSelect(pulse);
  for (iControls it = m_focusedControls.begin(); it != m_focusedControls.end(); ++it)
    (*it)->SetPulseOnSelect(pulse);
}

void CGUIListContainer::OnUp()
{
  if (m_cursor > 0)
    m_cursor--;
  else if (m_cursor == 0 && m_offset)
    m_offset--;
  else if ( m_dwControlUp == 0 || m_dwControlUp == GetID() )
  {
    if (m_items.size() > 0)
    { // move 2 last item in list
      m_offset = m_items.size() - m_itemsPerPage;
      if (m_offset < 0) m_offset = 0;
      m_cursor = m_items.size() - m_offset - 1;
    }
  }
  else
    CGUIControl::OnUp();
}

void CGUIListContainer::OnDown()
{
  if (m_offset + m_cursor + 1 < (int)m_items.size())
  {
    if (m_cursor + 1 < m_itemsPerPage)
      m_cursor++;
    else
      m_offset++;
  }
  else if( m_dwControlDown == 0 || m_dwControlDown == GetID() )
  { // move first item in list
    m_offset = 0;
    m_cursor = 0;
  }
  else
    CGUIControl::OnDown();
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
}

int CGUIListContainer::GetSelectedItem() const
{
  return m_cursor + m_offset;
}

bool CGUIListContainer::SelectItemFromPoint(float posX, float posY)
{
  int row = 0;
  while (row < m_itemsPerPage)
  {
    float height = (row == m_offset && m_bHasFocus) ? m_focusedHeight : m_itemHeight;
    if (posY < height && row + m_offset < (int)m_items.size())
    { // found
      m_cursor = row;
      return true;
    }
    row++;
    posY -= height;
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
  if (m_offset > (int)m_items.size() - m_itemsPerPage) m_offset = m_items.size() - m_itemsPerPage;
  if (m_offset < 0) m_offset = 0;
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

void CGUIListContainer::SetItemHeight(float itemHeight, float focusedHeight)
{
  m_itemHeight = itemHeight;
  m_focusedHeight = focusedHeight;
  Update();
}

void CGUIListContainer::SetItemLayout(const vector<CGUIControl*> &itemLayout, const vector<CGUIControl*> &focusedLayout)
{
  m_controls = itemLayout;
  m_focusedControls = focusedLayout;
  Update();
}

void CGUIListContainer::UpdateLayout()
{
  // calculate the number of items to display
  if (HasFocus())
    m_itemsPerPage = (int)((m_height - m_focusedHeight) / m_itemHeight) + 1;
  else
    m_itemsPerPage = (int)(m_height / m_itemHeight);
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, m_items.size());
  SendWindowMessage(msg);
}
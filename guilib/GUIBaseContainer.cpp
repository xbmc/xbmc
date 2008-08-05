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
#include "GUIBaseContainer.h"
#include "GuiControlFactory.h"
#include "utils/GUIInfoManager.h"
#include "XMLUtils.h"
#include "SkinInfo.h"
#include "FileItem.h"

using namespace std;

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
  m_staticContent = false;
  m_wasReset = false;
  m_layout = NULL;
  m_focusedLayout = NULL;
}

CGUIBaseContainer::~CGUIBaseContainer(void)
{
}

void CGUIBaseContainer::RenderItem(float posX, float posY, CGUIListItem *item, bool focused)
{
  if (!m_focusedLayout || !m_layout) return;

  // set the origin
  g_graphicsContext.SetOrigin(posX, posY);

  if (m_bInvalidated)
    item->SetInvalid();
  if (focused)
  {
    if (!item->GetFocusedLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(*m_focusedLayout);
      item->SetFocusedLayout(layout);
    }
    if (item->GetFocusedLayout())
    {
      if (item != m_lastItem || !HasFocus())
      {
        item->GetFocusedLayout()->SetFocusedItem(0);
      }
      if (item != m_lastItem && HasFocus())
      {
        item->GetFocusedLayout()->ResetAnimation(ANIM_TYPE_UNFOCUS);      
        unsigned int subItem = 1;
        if (m_lastItem && m_lastItem->GetFocusedLayout())
          subItem = m_lastItem->GetFocusedLayout()->GetFocusedItem();
        item->GetFocusedLayout()->SetFocusedItem(subItem ? subItem : 1);
      }
      item->GetFocusedLayout()->Render(item, m_dwParentID, m_renderTime);
    }
    m_lastItem = item;
  }
  else
  {
    if (item->GetFocusedLayout())
      item->GetFocusedLayout()->SetFocusedItem(0);  // focus is not set
    if (!item->GetLayout())
    {
      CGUIListItemLayout *layout = new CGUIListItemLayout(*m_layout);
      item->SetLayout(layout);
    }
    if (item->GetFocusedLayout() && item->GetFocusedLayout()->IsAnimating(ANIM_TYPE_UNFOCUS))
      item->GetFocusedLayout()->Render(item, m_dwParentID, m_renderTime);
    else if (item->GetLayout())
      item->GetLayout()->Render(item, m_dwParentID, m_renderTime);
  }
  g_graphicsContext.RestoreOrigin();
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

  case ACTION_MOVE_ITEM_DOWN:
    {
      OnNextLetter();
      return true;
    }
    break;
  case ACTION_MOVE_ITEM_UP:
    {
      OnPrevLetter();
      return true;
    }
    break;

  default:
    if (action.wID)
    { 
      return OnClick(action.wID);
    }
  }
  return false;
}

bool CGUIBaseContainer::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (!m_staticContent)
    {
      if (message.GetMessage() == GUI_MSG_LABEL_BIND && message.GetLPVOID())
      { // bind our items
        Reset();
        CFileItemList *items = (CFileItemList *)message.GetLPVOID();
        for (int i = 0; i < items->Size(); i++)
          m_items.push_back(items->Get(i));
        UpdateLayout(true); // true to refresh all items
        UpdateScrollByLetter();
        SelectItem(message.GetParam1());
        return true;
      }
      if (message.GetMessage() == GUI_MSG_LABEL_ADD && message.GetItem())
      {
        CGUIListItemPtr item = message.GetItem();
        m_items.push_back(item);
        UpdateScrollByLetter();
        if (m_pageControl)
        {
          CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, GetRows());
          SendWindowMessage(msg);
        }
        return true;
      }
      else if (message.GetMessage() == GUI_MSG_LABEL_RESET)
      {
        Reset();
        if (m_pageControl)
        {
          CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, GetRows());
          SendWindowMessage(msg);
        }
        return true;
      }
    }
    if (message.GetMessage() == GUI_MSG_ITEM_SELECTED)
    {
      message.SetParam1(GetSelectedItem());
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_PAGE_CHANGE)
    {
      if (message.GetSenderId() == m_pageControl && IsVisible())
      { // update our page if we're visible - not much point otherwise
        if ((int)message.GetParam1() != m_offset)
          m_pageChangeTimer.StartZero();
        ScrollToOffset(message.GetParam1());
        return true;
      }
    }
    else if (message.GetMessage() == GUI_MSG_REFRESH_LIST)
    { // update our list contents
      for (unsigned int i = 0; i < m_items.size(); ++i)
        m_items[i]->SetInvalid();
    }
    else if (message.GetMessage() == GUI_MSG_MOVE_OFFSET)
    {
      int count = (int)message.GetParam1();
      while (count < 0)
      {
        MoveUp(true);
        count++;
      }
      while (count > 0)
      {
        MoveDown(true);
        count--;
      }
      return true;
    }
  }
  return CGUIControl::OnMessage(message);
}

void CGUIBaseContainer::OnUp()
{
  bool wrapAround = m_dwControlUp == GetID() || !(m_dwControlUp || m_upActions.size());
  if (m_orientation == VERTICAL && MoveUp(wrapAround))
    return;
  // with horizontal lists it doesn't make much sense to have multiselect labels
  CGUIControl::OnUp();
}

void CGUIBaseContainer::OnDown()
{
  bool wrapAround = m_dwControlDown == GetID() || !(m_dwControlDown || m_downActions.size());
  if (m_orientation == VERTICAL && MoveDown(wrapAround))
    return;
  // with horizontal lists it doesn't make much sense to have multiselect labels
  CGUIControl::OnDown();
}

void CGUIBaseContainer::OnLeft()
{
  bool wrapAround = m_dwControlLeft == GetID() || !(m_dwControlLeft || m_leftActions.size());
  if (m_orientation == HORIZONTAL && MoveUp(wrapAround))
    return;
  else if (m_orientation == VERTICAL)
  {
    CGUIListItemLayout *focusedLayout = GetFocusedLayout();
    if (focusedLayout && focusedLayout->MoveLeft())
      return;
  }
  CGUIControl::OnLeft();
}

void CGUIBaseContainer::OnRight()
{
  bool wrapAround = m_dwControlRight == GetID() || !(m_dwControlRight || m_rightActions.size());
  if (m_orientation == HORIZONTAL && MoveDown(wrapAround))
    return;
  else if (m_orientation == VERTICAL)
  {
    CGUIListItemLayout *focusedLayout = GetFocusedLayout();
    if (focusedLayout && focusedLayout->MoveRight())
      return;
  }
  CGUIControl::OnRight();
}

void CGUIBaseContainer::OnNextLetter()
{
  int offset = CorrectOffset(m_offset, m_cursor);
  for (unsigned int i = 0; i < m_letterOffsets.size(); i++)
  {
    if (m_letterOffsets[i].first > offset)
    {
      SelectItem(m_letterOffsets[i].first);
      return;
    }
  }
}

void CGUIBaseContainer::OnPrevLetter()
{
  int offset = CorrectOffset(m_offset, m_cursor);
  if (!m_letterOffsets.size())
    return;
  for (unsigned int i = m_letterOffsets.size() - 1; i >= 0; i--)
  {
    if (m_letterOffsets[i].first < offset)
    {
      SelectItem(m_letterOffsets[i].first);
      return;
    }
  }
}

bool CGUIBaseContainer::MoveUp(bool wrapAround)
{
  return true;
}

bool CGUIBaseContainer::MoveDown(bool wrapAround)
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
  return CorrectOffset(m_offset, m_cursor);
}

CGUIListItemPtr CGUIBaseContainer::GetListItem(int offset, unsigned int flag) const
{
  if (!m_items.size())
    return CGUIListItemPtr();
  int item = GetSelectedItem() + offset;
  if (flag & INFOFLAG_LISTITEM_POSITION) // use offset from the first item displayed, taking into account scrolling
    item = CorrectOffset((int)(m_scrollOffset / m_layout->Size(m_orientation)), offset);

  if (flag & INFOFLAG_LISTITEM_WRAP)
  {
    item %= ((int)m_items.size());
    if (item < 0) item += m_items.size();
    return m_items[item];
  }
  else
  {
    if (item >= 0 && item < (int)m_items.size())
      return m_items[item];
  }
  return CGUIListItemPtr();
}

CGUIListItemLayout *CGUIBaseContainer::GetFocusedLayout() const
{
  CGUIListItemPtr item = GetListItem(0);
  if (item.get()) return item->GetFocusedLayout();
  return NULL;
}

bool CGUIBaseContainer::SelectItemFromPoint(const CPoint &point)
{
  if (!m_focusedLayout || !m_layout)
    return false;

  int row = 0;
  float pos = (m_orientation == VERTICAL) ? point.y : point.x;
  while (row < m_itemsPerPage)
  {
    const CGUIListItemLayout *layout = (row == m_cursor) ? m_focusedLayout : m_layout;
    if (pos < layout->Size(m_orientation) && row + m_offset < (int)m_items.size())
    { // found correct "row" -> check horizontal
      if (!InsideLayout(layout, point))
        return false;

      MoveToItem(row);
      CGUIListItemLayout *focusedLayout = GetFocusedLayout();
      if (focusedLayout)
      {
        CPoint pt(point);
        if (m_orientation == VERTICAL)
          pt.y = pos;
        else
          pt.x = pos;
        focusedLayout->SelectItemFromPoint(pt);
      }
      return true;
    }
    row++;
    pos -= layout->Size(m_orientation);
  }
  return false;
}

bool CGUIBaseContainer::OnMouseOver(const CPoint &point)
{
  // select the item under the pointer
  SelectItemFromPoint(point - CPoint(m_posX, m_posY));
  return CGUIControl::OnMouseOver(point);
}

bool CGUIBaseContainer::OnMouseClick(DWORD dwButton, const CPoint &point)
{
  if (SelectItemFromPoint(point - CPoint(m_posX, m_posY)))
  { // send click message to window
    OnClick(ACTION_MOUSE_CLICK + dwButton);
    return true;
  }
  return false;
}

bool CGUIBaseContainer::OnMouseDoubleClick(DWORD dwButton, const CPoint &point)
{
  if (SelectItemFromPoint(point - CPoint(m_posX, m_posY)))
  { // send double click message to window
    OnClick(ACTION_MOUSE_DOUBLE_CLICK + dwButton);
    return true;
  }
  return false;
}

bool CGUIBaseContainer::OnClick(DWORD actionID)
{
  int subItem = 0;
  if (actionID == ACTION_SELECT_ITEM || actionID == ACTION_MOUSE_LEFT_CLICK)
  {
    if (m_staticContent)
    { // "select" action
      int selected = GetSelectedItem();
      if (selected >= 0 && selected < (int)m_items.size())
      {
        CFileItemPtr item = boost::static_pointer_cast<CFileItem>(m_items[selected]);
        // multiple action strings are concat'd together, separated with " , "
        vector<CStdString> actions;
        StringUtils::SplitString(item->m_strPath, " , ", actions);
        for (unsigned int i = 0; i < actions.size(); i++)
        {
          CStdString action = actions[i];
          action.Replace(",,", ",");
          CGUIMessage message(GUI_MSG_EXECUTE, GetID(), GetParentID());
          message.SetStringParam(action);
          g_graphicsContext.SendMessage(message);
        }
      }
      return true;
    }
    // grab the currently focused subitem (if applicable)
    CGUIListItemLayout *focusedLayout = GetFocusedLayout();
    if (focusedLayout)
      subItem = focusedLayout->GetFocusedItem();
  }
  // Don't know what to do, so send to our parent window.
  CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID(), actionID, subItem);
  return SendWindowMessage(msg);
}

bool CGUIBaseContainer::OnMouseWheel(char wheel, const CPoint &point)
{
  Scroll(-wheel);
  return true;
}

CStdString CGUIBaseContainer::GetDescription() const
{
  CStdString strLabel;
  int item = GetSelectedItem();
  if (item >= 0 && item < (int)m_items.size())
  {
    CGUIListItemPtr pItem = m_items[item];
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
    SetInvalid();
    m_lastItem = NULL;
  }
  CGUIControl::SetFocus(bOnOff);
}

void CGUIBaseContainer::SaveStates(vector<CControlState> &states)
{
  states.push_back(CControlState(GetID(), GetSelectedItem()));
}

void CGUIBaseContainer::SetPageControl(DWORD id)
{
  m_pageControl = id;
}

void CGUIBaseContainer::ValidateOffset()
{
}

void CGUIBaseContainer::DoRender(DWORD currentTime)
{
  m_renderTime = currentTime;
  CGUIControl::DoRender(currentTime);
  if (m_pageChangeTimer.GetElapsedMilliseconds() > 200)
    m_pageChangeTimer.Stop();
  m_wasReset = false;
}

void CGUIBaseContainer::AllocResources()
{
  CalculateLayout();
}

void CGUIBaseContainer::FreeResources()
{
  CGUIControl::FreeResources();
  if (m_staticContent)
  { // free any static content
    Reset();
    m_staticItems.clear();
  }
  m_scrollSpeed = 0;
}

void CGUIBaseContainer::UpdateLayout(bool updateAllItems)
{
  if (updateAllItems)
  { // free memory of items
    for (iItems it = m_items.begin(); it != m_items.end(); it++)
      (*it)->FreeMemory();
  }
  // and recalculate the layout
  CalculateLayout();
  if (m_pageControl)
  {
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, GetRows());
    SendWindowMessage(msg);
  }
}

void CGUIBaseContainer::UpdateVisibility(const CGUIListItem *item)
{
  CGUIControl::UpdateVisibility(item);

  if (!IsVisible())
    return; // no need to update the content if we're not visible

  // check whether we need to update our layouts
  if ((m_layout && m_layout->GetCondition() && !g_infoManager.GetBool(m_layout->GetCondition(), GetParentID())) ||
      (m_focusedLayout && m_focusedLayout->GetCondition() && !g_infoManager.GetBool(m_focusedLayout->GetCondition(), GetParentID())))
  {
    // and do it
    int item = GetSelectedItem();
    UpdateLayout(true); // true to refresh all items
    SelectItem(item);
  }

  if (m_staticContent)
  { // update our item list with our new content, but only add those items that should
    // be visible.  Save the previous item and keep it if we are adding that one.
    CGUIListItem *lastItem = m_lastItem;
    Reset();
    for (unsigned int i = 0; i < m_staticItems.size(); ++i)
    {
      CFileItemPtr item = boost::static_pointer_cast<CFileItem>(m_staticItems[i]);
      // m_idepth is used to store the visibility condition
      if (!item->m_idepth || g_infoManager.GetBool(item->m_idepth, GetParentID()))
      {
        m_items.push_back(item);
        if (item.get() == lastItem)
          m_lastItem = lastItem;
      }
    }
    UpdateScrollByLetter();
  }
}

void CGUIBaseContainer::CalculateLayout()
{
  CGUIListItemLayout *oldFocusedLayout = m_focusedLayout;
  CGUIListItemLayout *oldLayout = m_layout;
  GetCurrentLayouts();

  // calculate the number of items to display
  assert(m_focusedLayout && m_layout);
  if (!m_focusedLayout || !m_layout) return;

  if (oldLayout == m_layout && oldFocusedLayout == m_focusedLayout)
    return; // nothing has changed, so don't update stuff

  m_itemsPerPage = (int)((Size() - m_focusedLayout->Size(m_orientation)) / m_layout->Size(m_orientation)) + 1;

  // ensure that the scroll offset is a multiple of our size
  m_scrollOffset = m_offset * m_layout->Size(m_orientation);
}

void CGUIBaseContainer::UpdateScrollByLetter()
{
  m_letterOffsets.clear();

  // for scrolling by letter we have an offset table into our vector.
  CStdString currentMatch;
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CGUIListItemPtr item = m_items[i];
    if (currentMatch != item->GetSortLabel().Left(1))
    {
      currentMatch = item->GetSortLabel().Left(1);
      m_letterOffsets.push_back(make_pair((int)i, currentMatch));
    }
  }
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
  float size = m_layout->Size(m_orientation);
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
  if (!m_wasReset)
  {
    g_infoManager.SetContainerMoving(GetID(), offset - m_offset);
    if (m_scrollSpeed)
      m_scrollTimer.Start();
    else
      m_scrollTimer.Stop();
  }
  m_offset = offset;
}

void CGUIBaseContainer::UpdateScrollOffset()
{
  m_scrollOffset += m_scrollSpeed * (m_renderTime - m_scrollLastTime);
  if ((m_scrollSpeed < 0 && m_scrollOffset < m_offset * m_layout->Size(m_orientation)) ||
      (m_scrollSpeed > 0 && m_scrollOffset > m_offset * m_layout->Size(m_orientation)))
  {
    m_scrollOffset = m_offset * m_layout->Size(m_orientation);
    m_scrollSpeed = 0;
    m_scrollTimer.Stop();
  }
  m_scrollLastTime = m_renderTime;
}

int CGUIBaseContainer::CorrectOffset(int offset, int cursor) const
{
  return offset + cursor;
}

void CGUIBaseContainer::Reset()
{
  m_wasReset = true;
  m_items.clear();
  m_lastItem = NULL;
}

void CGUIBaseContainer::LoadLayout(TiXmlElement *layout)
{
  TiXmlElement *itemElement = layout->FirstChildElement("itemlayout");
  while (itemElement)
  { // we have a new item layout
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, false);
    m_layouts.push_back(itemLayout);
    itemElement = itemElement->NextSiblingElement("itemlayout");
  }
  itemElement = layout->FirstChildElement("focusedlayout");
  while (itemElement)
  { // we have a new item layout
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, true);
    m_focusedLayouts.push_back(itemLayout);
    itemElement = itemElement->NextSiblingElement("focusedlayout");
  }
}

void CGUIBaseContainer::LoadContent(TiXmlElement *content)
{
  TiXmlElement *root = content->FirstChildElement("content");
  if (!root)
    return;

  g_SkinInfo.ResolveIncludes(root);

  m_staticContent = true;
  TiXmlElement *item = root->FirstChildElement("item");
  while (item)
  {
    // format:
    // <item label="Cool Video" label2="" thumb="q:\userdata\thumbnails\video\04385918.tbn">PlayMedia(c:\videos\cool_video.avi)</item>
    // <item label="My Album" label2="" thumb="q:\userdata\thumbnails\music\0\04385918.tbn">ActivateWindow(MyMusic,c:\music\my album)</item>
    // <item label="Apple Movie Trailers" label2="Bob" thumb="q:\userdata\thumbnails\programs\04385918.tbn">RunScript(q:\scripts\apple movie trailers\default.py)</item>

    // OR the more verbose, but includes-friendly:
    // <item>
    //   <label>blah</label>
    //   <label2>foo</label2>
    //   <thumb>bar.png</thumb>
    //   <icon>foo.jpg</icon>
    //   <onclick>ActivateWindow(Home)</onclick>
    // </item>
    g_SkinInfo.ResolveIncludes(item);
    if (item->FirstChild())
    {
      CFileItemPtr newItem;
      // check whether we're using the more verbose method...
      TiXmlNode *click = item->FirstChild("onclick");
      if (click && click->FirstChild())
      {
        CStdString label, label2, thumb, icon;
        XMLUtils::GetString(item, "label", label);
        XMLUtils::GetString(item, "label2", label2);
        XMLUtils::GetString(item, "thumb", thumb);
        XMLUtils::GetString(item, "icon", icon);
        const char *id = item->Attribute("id");
        int visibleCondition = 0;
        CGUIControlFactory::GetConditionalVisibility(item, visibleCondition);
        newItem.reset(new CFileItem(CGUIControlFactory::FilterLabel(label)));
        // multiple action strings are concat'd together, separated with " , "
        vector<CStdString> actions;
        CGUIControlFactory::GetMultipleString(item, "onclick", actions);
        for (vector<CStdString>::iterator it = actions.begin(); it != actions.end(); ++it)
          (*it).Replace(",", ",,");
        StringUtils::JoinString(actions, " , ", newItem->m_strPath);
        newItem->SetLabel2(CGUIControlFactory::FilterLabel(label2));
        newItem->SetThumbnailImage(thumb);
        newItem->SetIconImage(icon);
        if (id) newItem->m_iprogramCount = atoi(id);
        newItem->m_idepth = visibleCondition;
      }
      else
      {
        const char *label = item->Attribute("label");
        const char *label2 = item->Attribute("label2");
        const char *thumb = item->Attribute("thumb");
        const char *icon = item->Attribute("icon");
        const char *id = item->Attribute("id");
        newItem.reset(new CFileItem(label ? CGUIControlFactory::FilterLabel(label) : ""));
        newItem->m_strPath = item->FirstChild()->Value();
        if (label2) newItem->SetLabel2(CGUIControlFactory::FilterLabel(label2));
        if (thumb) newItem->SetThumbnailImage(thumb);
        if (icon) newItem->SetIconImage(icon);
        if (id) newItem->m_iprogramCount = atoi(id);
        newItem->m_idepth = 0;  // no visibility condition
      }
      m_staticItems.push_back(newItem);
    }
    item = item->NextSiblingElement("item");
  }
  // and make sure m_items is setup initially as well, so that initial item selection works as expected
  UpdateVisibility();
  return;
}

void CGUIBaseContainer::SetType(VIEW_TYPE type, const CStdString &label)
{
  m_type = type;
  m_label = label;
}

void CGUIBaseContainer::MoveToItem(int item)
{
  g_infoManager.SetContainerMoving(GetID(), item - m_cursor);
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

bool CGUIBaseContainer::InsideLayout(const CGUIListItemLayout *layout, const CPoint &point)
{
  if (!layout) return false;
  if ((m_orientation == VERTICAL && layout->Size(HORIZONTAL) && point.x > layout->Size(HORIZONTAL)) ||
      (m_orientation == HORIZONTAL && layout->Size(VERTICAL) && point.y > layout->Size(VERTICAL)))
    return false;
  return true;
}

#ifdef _DEBUG
void CGUIBaseContainer::DumpTextureUse()
{
  CLog::Log(LOGDEBUG, "%s for container %u", __FUNCTION__, GetID());
  for (unsigned int i = 0; i < m_items.size(); ++i)
  {
    CGUIListItemPtr item = m_items[i];
    if (item->GetFocusedLayout()) item->GetFocusedLayout()->DumpTextureUse();
    if (item->GetLayout()) item->GetLayout()->DumpTextureUse();
  }
}
#endif

bool CGUIBaseContainer::GetCondition(int condition, int data) const
{
  switch (condition)
  {
  case CONTAINER_ROW:
    return (m_orientation == VERTICAL) ? (m_cursor == data) : true;
  case CONTAINER_COLUMN:
    return (m_orientation == HORIZONTAL) ? (m_cursor == data) : true;
  case CONTAINER_POSITION:
    return (m_cursor == data);
  case CONTAINER_HAS_NEXT:
    return (HasNextPage());
  case CONTAINER_HAS_PREVIOUS:
    return (HasPreviousPage());
  case CONTAINER_SUBITEM:
    {
      CGUIListItemLayout *layout = GetFocusedLayout();
      return layout ? (layout->GetFocusedItem() == (unsigned int)data) : false;
    }
  case CONTAINER_SCROLLING:
    return (m_scrollTimer.GetElapsedMilliseconds() > m_scrollTime || m_pageChangeTimer.IsRunning());
  default:
    return false;
  }
}

void CGUIBaseContainer::GetCurrentLayouts()
{
  m_layout = NULL;
  for (unsigned int i = 0; i < m_layouts.size(); i++)
  {
    int condition = m_layouts[i].GetCondition();
    if (!condition || g_infoManager.GetBool(condition, GetParentID()))
    {
      m_layout = &m_layouts[i];
      break;
    }
  }
  if (!m_layout && m_layouts.size())
    m_layout = &m_layouts[0];  // failsafe

  m_focusedLayout = NULL;
  for (unsigned int i = 0; i < m_focusedLayouts.size(); i++)
  {
    int condition = m_focusedLayouts[i].GetCondition();
    if (!condition || g_infoManager.GetBool(condition, GetParentID()))
    {
      m_focusedLayout = &m_focusedLayouts[i];
      break;
    }
  }
  if (!m_focusedLayout && m_focusedLayouts.size())
    m_focusedLayout = &m_focusedLayouts[0];  // failsafe
}

bool CGUIBaseContainer::HasNextPage() const
{
  return false;
}

bool CGUIBaseContainer::HasPreviousPage() const
{
  return false;
}

CStdString CGUIBaseContainer::GetLabel(int info) const
{
  CStdString label;
  switch (info)
  {
  case CONTAINER_NUM_PAGES:
    label.Format("%u", (GetRows() + m_itemsPerPage - 1) / m_itemsPerPage);
    break;
  case CONTAINER_CURRENT_PAGE:
    label.Format("%u", GetCurrentPage());
    break;
  case CONTAINER_POSITION:
    label.Format("%i", m_cursor);
    break;
  case CONTAINER_NUM_ITEMS:
    {
      unsigned int numItems = GetNumItems();
      if (numItems && m_items[0]->IsFileItem() && (boost::static_pointer_cast<CFileItem>(m_items[0]))->IsParentFolder())
        label.Format("%u", numItems-1);
      else
        label.Format("%u", numItems);
    }
    break;
  default:
      break;
  }
  return label;
}

int CGUIBaseContainer::GetCurrentPage() const
{
  if (m_offset + m_itemsPerPage >= (int)GetRows())  // last page
    return (GetRows() + m_itemsPerPage - 1) / m_itemsPerPage;
  return m_offset / m_itemsPerPage + 1;
}


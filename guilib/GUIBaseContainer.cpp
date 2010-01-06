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

#include "GUIBaseContainer.h"
#include "GUIControlFactory.h"
#include "GUIWindowManager.h"
#include "utils/CharsetConverter.h"
#include "utils/GUIInfoManager.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "XMLUtils.h"
#include "SkinInfo.h"
#include "StringUtils.h"
#include "FileItem.h"
#include "Key.h"

using namespace std;

#define HOLD_TIME_START 100
#define HOLD_TIME_END   3000

CGUIBaseContainer::CGUIBaseContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime, int preloadItems)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
{
  m_cursor = 0;
  m_offset = 0;
  m_scrollOffset = 0;
  m_scrollSpeed = 0;
  m_scrollLastTime = 0;
  m_scrollTime = scrollTime ? scrollTime : 1;
  m_lastHoldTime = 0;
  m_itemsPerPage = 10;
  m_pageControl = 0;
  m_renderTime = 0;
  m_orientation = orientation;
  m_analogScrollCount = 0;
  m_lastItem = NULL;
  m_staticContent = false;
  m_staticUpdateTime = 0;
  m_wasReset = false;
  m_layout = NULL;
  m_focusedLayout = NULL;
  m_cacheItems = preloadItems;
}

CGUIBaseContainer::~CGUIBaseContainer(void)
{
}

void CGUIBaseContainer::Render()
{
  ValidateOffset();

  if (m_bInvalidated)
    UpdateLayout();

  if (!m_layout || !m_focusedLayout) return;

  UpdateScrollOffset();

  int offset = (int)floorf(m_scrollOffset / m_layout->Size(m_orientation));

  int cacheBefore, cacheAfter;
  GetCacheOffsets(cacheBefore, cacheAfter);

  // Free memory not used on screen
  if ((int)m_items.size() > m_itemsPerPage + cacheBefore + cacheAfter)
    FreeMemory(CorrectOffset(offset - cacheBefore, 0), CorrectOffset(offset + m_itemsPerPage + 1 + cacheAfter, 0));

  g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height);
  float pos = (m_orientation == VERTICAL) ? m_posY : m_posX;
  float end = (m_orientation == VERTICAL) ? m_posY + m_height : m_posX + m_width;

  // we offset our draw position to take into account scrolling and whether or not our focused
  // item is offscreen "above" the list.
  float drawOffset = (offset - cacheBefore) * m_layout->Size(m_orientation) - m_scrollOffset;
  if (m_offset + m_cursor < offset)
    drawOffset += m_focusedLayout->Size(m_orientation) - m_layout->Size(m_orientation);
  pos += drawOffset;
  end += cacheAfter * m_layout->Size(m_orientation);

  float focusedPos = 0;
  CGUIListItemPtr focusedItem;
  int current = offset - cacheBefore;
  while (pos < end && m_items.size())
  {
    int itemNo = CorrectOffset(current, 0);
    if (itemNo >= (int)m_items.size())
      break;
    bool focused = (current == m_offset + m_cursor);
    if (itemNo >= 0)
    {
      CGUIListItemPtr item = m_items[itemNo];
      // render our item
      if (focused)
      {
        focusedPos = pos;
        focusedItem = item;
      }
      else
      {
        if (m_orientation == VERTICAL)
          RenderItem(m_posX, pos, item.get(), false);
        else
          RenderItem(pos, m_posY, item.get(), false);
      }
    }
    // increment our position
    pos += focused ? m_focusedLayout->Size(m_orientation) : m_layout->Size(m_orientation);
    current++;
  }
  // render focused item last so it can overlap other items
  if (focusedItem)
  {
    if (m_orientation == VERTICAL)
      RenderItem(m_posX, focusedPos, focusedItem.get(), true);
    else
      RenderItem(focusedPos, m_posY, focusedItem.get(), true);
  }

  g_graphicsContext.RestoreClipRegion();

  UpdatePageControl(offset);

  CGUIControl::Render();
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
      item->GetFocusedLayout()->Render(item, m_parentID, m_renderTime);
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
      item->GetFocusedLayout()->Render(item, m_parentID, m_renderTime);
    else if (item->GetLayout())
      item->GetLayout()->Render(item, m_parentID, m_renderTime);
  }
  g_graphicsContext.RestoreOrigin();
}

bool CGUIBaseContainer::OnAction(const CAction &action)
{
  if (action.id >= KEY_ASCII)
  {
    OnJumpLetter((char)(action.id & 0xff));
    return true;
  }

  switch (action.id)
  {
  case ACTION_MOVE_LEFT:
  case ACTION_MOVE_RIGHT:
  case ACTION_MOVE_DOWN:
  case ACTION_MOVE_UP:
    {
      if (!HasFocus()) return false;
      if (action.holdTime > HOLD_TIME_START &&
        ((m_orientation == VERTICAL && (action.id == ACTION_MOVE_UP || action.id == ACTION_MOVE_DOWN)) ||
         (m_orientation == HORIZONTAL && (action.id == ACTION_MOVE_LEFT || action.id == ACTION_MOVE_RIGHT))))
      { // action is held down - repeat a number of times
        float speed = std::min(1.0f, (float)(action.holdTime - HOLD_TIME_START) / (HOLD_TIME_END - HOLD_TIME_START));
        unsigned int itemsPerFrame = 1;
        if (m_lastHoldTime) // number of rows/10 items/second max speed
          itemsPerFrame = std::max((unsigned int)1, (unsigned int)(speed * 0.0001f * GetRows() * (CTimeUtils::GetFrameTime() - m_lastHoldTime)));
        m_lastHoldTime = CTimeUtils::GetFrameTime();
        if (action.id == ACTION_MOVE_LEFT || action.id == ACTION_MOVE_UP)
          while (itemsPerFrame--) MoveUp(false);
        else
          while (itemsPerFrame--) MoveDown(false);
        return true;
      }
      else
      {
        m_lastHoldTime = 0;
        return CGUIControl::OnAction(action);
      }
    }
    break;

  case ACTION_FIRST_PAGE:
    SelectItem(0);
    return true;

  case ACTION_LAST_PAGE:
    if (m_items.size())
      SelectItem(m_items.size() - 1);
    return true;

  case ACTION_NEXT_LETTER:
    {
      OnNextLetter();
      return true;
    }
    break;
  case ACTION_PREV_LETTER:
    {
      OnPrevLetter();
      return true;
    }
    break;
  case ACTION_JUMP_SMS2:
  case ACTION_JUMP_SMS3:
  case ACTION_JUMP_SMS4:
  case ACTION_JUMP_SMS5:
  case ACTION_JUMP_SMS6:
  case ACTION_JUMP_SMS7:
  case ACTION_JUMP_SMS8:
  case ACTION_JUMP_SMS9:
    {
      OnJumpSMS(action.id - ACTION_JUMP_SMS2 + 2);
      return true;
    }
    break;

  default:
    if (action.id)
    {
      return OnClick(action.id);
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
      if (message.GetMessage() == GUI_MSG_LABEL_BIND && message.GetPointer())
      { // bind our items
        Reset();
        CFileItemList *items = (CFileItemList *)message.GetPointer();
        for (int i = 0; i < items->Size(); i++)
          m_items.push_back(items->Get(i));
        UpdateLayout(true); // true to refresh all items
        UpdateScrollByLetter();
        SelectItem(message.GetParam1());
        return true;
      }
      else if (message.GetMessage() == GUI_MSG_LABEL_RESET)
      {
        Reset();
        SetPageControlRange();
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
  bool wrapAround = m_controlUp == GetID() || !(m_controlUp || m_upActions.size());
  if (m_orientation == VERTICAL && MoveUp(wrapAround))
    return;
  // with horizontal lists it doesn't make much sense to have multiselect labels
  CGUIControl::OnUp();
}

void CGUIBaseContainer::OnDown()
{
  bool wrapAround = m_controlDown == GetID() || !(m_controlDown || m_downActions.size());
  if (m_orientation == VERTICAL && MoveDown(wrapAround))
    return;
  // with horizontal lists it doesn't make much sense to have multiselect labels
  CGUIControl::OnDown();
}

void CGUIBaseContainer::OnLeft()
{
  bool wrapAround = m_controlLeft == GetID() || !(m_controlLeft || m_leftActions.size());
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
  bool wrapAround = m_controlRight == GetID() || !(m_controlRight || m_rightActions.size());
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
  for (int i = (int)m_letterOffsets.size() - 1; i >= 0; i--)
  {
    if (m_letterOffsets[i].first < offset)
    {
      SelectItem(m_letterOffsets[i].first);
      return;
    }
  }
}

void CGUIBaseContainer::OnJumpLetter(char letter)
{
  if (m_matchTimer.GetElapsedMilliseconds() < letter_match_timeout)
    m_match.push_back(letter);
  else
    m_match.Format("%c", letter);

  m_matchTimer.StartZero();

  // we can't jump through letters if we have none
  if (0 == m_letterOffsets.size())
    return;

  // find the current letter we're focused on
  unsigned int offset = CorrectOffset(m_offset, m_cursor);
  for (unsigned int i = (offset + 1) % m_items.size(); i != offset; i = (i+1) % m_items.size())
  {
    CGUIListItemPtr item = m_items[i];
    if (0 == strnicmp(item->GetSortLabel().c_str(), m_match.c_str(), m_match.size()))
    {
      SelectItem(i);
      return;
    }
  }
  // no match found - repeat with a single letter
  if (m_match.size() > 1)
  {
    m_match.clear();
    OnJumpLetter(letter);
  }
}

void CGUIBaseContainer::OnJumpSMS(int letter)
{
  static const char letterMap[8][6] = { "ABC2", "DEF3", "GHI4", "JKL5", "MNO6", "PQRS7", "TUV8", "WXYZ9" };

  // only 2..9 supported
  if (letter < 2 || letter > 9 || !m_letterOffsets.size())
    return;

  const CStdString letters = letterMap[letter - 2];
  // find where we currently are
  int offset = CorrectOffset(m_offset, m_cursor);
  unsigned int currentLetter = 0;
  while (currentLetter + 1 < m_letterOffsets.size() && m_letterOffsets[currentLetter + 1].first <= offset)
    currentLetter++;

  // now switch to the next letter
  CStdString current = m_letterOffsets[currentLetter].second;
  int startPos = (letters.Find(current) + 1) % letters.size();
  // now jump to letters[startPos], or another one in the same range if possible
  int pos = startPos;
  while (true)
  {
    // check if we can jump to this letter
    for (unsigned int i = 0; i < m_letterOffsets.size(); i++)
    {
      if (m_letterOffsets[i].second == letters.Mid(pos, 1))
      {
        SelectItem(m_letterOffsets[i].first);
        return;
      }
    }
    pos = (pos + 1) % letters.size();
    if (pos == startPos)
      return;
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

bool CGUIBaseContainer::OnMouseOver(const CPoint &point)
{
  // select the item under the pointer
  SelectItemFromPoint(point - CPoint(m_posX, m_posY));
  return CGUIControl::OnMouseOver(point);
}

bool CGUIBaseContainer::OnMouseClick(int button, const CPoint &point)
{
  if (SelectItemFromPoint(point - CPoint(m_posX, m_posY)))
  { // send click message to window
    OnClick(ACTION_MOUSE_CLICK + button);
    return true;
  }
  return false;
}

bool CGUIBaseContainer::OnMouseDoubleClick(int button, const CPoint &point)
{
  if (SelectItemFromPoint(point - CPoint(m_posX, m_posY)))
  { // send double click message to window
    OnClick(ACTION_MOUSE_DOUBLE_CLICK + button);
    return true;
  }
  return false;
}

bool CGUIBaseContainer::OnClick(int actionID)
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
          g_windowManager.SendMessage(message);
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

void CGUIBaseContainer::SetPageControl(int id)
{
  m_pageControl = id;
}

void CGUIBaseContainer::ValidateOffset()
{
}

void CGUIBaseContainer::DoRender(unsigned int currentTime)
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
  SetPageControlRange();
}

void CGUIBaseContainer::SetPageControlRange()
{
  if (m_pageControl)
  {
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, GetRows());
    SendWindowMessage(msg);
  }
}

void CGUIBaseContainer::UpdatePageControl(int offset)
{
  if (m_pageControl)
  { // tell our pagecontrol (scrollbar or whatever) to update (offset it by our cursor position)
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, offset);
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
    bool updateItems = false;
    if (!m_staticUpdateTime)
      m_staticUpdateTime = CTimeUtils::GetFrameTime();
    if (CTimeUtils::GetFrameTime() - m_staticUpdateTime > 1000)
    {
      m_staticUpdateTime = CTimeUtils::GetFrameTime();
      updateItems = true;
    }
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
      if (updateItems && item->HasProperties()) 
      { // has info, so update it
        CStdString info = item->GetProperty("label");
        if (!info.IsEmpty()) item->SetLabel(CGUIInfoLabel::GetLabel(info));
        info = item->GetProperty("label2");
        if (!info.IsEmpty()) item->SetLabel2(CGUIInfoLabel::GetLabel(info));
        info = item->GetProperty("icon");
        if (!info.IsEmpty()) item->SetIconImage(CGUIInfoLabel::GetLabel(info, true));
        info = item->GetProperty("thumb");
        if (!info.IsEmpty()) item->SetThumbnailImage(CGUIInfoLabel::GetLabel(info, true));
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
  if (!m_focusedLayout || !m_layout)
    return;

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
    // The letter offset jumping is only for ASCII characters at present, and
    // our checks are all done in uppercase
    CStdString nextLetter = item->GetSortLabel().Left(1);
    nextLetter.ToUpper();
    if (currentMatch != nextLetter)
    {
      currentMatch = nextLetter;
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
  float size = (m_layout) ? m_layout->Size(m_orientation) : 10.0f;
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

  vector<CGUIListItemPtr> items;
  TiXmlElement *item = root->FirstChildElement("item");
  while (item)
  {
    // format:
    // <item label="Cool Video" label2="" thumb="mythumb.png">PlayMedia(c:\videos\cool_video.avi)</item>
    // <item label="My Album" label2="" thumb="whatever.jpg">ActivateWindow(MyMusic,c:\music\my album)</item>
    // <item label="Apple Movie Trailers" label2="Bob" thumb="foo.tbn">RunScript(special://xbmc/scripts/apple movie trailers/default.py)</item>

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
        XMLUtils::GetString(item, "label", label);   label  = CGUIControlFactory::FilterLabel(label);
        XMLUtils::GetString(item, "label2", label2); label2 = CGUIControlFactory::FilterLabel(label2);
        XMLUtils::GetString(item, "thumb", thumb);   thumb  = CGUIControlFactory::FilterLabel(thumb);
        XMLUtils::GetString(item, "icon", icon);     icon   = CGUIControlFactory::FilterLabel(icon);
        const char *id = item->Attribute("id");
        int visibleCondition = 0;
        CGUIControlFactory::GetConditionalVisibility(item, visibleCondition);
        newItem.reset(new CFileItem(CGUIInfoLabel::GetLabel(label)));
        // multiple action strings are concat'd together, separated with " , "
        vector<CGUIActionDescriptor> actions;
        CGUIControlFactory::GetMultipleString(item, "onclick", actions);
        newItem->m_strPath = "";
        for (vector<CGUIActionDescriptor>::iterator it = actions.begin(); it != actions.end(); ++it)
        {
          (*it).m_action.Replace(",", ",,");
          if (newItem->m_strPath.length() > 0)
          {
            newItem->m_strPath   += " , ";
          }
          newItem->m_strPath += (*it).m_action;          
        }
        newItem->SetLabel2(CGUIInfoLabel::GetLabel(label2));
        newItem->SetThumbnailImage(CGUIInfoLabel::GetLabel(thumb, true));
        newItem->SetIconImage(CGUIInfoLabel::GetLabel(icon, true));
        if (label.Find("$INFO") >= 0) newItem->SetProperty("label", label);
        if (label2.Find("$INFO") >= 0) newItem->SetProperty("label2", label2);
        if (icon.Find("$INFO") >= 0) newItem->SetProperty("icon", icon);
        if (thumb.Find("$INFO") >= 0) newItem->SetProperty("thumb", thumb);
        if (id) newItem->m_iprogramCount = atoi(id);
        newItem->m_idepth = visibleCondition;
      }
      else
      {
        CStdString label, label2, thumb, icon;
        label  = item->Attribute("label");  label  = CGUIControlFactory::FilterLabel(label);
        label2 = item->Attribute("label2"); label2 = CGUIControlFactory::FilterLabel(label2);
        thumb  = item->Attribute("thumb");  thumb  = CGUIControlFactory::FilterLabel(thumb);
        icon   = item->Attribute("icon");   icon   = CGUIControlFactory::FilterLabel(icon);
        const char *id = item->Attribute("id");
        newItem.reset(new CFileItem(CGUIInfoLabel::GetLabel(label)));
        newItem->m_strPath = item->FirstChild()->Value();
        newItem->SetLabel2(CGUIInfoLabel::GetLabel(label2));
        newItem->SetThumbnailImage(CGUIInfoLabel::GetLabel(thumb, true));
        newItem->SetIconImage(CGUIInfoLabel::GetLabel(icon, true));
        if (id) newItem->m_iprogramCount = atoi(id);
        newItem->m_idepth = 0;  // no visibility condition
      }
      items.push_back(newItem);
    }
    item = item->NextSiblingElement("item");
  }
  SetStaticContent(items);
}

void CGUIBaseContainer::SetStaticContent(const vector<CGUIListItemPtr> &items)
{
  m_staticContent = true;
  m_staticUpdateTime = 0;
  m_staticItems.clear();
  m_staticItems.assign(items.begin(), items.end());
  UpdateVisibility();
}

void CGUIBaseContainer::SetType(VIEW_TYPE type, const CStdString &label)
{
  m_type = type;
  m_label = label;
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

void CGUIBaseContainer::GetCacheOffsets(int &cacheBefore, int &cacheAfter)
{
  if (m_scrollSpeed > 0)
  {
    cacheBefore = 0;
    cacheAfter = m_cacheItems;
  }
  else if (m_scrollSpeed < 0)
  {
    cacheBefore = m_cacheItems;
    cacheAfter = 0;
  }
  else
  {
    cacheBefore = m_cacheItems / 2;
    cacheAfter = m_cacheItems / 2;
  }
}

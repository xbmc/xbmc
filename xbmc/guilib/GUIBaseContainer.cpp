/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "GUIBaseContainer.h"
#include "GUIControlFactory.h"
#include "GUIWindowManager.h"
#include "utils/CharsetConverter.h"
#include "GUIInfoManager.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "GUIStaticItem.h"
#include "Key.h"
#include "utils/MathUtils.h"
#include "utils/XBMCTinyXML.h"

using namespace std;

#define HOLD_TIME_START 100
#define HOLD_TIME_END   3000
#define SCROLLING_GAP   200U
#define SCROLLING_THRESHOLD 300U

CGUIBaseContainer::CGUIBaseContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
    , m_scroller(scroller)
{
  m_cursor = 0;
  m_offset = 0;
  m_lastHoldTime = 0;
  m_itemsPerPage = 10;
  m_pageControl = 0;
  m_orientation = orientation;
  m_analogScrollCount = 0;
  m_staticContent = false;
  m_staticUpdateTime = 0;
  m_staticDefaultItem = -1;
  m_staticDefaultAlways = false;
  m_wasReset = false;
  m_layout = NULL;
  m_focusedLayout = NULL;
  m_cacheItems = preloadItems;
  m_scrollItemsPerFrame = 0.0f;
  m_type = VIEW_TYPE_NONE;
}

CGUIBaseContainer::~CGUIBaseContainer(void)
{
}

void CGUIBaseContainer::DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  CGUIControl::DoProcess(currentTime, dirtyregions);

  if (m_pageChangeTimer.GetElapsedMilliseconds() > 200)
    m_pageChangeTimer.Stop();
  m_wasReset = false;
}

void CGUIBaseContainer::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  ValidateOffset();

  if (m_bInvalidated)
    UpdateLayout();

  if (!m_layout || !m_focusedLayout) return;

  UpdateScrollOffset(currentTime);

  int offset = (int)floorf(m_scroller.GetValue() / m_layout->Size(m_orientation));

  int cacheBefore, cacheAfter;
  GetCacheOffsets(cacheBefore, cacheAfter);

  // Free memory not used on screen
  if ((int)m_items.size() > m_itemsPerPage + cacheBefore + cacheAfter)
    FreeMemory(CorrectOffset(offset - cacheBefore, 0), CorrectOffset(offset + m_itemsPerPage + 1 + cacheAfter, 0));

  CPoint origin = CPoint(m_posX, m_posY) + m_renderOffset;
  float pos = (m_orientation == VERTICAL) ? origin.y : origin.x;
  float end = (m_orientation == VERTICAL) ? m_posY + m_height : m_posX + m_width;

  // we offset our draw position to take into account scrolling and whether or not our focused
  // item is offscreen "above" the list.
  float drawOffset = (offset - cacheBefore) * m_layout->Size(m_orientation) - m_scroller.GetValue();
  if (GetOffset() + GetCursor() < offset)
    drawOffset += m_focusedLayout->Size(m_orientation) - m_layout->Size(m_orientation);
  pos += drawOffset;
  end += cacheAfter * m_layout->Size(m_orientation);

  int current = offset - cacheBefore;
  while (pos < end && m_items.size())
  {
    int itemNo = CorrectOffset(current, 0);
    if (itemNo >= (int)m_items.size())
      break;
    bool focused = (current == GetOffset() + GetCursor());
    if (itemNo >= 0)
    {
      CGUIListItemPtr item = m_items[itemNo];
      // render our item
      if (m_orientation == VERTICAL)
        ProcessItem(origin.x, pos, item, focused, currentTime, dirtyregions);
      else
        ProcessItem(pos, origin.y, item, focused, currentTime, dirtyregions);
    }
    // increment our position
    pos += focused ? m_focusedLayout->Size(m_orientation) : m_layout->Size(m_orientation);
    current++;
  }

  UpdatePageControl(offset);

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIBaseContainer::ProcessItem(float posX, float posY, CGUIListItemPtr& item, bool focused, unsigned int currentTime, CDirtyRegionList &dirtyregions)
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
      item->GetFocusedLayout()->Process(item.get(), m_parentID, currentTime, dirtyregions);
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
    if (item->GetFocusedLayout())
      item->GetFocusedLayout()->Process(item.get(), m_parentID, currentTime, dirtyregions);
    if (item->GetLayout())
      item->GetLayout()->Process(item.get(), m_parentID, currentTime, dirtyregions);
  }

  g_graphicsContext.RestoreOrigin();
}

void CGUIBaseContainer::Render()
{
  if (!m_layout || !m_focusedLayout) return;

  int offset = (int)floorf(m_scroller.GetValue() / m_layout->Size(m_orientation));

  int cacheBefore, cacheAfter;
  GetCacheOffsets(cacheBefore, cacheAfter);

  if (g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height))
  {
    CPoint origin = CPoint(m_posX, m_posY) + m_renderOffset;
    float pos = (m_orientation == VERTICAL) ? origin.y : origin.x;
    float end = (m_orientation == VERTICAL) ? m_posY + m_height : m_posX + m_width;

    // we offset our draw position to take into account scrolling and whether or not our focused
    // item is offscreen "above" the list.
    float drawOffset = (offset - cacheBefore) * m_layout->Size(m_orientation) - m_scroller.GetValue();
    if (GetOffset() + GetCursor() < offset)
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
      bool focused = (current == GetOffset() + GetCursor());
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
            RenderItem(origin.x, pos, item.get(), false);
          else
            RenderItem(pos, origin.y, item.get(), false);
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
        RenderItem(origin.x, focusedPos, focusedItem.get(), true);
      else
        RenderItem(focusedPos, origin.y, focusedItem.get(), true);
    }

    g_graphicsContext.RestoreClipRegion();
  }

  UpdatePageControl(offset);

  CGUIControl::Render();
}


void CGUIBaseContainer::RenderItem(float posX, float posY, CGUIListItem *item, bool focused)
{
  if (!m_focusedLayout || !m_layout) return;

  // set the origin
  g_graphicsContext.SetOrigin(posX, posY);

  if (focused)
  {
    if (item->GetFocusedLayout())
      item->GetFocusedLayout()->Render(item, m_parentID);
  }
  else
  {
    if (item->GetFocusedLayout() && item->GetFocusedLayout()->IsAnimating(ANIM_TYPE_UNFOCUS))
      item->GetFocusedLayout()->Render(item, m_parentID);
    else if (item->GetLayout())
      item->GetLayout()->Render(item, m_parentID);
  }
  g_graphicsContext.RestoreOrigin();
}

bool CGUIBaseContainer::OnAction(const CAction &action)
{
  if (action.GetID() >= KEY_ASCII)
  {
    OnJumpLetter((char)(action.GetID() & 0xff));
    return true;
  }

  switch (action.GetID())
  {
  case ACTION_MOVE_LEFT:
  case ACTION_MOVE_RIGHT:
  case ACTION_MOVE_DOWN:
  case ACTION_MOVE_UP:
  case ACTION_NAV_BACK:
    {
      if (!HasFocus()) return false;

      if (action.GetHoldTime() > HOLD_TIME_START &&
        ((m_orientation == VERTICAL && (action.GetID() == ACTION_MOVE_UP || action.GetID() == ACTION_MOVE_DOWN)) ||
         (m_orientation == HORIZONTAL && (action.GetID() == ACTION_MOVE_LEFT || action.GetID() == ACTION_MOVE_RIGHT))))
      { // action is held down - repeat a number of times
        float speed = std::min(1.0f, (float)(action.GetHoldTime() - HOLD_TIME_START) / (HOLD_TIME_END - HOLD_TIME_START));
        unsigned int frameDuration = std::min(CTimeUtils::GetFrameTime() - m_lastHoldTime, 50u); // max 20fps

        // scrollrate is minimum 10 items/sec and timed to take around 10 seconds
        // with ramp up (num_rows/7 items per second max speed) to traverse a long list
        m_scrollItemsPerFrame += std::max(0.01f*(float)frameDuration, (float)(speed * 0.00015f * GetRows() * frameDuration));
        m_lastHoldTime = CTimeUtils::GetFrameTime();

        if(m_scrollItemsPerFrame < 1.0f)//not enough hold time accumulated for one step
          return false;

        while (m_scrollItemsPerFrame >= 1)
        {
          if (action.GetID() == ACTION_MOVE_LEFT || action.GetID() == ACTION_MOVE_UP)
            MoveUp(false);
          else
            MoveDown(false);
          m_scrollItemsPerFrame--;
        }
        return true;
      }
      else
      {
        //if HOLD_TIME_START is reached we need
        //a sane initial value for calculating m_scrollItemsPerPage
        m_lastHoldTime = CTimeUtils::GetFrameTime();
        m_scrollItemsPerFrame = 0.0f;
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
      OnJumpSMS(action.GetID() - ACTION_JUMP_SMS2 + 2);
      return true;
    }
    break;

  default:
    if (action.GetID())
    {
      return OnClick(action.GetID());
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
    if (message.GetMessage() == GUI_MSG_ITEM_SELECT)
    {
      SelectItem(message.GetParam1());
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_ITEM_SELECTED)
    {
      message.SetParam1(GetSelectedItem());
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_PAGE_CHANGE)
    {
      if (message.GetSenderId() == m_pageControl && IsVisible())
      { // update our page if we're visible - not much point otherwise
        if ((int)message.GetParam1() != GetOffset())
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
  bool wrapAround = m_actionUp.GetNavigation() == GetID() || !m_actionUp.HasActionsMeetingCondition();
  if (m_orientation == VERTICAL && MoveUp(wrapAround))
    return;
  // with horizontal lists it doesn't make much sense to have multiselect labels
  CGUIControl::OnUp();
}

void CGUIBaseContainer::OnDown()
{
  bool wrapAround = m_actionDown.GetNavigation() == GetID() || !m_actionDown.HasActionsMeetingCondition();
  if (m_orientation == VERTICAL && MoveDown(wrapAround))
    return;
  // with horizontal lists it doesn't make much sense to have multiselect labels
  CGUIControl::OnDown();
}

void CGUIBaseContainer::OnLeft()
{
  bool wrapAround = m_actionLeft.GetNavigation() == GetID() || !m_actionLeft.HasActionsMeetingCondition();
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
  bool wrapAround = m_actionRight.GetNavigation() == GetID() || !m_actionRight.HasActionsMeetingCondition();
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
  int offset = CorrectOffset(GetOffset(), GetCursor());
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
  int offset = CorrectOffset(GetOffset(), GetCursor());
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
  unsigned int offset = CorrectOffset(GetOffset(), GetCursor()) - 1;
  for (unsigned int i = (offset + 1) % m_items.size(); i != offset; i = (i+1) % m_items.size())
  {
    CGUIListItemPtr item = m_items[i];
    if (0 == strnicmp(SortUtils::RemoveArticles(item->GetLabel()).c_str(), m_match.c_str(), m_match.size()))
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
  int offset = CorrectOffset(GetOffset(), GetCursor());
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
  ScrollToOffset(GetOffset() + amount);
}

int CGUIBaseContainer::GetSelectedItem() const
{
  return CorrectOffset(GetOffset(), GetCursor());
}

CGUIListItemPtr CGUIBaseContainer::GetListItem(int offset, unsigned int flag) const
{
  if (!m_items.size())
    return CGUIListItemPtr();
  int item = GetSelectedItem() + offset;
  if (flag & INFOFLAG_LISTITEM_POSITION) // use offset from the first item displayed, taking into account scrolling
    item = CorrectOffset((int)(m_scroller.GetValue() / m_layout->Size(m_orientation)), offset);

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

EVENT_RESULT CGUIBaseContainer::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id >= ACTION_MOUSE_LEFT_CLICK && event.m_id <= ACTION_MOUSE_DOUBLE_CLICK)
  {
    if (SelectItemFromPoint(point - CPoint(m_posX, m_posY)))
    {
      OnClick(event.m_id);
      return EVENT_RESULT_HANDLED;
    }
  }
  else if (event.m_id == ACTION_MOUSE_WHEEL_UP)
  {
    Scroll(-1);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_MOUSE_WHEEL_DOWN)
  {
    Scroll(1);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_GESTURE_NOTIFY)
  {
    return (m_orientation == HORIZONTAL) ? EVENT_RESULT_PAN_HORIZONTAL : EVENT_RESULT_PAN_VERTICAL;
  }
  else if (event.m_id == ACTION_GESTURE_BEGIN)
  { // grab exclusive access
    CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, GetID(), GetParentID());
    SendWindowMessage(msg);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_GESTURE_PAN)
  { // do the drag and validate our offset (corrects for end of scroll)
    m_scroller.SetValue(m_scroller.GetValue() - ((m_orientation == HORIZONTAL) ? event.m_offsetX : event.m_offsetY));
    float size = (m_layout) ? m_layout->Size(m_orientation) : 10.0f;
    int offset = (int)MathUtils::round_int(m_scroller.GetValue() / size);
    m_lastScrollStartTimer.Stop();
    m_scrollTimer.Start();
    SetOffset(offset);
    ValidateOffset();
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_GESTURE_END)
  { // release exclusive access
    CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, 0, GetParentID());
    SendWindowMessage(msg);
    m_scrollTimer.Stop();
    // and compute the nearest offset from this and scroll there
    float size = (m_layout) ? m_layout->Size(m_orientation) : 10.0f;
    float offset = m_scroller.GetValue() / size;
    int toOffset = (int)MathUtils::round_int(offset);
    if (toOffset < offset)
      SetOffset(toOffset+1);
    else
      SetOffset(toOffset-1);
    ScrollToOffset(toOffset);
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
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
        CGUIStaticItemPtr item = boost::static_pointer_cast<CGUIStaticItem>(m_items[selected]);
        item->GetClickActions().ExecuteActions(GetID(), GetParentID());
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
    m_lastItem.reset();
  }
  CGUIControl::SetFocus(bOnOff);
}

void CGUIBaseContainer::SaveStates(vector<CControlState> &states)
{
  if (!m_staticDefaultAlways)
    states.push_back(CControlState(GetID(), GetSelectedItem()));
}

void CGUIBaseContainer::SetPageControl(int id)
{
  m_pageControl = id;
}

bool CGUIBaseContainer::GetOffsetRange(int &minOffset, int &maxOffset) const
{
  minOffset = 0;
  maxOffset = GetRows() - m_itemsPerPage;
  return true;
}

void CGUIBaseContainer::ValidateOffset()
{
}

void CGUIBaseContainer::AllocResources()
{
  CGUIControl::AllocResources();
  CalculateLayout();
  UpdateStaticItems(true);
  if (m_staticDefaultItem != -1) // select default item
    SelectStaticItemById(m_staticDefaultItem);
}

void CGUIBaseContainer::FreeResources(bool immediately)
{
  CGUIControl::FreeResources(immediately);
  if (m_staticContent)
  { // free any static content
    Reset();
  }
  m_scroller.Stop();
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
  MarkDirtyRegion();
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

  if (!IsVisible() && !CGUIControl::CanFocus())
    return; // no need to update the content if we're not visible and we can't focus

  // check whether we need to update our layouts
  if ((m_layout && !m_layout->CheckCondition()) ||
      (m_focusedLayout && !m_focusedLayout->CheckCondition()))
  {
    // and do it
    int itemIndex = GetSelectedItem();
    UpdateLayout(true); // true to refresh all items
    SelectItem(itemIndex);
  }

  UpdateStaticItems();
}

void CGUIBaseContainer::UpdateStaticItems(bool refreshItems)
{
  if (m_staticContent)
  { // update our item list with our new content, but only add those items that should
    // be visible.  Save the previous item and keep it if we are adding that one.
    std::vector<CGUIListItemPtr> items;
    int reselect = -1;
    int selected = GetSelectedItem();
    CGUIListItem* selectedItem = (selected >= 0 && (unsigned int)selected < m_items.size()) ? m_items[selected].get() : NULL;
    bool updateItemsProperties = false;
    if (!m_staticUpdateTime)
      m_staticUpdateTime = CTimeUtils::GetFrameTime();
    if (CTimeUtils::GetFrameTime() - m_staticUpdateTime > 1000)
    {
      m_staticUpdateTime = CTimeUtils::GetFrameTime();
      updateItemsProperties = true;
    }
    for (unsigned int i = 0; i < m_staticItems.size(); ++i)
    {
      CGUIStaticItemPtr staticItem = boost::static_pointer_cast<CGUIStaticItem>(m_staticItems[i]);
      if (staticItem->UpdateVisibility(GetParentID()))
        refreshItems = true;
      if (staticItem->IsVisible())
      {
        items.push_back(staticItem);
        // if item is selected and it changed position, re-select it
        if (staticItem.get() == selectedItem && selected != (int)items.size() - 1)
          reselect = items.size() - 1;
      }
      // update any properties
      if (updateItemsProperties)
        staticItem->UpdateProperties(GetParentID());
    }
    if (refreshItems)
    {
      Reset();
      m_items = items;
      SetPageControlRange();
      if (reselect >= 0 && reselect < (int)m_items.size())
        SelectItem(reselect);
      SetInvalid();
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

  m_itemsPerPage = std::max((int)((Size() - m_focusedLayout->Size(m_orientation)) / m_layout->Size(m_orientation)) + 1, 1);

  // ensure that the scroll offset is a multiple of our size
  m_scroller.SetValue(GetOffset() * m_layout->Size(m_orientation));
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
    CStdString nextLetter;
    g_charsetConverter.wToUTF8(item->GetSortLabel().Left(1).ToUpper(), nextLetter);
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

int CGUIBaseContainer::ScrollCorrectionRange() const
{
  int range = m_itemsPerPage / 4;
  if (range <= 0) range = 1;
  return range;
}

void CGUIBaseContainer::ScrollToOffset(int offset)
{
  int minOffset, maxOffset;
  if(GetOffsetRange(minOffset, maxOffset))
    offset = std::max(minOffset, std::min(offset, maxOffset));
  float size = (m_layout) ? m_layout->Size(m_orientation) : 10.0f;
  int range = ScrollCorrectionRange();
  if (offset * size < m_scroller.GetValue() &&  m_scroller.GetValue() - offset * size > size * range)
  { // scrolling up, and we're jumping more than 0.5 of a screen
    m_scroller.SetValue((offset + range) * size);
  }
  if (offset * size > m_scroller.GetValue() && offset * size - m_scroller.GetValue() > size * range)
  { // scrolling down, and we're jumping more than 0.5 of a screen
    m_scroller.SetValue((offset - range) * size);
  }
  m_scroller.ScrollTo(offset * size);
  m_lastScrollStartTimer.StartZero();
  if (!m_wasReset)
  {
    SetContainerMoving(offset - GetOffset());
    if (m_scroller.IsScrolling())
      m_scrollTimer.Start();
    else
      m_scrollTimer.Stop();
  }
  SetOffset(offset);
}

void CGUIBaseContainer::SetContainerMoving(int direction)
{
  if (direction)
    g_infoManager.SetContainerMoving(GetID(), direction > 0, m_scroller.IsScrolling());
}

void CGUIBaseContainer::UpdateScrollOffset(unsigned int currentTime)
{
  if (m_scroller.Update(currentTime))
    MarkDirtyRegion();
  else if (m_lastScrollStartTimer.GetElapsedMilliseconds() >= SCROLLING_GAP)
  {
    m_scrollTimer.Stop();
    m_lastScrollStartTimer.Stop();
  }
}

int CGUIBaseContainer::CorrectOffset(int offset, int cursor) const
{
  return offset + cursor;
}

void CGUIBaseContainer::Reset()
{
  m_wasReset = true;
  m_items.clear();
}

void CGUIBaseContainer::LoadLayout(TiXmlElement *layout)
{
  TiXmlElement *itemElement = layout->FirstChildElement("itemlayout");
  while (itemElement)
  { // we have a new item layout
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, GetParentID(), false);
    m_layouts.push_back(itemLayout);
    itemElement = itemElement->NextSiblingElement("itemlayout");
  }
  itemElement = layout->FirstChildElement("focusedlayout");
  while (itemElement)
  { // we have a new item layout
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, GetParentID(), true);
    m_focusedLayouts.push_back(itemLayout);
    itemElement = itemElement->NextSiblingElement("focusedlayout");
  }
}

void CGUIBaseContainer::LoadContent(TiXmlElement *content)
{
  TiXmlElement *root = content->FirstChildElement("content");
  if (!root)
    return;

  vector<CGUIListItemPtr> items;
  TiXmlElement *item = root->FirstChildElement("item");
  while (item)
  {
    if (item->FirstChild())
    {
      CGUIStaticItemPtr newItem(new CGUIStaticItem(item, GetParentID()));
      items.push_back(newItem);
    }
    item = item->NextSiblingElement("item");
  }
  SetStaticContent(items, false);
}

void CGUIBaseContainer::SetStaticContent(const vector<CGUIListItemPtr> &items, bool forceUpdate /* = true */)
{
  m_staticContent = true;
  m_staticUpdateTime = 0;
  m_staticItems.clear();
  m_staticItems.assign(items.begin(), items.end());
  if (forceUpdate)
    UpdateStaticItems(true);
}

void CGUIBaseContainer::SetRenderOffset(const CPoint &offset)
{
  m_renderOffset = offset;
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
    for (int i = std::max(keepEnd + 1, 0); i < (int)m_items.size(); ++i)
      m_items[i]->FreeMemory();
  }
  else
  { // wrapping
    for (int i = std::max(keepEnd + 1, 0); i < keepStart && i < (int)m_items.size(); ++i)
      m_items[i]->FreeMemory();
  }
}

bool CGUIBaseContainer::InsideLayout(const CGUIListItemLayout *layout, const CPoint &point) const
{
  if (!layout) return false;
  if ((m_orientation == VERTICAL && (layout->Size(HORIZONTAL) > 1) && point.x > layout->Size(HORIZONTAL)) ||
      (m_orientation == HORIZONTAL && (layout->Size(VERTICAL) > 1)&& point.y > layout->Size(VERTICAL)))
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
    return (m_orientation == VERTICAL) ? (GetCursor() == data) : true;
  case CONTAINER_COLUMN:
    return (m_orientation == HORIZONTAL) ? (GetCursor() == data) : true;
  case CONTAINER_POSITION:
    return (GetCursor() == data);
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
    return (m_scrollTimer.GetElapsedMilliseconds() > std::max(m_scroller.GetDuration(), SCROLLING_THRESHOLD) || m_pageChangeTimer.IsRunning());
  default:
    return false;
  }
}

void CGUIBaseContainer::GetCurrentLayouts()
{
  m_layout = NULL;
  for (unsigned int i = 0; i < m_layouts.size(); i++)
  {
    if (m_layouts[i].CheckCondition())
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
    if (m_focusedLayouts[i].CheckCondition())
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
    label.Format("%i", GetCursor());
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
  if (GetOffset() + m_itemsPerPage >= (int)GetRows())  // last page
    return (GetRows() + m_itemsPerPage - 1) / m_itemsPerPage;
  return GetOffset() / m_itemsPerPage + 1;
}

void CGUIBaseContainer::GetCacheOffsets(int &cacheBefore, int &cacheAfter)
{
  if (m_scroller.IsScrollingDown())
  {
    cacheBefore = 0;
    cacheAfter = m_cacheItems;
  }
  else if (m_scroller.IsScrollingUp())
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

void CGUIBaseContainer::SetCursor(int cursor)
{
  m_cursor = cursor;
}

void CGUIBaseContainer::SetOffset(int offset)
{
  if (m_offset != offset)
    MarkDirtyRegion();
  m_offset = offset;
}

bool CGUIBaseContainer::CanFocus() const
{
  return (!m_items.empty() && CGUIControl::CanFocus());
}

void CGUIBaseContainer::SelectStaticItemById(int id)
{
  if (m_staticContent)
  {
    for (unsigned int i = 0 ; i < m_items.size() ; i++)
    {
      CGUIStaticItemPtr item = boost::static_pointer_cast<CGUIStaticItem>(m_items[i]);
      if (item->m_iprogramCount == id)
      {
        SelectItem(i);
        return;
      }
    }
  }
}

void CGUIBaseContainer::OnFocus()
{
  if (m_staticDefaultAlways)
    SelectStaticItemById(m_staticDefaultItem);

  CGUIControl::OnFocus();
}

/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIBaseContainer.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIInfoManager.h"
#include "GUIListItemLayout.h"
#include "GUIMessage.h"
#include "ServiceBroker.h"
#include "guilib/GUIListItem.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "guilib/listproviders/IListProvider.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/keyboard/KeyIDs.h"
#include "input/mouse/MouseEvent.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/CharsetConverter.h"
#include "utils/MathUtils.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <memory>

using namespace KODI;

#define HOLD_TIME_START 100
#define HOLD_TIME_END   3000
#define SCROLLING_GAP   200U
#define SCROLLING_THRESHOLD 300U

CGUIBaseContainer::CGUIBaseContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems)
    : IGUIContainer(parentID, controlID, posX, posY, width, height)
    , m_scroller(scroller)
{
  m_cursor = 0;
  m_offset = 0;
  m_lastHoldTime = 0;
  m_itemsPerPage = 10;
  m_pageControl = 0;
  m_orientation = orientation;
  m_analogScrollCount = 0;
  m_wasReset = false;
  m_layout = NULL;
  m_focusedLayout = NULL;
  m_cacheItems = preloadItems;
  m_scrollItemsPerFrame = 0.0f;
  m_type = VIEW_TYPE_NONE;
  m_autoScrollMoveTime = 0;
  m_autoScrollDelayTime = 0;
  m_autoScrollIsReversed = false;
  m_lastRenderTime = 0;
}

CGUIBaseContainer::CGUIBaseContainer(const CGUIBaseContainer& other)
  : IGUIContainer(other),
    m_renderOffset(other.m_renderOffset),
    m_analogScrollCount(other.m_analogScrollCount),
    m_lastHoldTime(other.m_lastHoldTime),
    m_orientation(other.m_orientation),
    m_itemsPerPage(other.m_itemsPerPage),
    m_pageControl(other.m_pageControl),
    m_layoutCondition(other.m_layoutCondition),
    m_focusedLayoutCondition(other.m_focusedLayoutCondition),
    m_scroller(other.m_scroller),
    m_listProvider(other.m_listProvider ? other.m_listProvider->Clone() : nullptr),
    m_wasReset(other.m_wasReset),
    m_letterOffsets(other.m_letterOffsets),
    m_autoScrollCondition(other.m_autoScrollCondition),
    m_autoScrollMoveTime(other.m_autoScrollMoveTime),
    m_autoScrollDelayTime(other.m_autoScrollDelayTime),
    m_autoScrollIsReversed(other.m_autoScrollIsReversed),
    m_lastRenderTime(other.m_lastRenderTime),
    m_cursor(other.m_cursor),
    m_offset(other.m_offset),
    m_cacheItems(other.m_cacheItems),
    m_scrollTimer(other.m_scrollTimer),
    m_lastScrollStartTimer(other.m_lastScrollStartTimer),
    m_pageChangeTimer(other.m_pageChangeTimer),
    m_clickActions(other.m_clickActions),
    m_focusActions(other.m_focusActions),
    m_unfocusActions(other.m_unfocusActions),
    m_matchTimer(other.m_matchTimer),
    m_match(other.m_match),
    m_scrollItemsPerFrame(other.m_scrollItemsPerFrame),
    m_gestureActive(other.m_gestureActive),
    m_waitForScrollEnd(other.m_waitForScrollEnd),
    m_lastScrollValue(other.m_lastScrollValue)
{
  // Initialize CGUIControl
  m_bInvalidated = true;

  for (const auto& item : other.m_items)
    m_items.emplace_back(std::make_shared<CGUIListItem>(*item));

  for (const auto& layout : other.m_layouts)
    m_layouts.emplace_back(layout, this);

  for (const auto& focusedLayout : other.m_focusedLayouts)
    m_focusedLayouts.emplace_back(focusedLayout, this);
}

CGUIBaseContainer::~CGUIBaseContainer(void)
{
  // release the container from items
  for (const auto& item : m_items)
    item->FreeMemory();
}

void CGUIBaseContainer::DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  CGUIControl::DoProcess(currentTime, dirtyregions);

  if (m_pageChangeTimer.IsRunning() && m_pageChangeTimer.GetElapsedMilliseconds() > 200)
    m_pageChangeTimer.Stop();
  m_wasReset = false;

  // if not visible, we reset the autoscroll timer
  if (!IsVisible() && m_autoScrollMoveTime)
  {
    ResetAutoScrolling();
  }
}

void CGUIBaseContainer::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  // update our auto-scrolling as necessary
  UpdateAutoScrolling(currentTime);

  if (!m_waitForScrollEnd && !m_gestureActive)
    ValidateOffset();

  if (m_bInvalidated)
    UpdateLayout();

  if (!m_layout || !m_focusedLayout) return;

  UpdateScrollOffset(currentTime);

  if (m_scroller.IsScrolling())
    MarkDirtyRegion();

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
      std::shared_ptr<CGUIListItem> item = m_items[itemNo];
      item->SetCurrentItem(itemNo + 1);

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

  // when we are scrolling up, offset will become lower (integer division, see offset calc)
  // to have same behaviour when scrolling down, we need to set page control to offset+1
  UpdatePageControl(offset + (m_scroller.IsScrollingDown() ? 1 : 0));

  m_lastRenderTime = currentTime;

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIBaseContainer::ProcessItem(float posX,
                                    float posY,
                                    std::shared_ptr<CGUIListItem>& item,
                                    bool focused,
                                    unsigned int currentTime,
                                    CDirtyRegionList& dirtyregions)
{
  if (!m_focusedLayout || !m_layout) return;

  // set the origin
  CServiceBroker::GetWinSystem()->GetGfxContext().SetOrigin(posX, posY);

  if (m_bInvalidated)
    item->SetInvalid();
  if (focused)
  {
    if (!item->GetFocusedLayout())
    {
      item->SetFocusedLayout(std::make_unique<CGUIListItemLayout>(*m_focusedLayout, this));
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
      auto layout = std::make_unique<CGUIListItemLayout>(*m_layout, this);
      item->SetLayout(std::move(layout));
    }
    if (item->GetFocusedLayout() && item->GetFocusedLayout()->IsAnimating(ANIM_TYPE_UNFOCUS))
      item->GetFocusedLayout()->Process(item.get(), m_parentID, currentTime, dirtyregions);
    if (item->GetLayout())
      item->GetLayout()->Process(item.get(), m_parentID, currentTime, dirtyregions);
  }

  CServiceBroker::GetWinSystem()->GetGfxContext().RestoreOrigin();
}

void CGUIBaseContainer::Render()
{
  if (!m_layout || !m_focusedLayout) return;

  int offset = (int)floorf(m_scroller.GetValue() / m_layout->Size(m_orientation));

  int cacheBefore, cacheAfter;
  GetCacheOffsets(cacheBefore, cacheAfter);

  if (CServiceBroker::GetWinSystem()->GetGfxContext().SetClipRegion(m_posX, m_posY, m_width, m_height))
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
    std::shared_ptr<CGUIListItem> focusedItem;
    int current = offset - cacheBefore;

    std::vector<RENDERITEM> renderitems;
    while (pos < end && m_items.size())
    {
      int itemNo = CorrectOffset(current, 0);
      if (itemNo >= (int)m_items.size())
        break;
      bool focused = (current == GetOffset() + GetCursor());
      if (itemNo >= 0)
      {
        std::shared_ptr<CGUIListItem> item = m_items[itemNo];
        // render our item
        if (focused)
        {
          focusedPos = pos;
          focusedItem = item;
        }
        else
        {
          if (m_orientation == VERTICAL)
            renderitems.emplace_back(RENDERITEM{origin.x, pos, item, false});
          else
            renderitems.emplace_back(RENDERITEM{pos, origin.y, item, false});
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
        renderitems.emplace_back(RENDERITEM{origin.x, focusedPos, focusedItem, true});
      else
        renderitems.emplace_back(RENDERITEM{focusedPos, origin.y, focusedItem, true});
    }

    if (CServiceBroker::GetWinSystem()->GetGfxContext().GetRenderOrder() ==
        RENDER_ORDER_FRONT_TO_BACK)
    {
      for (auto it = std::crbegin(renderitems); it != std::crend(renderitems); it++)
      {
        RenderItem(it->posX, it->posY, it->item.get(), it->focused);
      }
    }
    else
    {
      for (const auto& renderitem : renderitems)
      {
        RenderItem(renderitem.posX, renderitem.posY, renderitem.item.get(), renderitem.focused);
      }
    }

    CServiceBroker::GetWinSystem()->GetGfxContext().RestoreClipRegion();
  }

  CGUIControl::Render();
}


void CGUIBaseContainer::RenderItem(float posX, float posY, CGUIListItem *item, bool focused)
{
  if (!m_focusedLayout || !m_layout) return;

  // set the origin
  CServiceBroker::GetWinSystem()->GetGfxContext().SetOrigin(posX, posY);

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
  CServiceBroker::GetWinSystem()->GetGfxContext().RestoreOrigin();
}

bool CGUIBaseContainer::OnAction(const CAction &action)
{
  if (action.GetID() == KEY_UNICODE)
  {
    std::string letter;
    g_charsetConverter.wToUTF8({action.GetUnicode()}, letter);
    OnJumpLetter(letter);
    return true;
  }
  // stop the timer on any other action
  m_matchTimer.Stop();

  switch (action.GetID())
  {
  case ACTION_MOVE_LEFT:
  case ACTION_MOVE_RIGHT:
  case ACTION_MOVE_DOWN:
  case ACTION_MOVE_UP:
  case ACTION_NAV_BACK:
  case ACTION_PREVIOUS_MENU:
    {
      if (!HasFocus()) return false;

      if (action.GetHoldTime() > HOLD_TIME_START &&
        ((m_orientation == VERTICAL && (action.GetID() == ACTION_MOVE_UP || action.GetID() == ACTION_MOVE_DOWN)) ||
         (m_orientation == HORIZONTAL && (action.GetID() == ACTION_MOVE_LEFT || action.GetID() == ACTION_MOVE_RIGHT))))
      { // action is held down - repeat a number of times
        float speed = std::min(1.0f, (float)(action.GetHoldTime() - HOLD_TIME_START) / (HOLD_TIME_END - HOLD_TIME_START));
        unsigned int frameDuration = std::min(CTimeUtils::GetFrameTime() - m_lastHoldTime, 50u); // max 20fps

        // maximal scroll rate is at least 30 items per second, and at most (item_rows/7) items per second
        //  i.e. timed to take 7 seconds to traverse the list at full speed.
        // minimal scroll rate is at least 10 items per second
        float maxSpeed = std::max(frameDuration * 0.001f * 30, frameDuration * 0.001f * GetRows() / 7);
        float minSpeed = frameDuration * 0.001f * 10;
        m_scrollItemsPerFrame += std::max(minSpeed, speed*maxSpeed); // accelerate to max speed
        m_lastHoldTime = CTimeUtils::GetFrameTime();

        if(m_scrollItemsPerFrame < 1.0f)//not enough hold time accumulated for one step
          return true;

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
  case ACTION_CONTEXT_MENU:
    if (OnContextMenu())
      return true;
    break;
  case ACTION_SHOW_INFO:
    if (m_listProvider)
    {
      const int selected = GetSelectedItem();
      if (selected >= 0 && selected < static_cast<int>(m_items.size()))
      {
        if (m_listProvider->OnInfo(m_items[selected]))
          return true;
      }
    }
    if (OnInfo())
      return true;
    else if (action.GetID())
      return OnClick(action.GetID());

    return false;

  case ACTION_PLAYER_PLAY:
    if (m_listProvider)
    {
      const int selected = GetSelectedItem();
      if (selected >= 0 && selected < static_cast<int>(m_items.size()))
      {
        if (m_listProvider->OnPlay(m_items[selected]))
          return true;
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
    OnNextLetter();
    return true;
  case ACTION_PREV_LETTER:
    OnPrevLetter();
    return true;
  case ACTION_JUMP_SMS2:
  case ACTION_JUMP_SMS3:
  case ACTION_JUMP_SMS4:
  case ACTION_JUMP_SMS5:
  case ACTION_JUMP_SMS6:
  case ACTION_JUMP_SMS7:
  case ACTION_JUMP_SMS8:
  case ACTION_JUMP_SMS9:
    OnJumpSMS(action.GetID() - ACTION_JUMP_SMS2 + 2);
    return true;

  default:
    break;
  }
  return action.GetID() && OnClick(action.GetID());
}

bool CGUIBaseContainer::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (!m_listProvider)
    {
      if (message.GetMessage() == GUI_MSG_LABEL_BIND && message.GetPointer())
      { // bind our items
        Reset();
        CFileItemList *items = static_cast<CFileItemList*>(message.GetPointer());
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
    else if (message.GetMessage() == GUI_MSG_SETFOCUS)
    {
      if (message.GetParam1()) // subfocus item is specified, so set the offset appropriately
      {
        int offset = GetOffset();
        if (message.GetParam2() && message.GetParam2() == 1)
          offset = 0;
        int item = std::min(offset + message.GetParam1() - 1, (int)m_items.size() - 1);
        SelectItem(item);
      }
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
        if (message.GetParam1() != GetOffset())
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
    else if (message.GetMessage() == GUI_MSG_REFRESH_THUMBS)
    {
      if (m_listProvider)
        m_listProvider->FreeResources(true);
    }
    else if (message.GetMessage() == GUI_MSG_MOVE_OFFSET)
    {
      int count = message.GetParam1();
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
  CGUIAction action = GetAction(ACTION_MOVE_UP);
  bool wrapAround = action.GetNavigation() == GetID() || !action.HasActionsMeetingCondition();
  if (m_orientation == VERTICAL && MoveUp(wrapAround))
    return;
  // with horizontal lists it doesn't make much sense to have multiselect labels
  CGUIControl::OnUp();
}

void CGUIBaseContainer::OnDown()
{
  CGUIAction action = GetAction(ACTION_MOVE_DOWN);
  bool wrapAround = action.GetNavigation() == GetID() || !action.HasActionsMeetingCondition();
  if (m_orientation == VERTICAL && MoveDown(wrapAround))
    return;
  // with horizontal lists it doesn't make much sense to have multiselect labels
  CGUIControl::OnDown();
}

void CGUIBaseContainer::OnLeft()
{
  CGUIAction action = GetAction(ACTION_MOVE_LEFT);
  bool wrapAround = action.GetNavigation() == GetID() || !action.HasActionsMeetingCondition();
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
  CGUIAction action = GetAction(ACTION_MOVE_RIGHT);
  bool wrapAround = action.GetNavigation() == GetID() || !action.HasActionsMeetingCondition();
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

void CGUIBaseContainer::OnJumpLetter(const std::string& letter, bool skip /*=false*/)
{
  if (m_matchTimer.GetElapsedMilliseconds() < letter_match_timeout)
    m_match += letter;
  else
    m_match = letter;

  m_matchTimer.StartZero();

  // we can't jump through letters if we have none
  if (0 == m_letterOffsets.size())
    return;

  // find the current letter we're focused on
  unsigned int offset = CorrectOffset(GetOffset(), GetCursor());
  unsigned int i      = (offset + ((skip) ? 1 : 0)) % m_items.size();
  do
  {
    std::shared_ptr<CGUIListItem> item = m_items[i];
    std::string label = item->GetLabel();
    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
      label = SortUtils::RemoveArticles(label);
    if (0 == StringUtils::CompareNoCase(label, m_match, m_match.size()))
    {
      SelectItem(i);
      return;
    }
    i = (i+1) % m_items.size();
  } while (i != offset);

  // no match found - repeat with a single letter
  std::wstring wmatch;
  g_charsetConverter.utf8ToW(m_match, wmatch);
  if (wmatch.length() > 1)
  {
    m_match.clear();
    OnJumpLetter(letter, true);
  }
}

void CGUIBaseContainer::OnJumpSMS(int letter)
{
  static const char letterMap[8][6] = { "ABC2", "DEF3", "GHI4", "JKL5", "MNO6", "PQRS7", "TUV8", "WXYZ9" };

  // only 2..9 supported
  if (letter < 2 || letter > 9 || !m_letterOffsets.size())
    return;

  const std::string letters = letterMap[letter - 2];
  // find where we currently are
  int offset = CorrectOffset(GetOffset(), GetCursor());
  unsigned int currentLetter = 0;
  while (currentLetter + 1 < m_letterOffsets.size() && m_letterOffsets[currentLetter + 1].first <= offset)
    currentLetter++;

  // now switch to the next letter
  std::string current = m_letterOffsets[currentLetter].second;
  size_t startPos = (letters.find(current) + 1) % letters.size();
  // now jump to letters[startPos], or another one in the same range if possible
  size_t pos = startPos;
  while (true)
  {
    // check if we can jump to this letter
    for (size_t i = 0; i < m_letterOffsets.size(); i++)
    {
      if (m_letterOffsets[i].second == letters.substr(pos, 1))
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
  ResetAutoScrolling();
  ScrollToOffset(GetOffset() + amount);
}

int CGUIBaseContainer::GetSelectedItem() const
{
  return CorrectOffset(GetOffset(), GetCursor());
}

std::shared_ptr<CGUIListItem> CGUIBaseContainer::GetListItem(int offset, unsigned int flag) const
{
  if (!m_items.size() || !m_layout)
    return std::shared_ptr<CGUIListItem>();
  int item = GetSelectedItem() + offset;
  if (flag & INFOFLAG_LISTITEM_POSITION) // use offset from the first item displayed, taking into account scrolling
    item = CorrectOffset((int)(m_scroller.GetValue() / m_layout->Size(m_orientation)), offset);

  if (flag & INFOFLAG_LISTITEM_ABSOLUTE) // use offset from the first item
    item = CorrectOffset(0, offset);

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
  return std::shared_ptr<CGUIListItem>();
}

CGUIListItemLayout *CGUIBaseContainer::GetFocusedLayout() const
{
  std::shared_ptr<CGUIListItem> item = GetListItem(0);
  if (item.get()) return item->GetFocusedLayout();
  return NULL;
}

bool CGUIBaseContainer::OnMouseOver(const CPoint &point)
{
  // select the item under the pointer
  if (!m_waitForScrollEnd)
    SelectItemFromPoint(point - CPoint(m_posX, m_posY));
  return CGUIControl::OnMouseOver(point);
}

EVENT_RESULT CGUIBaseContainer::OnMouseEvent(const CPoint& point, const MOUSE::CMouseEvent& event)
{
  if (event.m_id == ACTION_MOUSE_LEFT_CLICK ||
      event.m_id == ACTION_MOUSE_DOUBLE_CLICK ||
      event.m_id == ACTION_MOUSE_RIGHT_CLICK)
  {
    // Cancel touch
    m_waitForScrollEnd = false;
    int select = GetSelectedItem();
    if (SelectItemFromPoint(point - CPoint(m_posX, m_posY)))
    {
      if (event.m_id != ACTION_MOUSE_RIGHT_CLICK || select == GetSelectedItem())
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
    m_waitForScrollEnd = true;
    m_lastScrollValue = m_scroller.GetValue();
    return (m_orientation == HORIZONTAL) ? EVENT_RESULT_PAN_HORIZONTAL : EVENT_RESULT_PAN_VERTICAL;
  }
  else if (event.m_id == ACTION_GESTURE_BEGIN)
  { // grab exclusive access
    m_gestureActive = true;
    CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, GetID(), GetParentID());
    SendWindowMessage(msg);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_GESTURE_PAN)
  { // do the drag and validate our offset (corrects for end of scroll)
    m_scroller.SetValue(m_scroller.GetValue() - ((m_orientation == HORIZONTAL) ? event.m_offsetX : event.m_offsetY));
    float size = (m_layout) ? m_layout->Size(m_orientation) : 10.0f;
    int offset = MathUtils::round_int(static_cast<double>(m_scroller.GetValue() / size));
    m_lastScrollStartTimer.Stop();
    m_scrollTimer.Start();
    const int absCursor = CorrectOffset(GetOffset(), GetCursor());
    SetOffset(offset);
    ValidateOffset();
    // Notify Application if Inertial scrolling reaches lists end
    if (m_waitForScrollEnd)
    {
      if (fabs(m_scroller.GetValue() - m_lastScrollValue) < 0.001f)
      {
        m_waitForScrollEnd = false;
        return EVENT_RESULT_UNHANDLED;
      }
      else
        m_lastScrollValue = m_scroller.GetValue();
    }
    else
    {
      CGUIBaseContainer::SetCursor(absCursor - CorrectOffset(GetOffset(), 0));
    }
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_GESTURE_END || event.m_id == ACTION_GESTURE_ABORT)
  { // release exclusive access
    CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, 0, GetParentID());
    SendWindowMessage(msg);
    m_scrollTimer.Stop();
    // and compute the nearest offset from this and scroll there
    float size = (m_layout) ? m_layout->Size(m_orientation) : 10.0f;
    float offset = m_scroller.GetValue() / size;
    int toOffset = MathUtils::round_int(static_cast<double>(offset));
    if (toOffset < offset)
      SetOffset(toOffset+1);
    else
      SetOffset(toOffset-1);
    ScrollToOffset(toOffset);
    ValidateOffset();
    SetCursor(GetCursor());
    SetFocus(true);
    m_waitForScrollEnd = false;
    m_gestureActive = false;
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

bool CGUIBaseContainer::OnClick(int actionID)
{
  int subItem = 0;
  if (actionID == ACTION_SELECT_ITEM || actionID == ACTION_MOUSE_LEFT_CLICK)
  {
    if (m_listProvider)
    { // "select" action
      int selected = GetSelectedItem();
      if (selected >= 0 && selected < static_cast<int>(m_items.size()))
      {
        // One of the actions could trigger a reload of the GUI which destroys
        // this CGUIBaseContainer and therefore the m_items[selected] we are
        // going to process. The shared_ptr ensures that item survives until
        // it has been processed.
        std::shared_ptr<CGUIListItem> item = m_items[selected];

        if (m_clickActions.HasActionsMeetingCondition())
          m_clickActions.ExecuteActions(0, GetParentID(), item);
        else
          m_listProvider->OnClick(item);
      }
      return true;
    }
    // grab the currently focused subitem (if applicable)
    CGUIListItemLayout *focusedLayout = GetFocusedLayout();
    if (focusedLayout)
      subItem = focusedLayout->GetFocusedItem();
  }
  else if (actionID == ACTION_MOUSE_RIGHT_CLICK)
  {
    if (OnContextMenu())
      return true;
  }
  // Don't know what to do, so send to our parent window.
  CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID(), actionID, subItem);
  return SendWindowMessage(msg);
}

bool CGUIBaseContainer::OnContextMenu()
{
  if (m_listProvider)
  {
    int selected = GetSelectedItem();
    if (selected >= 0 && selected < static_cast<int>(m_items.size()))
    {
      m_listProvider->OnContextMenu(m_items[selected]);
      return true;
    }
  }
  return false;
}

std::string CGUIBaseContainer::GetDescription() const
{
  std::string strLabel;
  int item = GetSelectedItem();
  if (item >= 0 && item < (int)m_items.size())
  {
    std::shared_ptr<CGUIListItem> pItem = m_items[item];
    if (pItem->m_bIsFolder)
      strLabel = StringUtils::Format("[{}]", pItem->GetLabel());
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

void CGUIBaseContainer::SaveStates(std::vector<CControlState> &states)
{
  if (!m_listProvider || !m_listProvider->AlwaysFocusDefaultItem())
    states.emplace_back(GetID(), GetSelectedItem());
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
  if (m_listProvider)
  {
    UpdateListProvider(true);
  }
}

void CGUIBaseContainer::FreeResources(bool immediately)
{
  CGUIControl::FreeResources(immediately);
  if (m_listProvider)
  {
    if (immediately)
    {
      Reset();
      m_listProvider->Reset();
    }
  }
  m_scroller.Stop();
}

void CGUIBaseContainer::UpdateLayout(bool updateAllItems)
{
  if (updateAllItems)
  { // free memory of items
    for (iItems it = m_items.begin(); it != m_items.end(); ++it)
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

  // update layouts in case of condition changed
  if ((m_layout && m_layout->CheckCondition() != m_layoutCondition) ||
      (m_focusedLayout && m_focusedLayout->CheckCondition() != m_focusedLayoutCondition))
  {
    if (m_layout)
      m_layoutCondition = m_layout->CheckCondition();
    if (m_focusedLayout)
      m_focusedLayoutCondition = m_focusedLayout->CheckCondition();

    int itemIndex = GetSelectedItem();
    UpdateLayout(true); // true to refresh all items
    SelectItem(itemIndex);
  }

  UpdateListProvider();
}

void CGUIBaseContainer::AssignDepth()
{
  std::shared_ptr<CGUIListItem> focusedItem = nullptr;
  int32_t current = 0;

  for (const auto& item : m_items)
  {
    bool focused = (current == GetOffset() + GetCursor());
    if (focused)
    {
      focusedItem = item;
    }
    else
    {
      if (item->GetFocusedLayout())
        item->GetFocusedLayout()->AssignDepth();
      if (item->GetLayout())
        item->GetLayout()->AssignDepth();
    }
    current++;
  }

  if (focusedItem)
  {
    if (focusedItem->GetFocusedLayout())
      focusedItem->GetFocusedLayout()->AssignDepth();
    if (focusedItem->GetLayout())
      focusedItem->GetLayout()->AssignDepth();
  }
}

void CGUIBaseContainer::UpdateListProvider(bool forceRefresh /* = false */)
{
  if (m_listProvider)
  {
    if (m_listProvider->Update(forceRefresh))
    {
      // save the current item
      int currentItem = GetSelectedItem();
      CGUIListItem *current = (currentItem >= 0 && currentItem < (int)m_items.size()) ? m_items[currentItem].get() : NULL;
      const std::string prevSelectedPath((current && current->IsFileItem()) ? static_cast<CFileItem *>(current)->GetPath() : "");

      Reset();
      m_listProvider->Fetch(m_items);
      SetPageControlRange();
      // update the newly selected item
      bool found = false;

      // first, try to re-identify selected item by comparing item pointers, though it is not guaranteed that item instances got not recreated on update.
      for (int i = 0; i < (int)m_items.size(); i++)
      {
        if (m_items[i].get() == current)
        {
          found = true;
          if (i != currentItem)
          {
            SelectItem(i);
            break;
          }
        }
      }
      if (!found && !prevSelectedPath.empty())
      {
        // as fallback, try to re-identify selected item by comparing item paths.
        for (int i = 0; i < static_cast<int>(m_items.size()); i++)
        {
          const std::shared_ptr<CGUIListItem> c(m_items[i]);
          if (c->IsFileItem())
          {
            const std::string &selectedPath = static_cast<CFileItem *>(c.get())->GetPath();
            if (selectedPath == prevSelectedPath)
            {
              found = true;
              if (i != currentItem)
              {
                SelectItem(i);
                break;
              }
            }
          }
        }
      }
      if (!found && currentItem >= (int)m_items.size())
        SelectItem(m_items.size()-1);
      SetInvalid();
    }
    // always update the scroll by letter, as the list provider may have altered labels
    // while not actually changing the list items.
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
  std::string currentMatch;
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    std::shared_ptr<CGUIListItem> item = m_items[i];
    // The letter offset jumping is only for ASCII characters at present, and
    // our checks are all done in uppercase
    std::string nextLetter;
    std::wstring character = item->GetSortLabel().substr(0, 1);
    StringUtils::ToUpper(character);
    g_charsetConverter.wToUTF8(character, nextLetter);
    if (currentMatch != nextLetter)
    {
      currentMatch = nextLetter;
      m_letterOffsets.emplace_back(static_cast<int>(i), currentMatch);
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
  else
  {
    m_scrollTimer.Stop();
    m_scroller.Update(~0U);
  }
  SetOffset(offset);
}

void CGUIBaseContainer::SetAutoScrolling(const TiXmlNode *node)
{
  if (!node) return;
  const TiXmlElement *scroll = node->FirstChildElement("autoscroll");
  if (scroll)
  {
    scroll->Attribute("time", &m_autoScrollMoveTime);
    if (scroll->Attribute("reverse"))
      m_autoScrollIsReversed = true;
    if (scroll->FirstChild())
      m_autoScrollCondition = CServiceBroker::GetGUI()->GetInfoManager().Register(scroll->FirstChild()->ValueStr(), GetParentID());
  }
}

void CGUIBaseContainer::ResetAutoScrolling()
{
  m_autoScrollDelayTime = 0;
}

void CGUIBaseContainer::UpdateAutoScrolling(unsigned int currentTime)
{
  if (m_autoScrollCondition && m_autoScrollCondition->Get(INFO::DEFAULT_CONTEXT))
  {
    if (m_lastRenderTime)
      m_autoScrollDelayTime += currentTime - m_lastRenderTime;
    if (m_autoScrollDelayTime > (unsigned int)m_autoScrollMoveTime && !m_scroller.IsScrolling())
    { // delay is finished - start moving
      m_autoScrollDelayTime = 0;
      // Move up or down whether reversed moving is true or false
      m_autoScrollIsReversed ? MoveUp(true) : MoveDown(true);
    }
  }
  else
    ResetAutoScrolling();
}

void CGUIBaseContainer::SetContainerMoving(int direction)
{
  if (direction)
    CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetGUIControlsInfoProvider().SetContainerMoving(GetID(), direction > 0, m_scroller.IsScrolling());
}

void CGUIBaseContainer::UpdateScrollOffset(unsigned int currentTime)
{
  if (m_scroller.Update(currentTime))
    MarkDirtyRegion();
  else if (m_lastScrollStartTimer.IsRunning() && m_lastScrollStartTimer.GetElapsedMilliseconds() >= SCROLLING_GAP)
  {
    m_scrollTimer.Stop();
    m_lastScrollStartTimer.Stop();
    SetCursor(GetCursor());
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
  m_lastItem.reset();
  ResetAutoScrolling();
}

void CGUIBaseContainer::LoadLayout(TiXmlElement *layout)
{
  TiXmlElement *itemElement = layout->FirstChildElement("itemlayout");
  while (itemElement)
  { // we have a new item layout
    m_layouts.emplace_back();
    m_layouts.back().LoadLayout(itemElement, GetParentID(), false, m_width, m_height);
    itemElement = itemElement->NextSiblingElement("itemlayout");
    m_layouts.back().SetParentControl(this);
  }
  itemElement = layout->FirstChildElement("focusedlayout");
  while (itemElement)
  { // we have a new item layout
    m_focusedLayouts.emplace_back();
    m_focusedLayouts.back().LoadLayout(itemElement, GetParentID(), true, m_width, m_height);
    itemElement = itemElement->NextSiblingElement("focusedlayout");
    m_focusedLayouts.back().SetParentControl(this);
  }
}

void CGUIBaseContainer::LoadListProvider(TiXmlElement *content, int defaultItem, bool defaultAlways)
{
  m_listProvider = IListProvider::Create(content, GetParentID());
  if (m_listProvider)
    m_listProvider->SetDefaultItem(defaultItem, defaultAlways);
}

void CGUIBaseContainer::SetListProvider(std::unique_ptr<IListProvider> provider)
{
  m_listProvider = std::move(provider);
  UpdateListProvider(true);
}

void CGUIBaseContainer::SetRenderOffset(const CPoint &offset)
{
  m_renderOffset = offset;
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
  CLog::Log(LOGDEBUG, "{} for container {}", __FUNCTION__, GetID());
  for (unsigned int i = 0; i < m_items.size(); ++i)
  {
    std::shared_ptr<CGUIListItem> item = m_items[i];
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
  case CONTAINER_HAS_PARENT_ITEM:
    return (m_items.size() && m_items[0]->IsFileItem() && (std::static_pointer_cast<CFileItem>(m_items[0]))->IsParentFolder());
  case CONTAINER_SUBITEM:
    {
      CGUIListItemLayout *layout = GetFocusedLayout();
      return layout ? (layout->GetFocusedItem() == (unsigned int)data) : false;
    }
  case CONTAINER_SCROLLING:
    return ((m_scrollTimer.IsRunning() && m_scrollTimer.GetElapsedMilliseconds() > std::max(m_scroller.GetDuration(), SCROLLING_THRESHOLD)) || m_pageChangeTimer.IsRunning());
  case CONTAINER_ISUPDATING:
    return (m_listProvider) ? m_listProvider->IsUpdating() : false;
  default:
    return false;
  }
}

void CGUIBaseContainer::GetCurrentLayouts()
{
  m_layout = NULL;
  for (auto &layout : m_layouts)
  {
    if (layout.CheckCondition())
    {
      m_layout = &layout;
      break;
    }
  }
  if (!m_layout && !m_layouts.empty())
    m_layout = &m_layouts.front(); // failsafe

  m_focusedLayout = NULL;
  for (auto &layout : m_focusedLayouts)
  {
    if (layout.CheckCondition())
    {
      m_focusedLayout = &layout;
      break;
    }
  }
  if (!m_focusedLayout && !m_focusedLayouts.empty())
    m_focusedLayout = &m_focusedLayouts.front(); // failsafe
}

bool CGUIBaseContainer::HasNextPage() const
{
  return false;
}

bool CGUIBaseContainer::HasPreviousPage() const
{
  return false;
}

std::string CGUIBaseContainer::GetLabel(int info) const
{
  std::string label;
  switch (info)
  {
  case CONTAINER_NUM_PAGES:
    label = std::to_string((GetRows() + m_itemsPerPage - 1) / m_itemsPerPage);
    break;
  case CONTAINER_CURRENT_PAGE:
    label = std::to_string(GetCurrentPage());
    break;
  case CONTAINER_POSITION:
    label = std::to_string(GetCursor());
    break;
  case CONTAINER_CURRENT_ITEM:
    {
      if (m_items.size() && m_items[0]->IsFileItem() && (std::static_pointer_cast<CFileItem>(m_items[0]))->IsParentFolder())
        label = std::to_string(GetSelectedItem());
      else
        label = std::to_string(GetSelectedItem() + 1);
    }
    break;
  case CONTAINER_NUM_ALL_ITEMS:
  case CONTAINER_NUM_ITEMS:
    {
      unsigned int numItems = GetNumItems();
      if (info == CONTAINER_NUM_ITEMS && numItems && m_items[0]->IsFileItem() && (std::static_pointer_cast<CFileItem>(m_items[0]))->IsParentFolder())
        label = std::to_string(numItems - 1);
      else
        label = std::to_string(numItems);
    }
    break;
  case CONTAINER_NUM_NONFOLDER_ITEMS:
    {
      int numItems = 0;
      for (const auto& item : m_items)
      {
        if (!item->m_bIsFolder)
          numItems++;
      }
      label = std::to_string(numItems);
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

void CGUIBaseContainer::GetCacheOffsets(int &cacheBefore, int &cacheAfter) const
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
  if (m_cursor != cursor)
    MarkDirtyRegion();
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
  if (CGUIControl::CanFocus())
  {
    /*
     We allow focus if we have items available or if we have a list provider
     that's in the process of updating.
     */
    return !m_items.empty() || (m_listProvider && m_listProvider->IsUpdating());
  }
  return false;
}

void CGUIBaseContainer::OnFocus()
{
  if (m_listProvider && m_listProvider->AlwaysFocusDefaultItem())
    SelectItem(m_listProvider->GetDefaultItem());

  if (m_focusActions.HasAnyActions())
    m_focusActions.ExecuteActions(GetID(), GetParentID());

  CGUIControl::OnFocus();
}

void CGUIBaseContainer::OnUnFocus()
{
  if (m_unfocusActions.HasAnyActions())
    m_unfocusActions.ExecuteActions(GetID(), GetParentID());

  CGUIControl::OnUnFocus();
}

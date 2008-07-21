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
#include "GUIEPGGridContainer.h"
#include "GUIControlFactory.h"
#include "GUIListItem.h"

CGUIEPGGridContainer::CGUIEPGGridContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, int scrollTime, int timeBlocks)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  ControlType = GUICONTAINER_EPGGRID;
  m_posX = posX;
  m_posY = posY;
  m_width = width;
  m_height = height;
  m_curChannel = 0;
  m_curItem = 0;
  m_chanOffset = 0;
  m_itemOffset = 0;
  m_scrollTime = scrollTime ? scrollTime : 1;
  m_scrollOffset = 0;
  m_scrollSpeed = 0;
  m_channelsPerPage = 10;
  m_renderTime = 0;
  m_lastItem = NULL;
  m_numTimeBlocks = timeBlocks;
  m_wrapAround = false; /* get from settings */
}

CGUIEPGGridContainer::~CGUIEPGGridContainer(void)
{
}

void CGUIEPGGridContainer::RenderItem(float posX, float posY, CGUIListItem *item, bool focused)
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

void CGUIEPGGridContainer::Render()
{
  ValidateOffset();

  if (m_bInvalidated)
    UpdateLayout();

  if (!m_layout || !m_focusedLayout)
    return;

  m_scrollOffset += m_scrollSpeed * (m_renderTime - m_scrollLastTime);
  if ((m_scrollSpeed < 0 && m_scrollOffset < m_chanOffset * m_layout->Size(VERTICAL)) ||
    (m_scrollSpeed > 0 && m_scrollOffset > m_chanOffset * m_layout->Size(VERTICAL)))
  {
    m_scrollOffset = m_chanOffset * m_layout->Size(VERTICAL);
    m_scrollSpeed = 0;
  }
  m_scrollLastTime = m_renderTime;

  int offset = (int)(m_scrollOffset / m_layout->Size(VERTICAL));
  // Free memory not used on screen at the moment, do this first so there's more memory for the new items.
  /*FreeMemory(CorrectOffset(offset, 0), CorrectOffset(offset, m_channelsPerPage + 1));*/
  g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height);

  float posX = m_posX;
  float posY = m_posY;
  float focusedPosX = 0;
  float focusedPosY = 0;
  CGUIListItemPtr focusedItem;

  int iC = m_chanOffset;
  int iS = 0;
  int x  = 0;

  while (posY < m_posY + m_height && m_gridItems.size())
  {
    if (iC >= (int)m_gridItems.size())
      break;
    while (posX < m_posX + m_width && iS < (int) m_gridItems[iC].size())
    {
      CGUIListItemPtr item = m_gridItems[iC][iS];

      bool focused = false;
      if (iC == m_curChannel)
        if(iS == m_curItem)
          focused = true;

      // render our item
      if (focused)
      {
        focusedPosX = posX;
        focusedPosY = posY;
        focusedItem = item;
      }
      else
        RenderItem(posX, posY, item.get(), focused);

      // increment our X position
      iS++;
      posX += item->GetLayout()->Size(HORIZONTAL); // assumes focused & unfocused layouts have equal length

    }
    // increment our Y position
    iC++;
    posX = m_posX; // reset X position
    iS = 0;
    posY += m_layout->Size(VERTICAL); // assumes focused & unfocused layouts have equal height
  }
  // and render the focused item last (for overlapping purposes)
  if (focusedItem)
    RenderItem(focusedPosX, focusedPosY, focusedItem.get(), true);

  CGUIControl::Render();
  g_graphicsContext.RestoreClipRegion();
}

void CGUIEPGGridContainer::UpdateItems(EPGGrid &gridData)
{
  if (m_gridItems.size() > 0)
    m_gridItems.clear();

  float posX = m_posX;
  float posY = m_posY;
  float blockSize = m_width / m_numTimeBlocks;

  CDateTime gridCursor, gridStart, gridEnd;
  CDateTimeSpan gridDuration, blockDuration, offset;

  gridStart = CDateTime::GetCurrentDateTime();
  offset.SetDateTimeSpan(0, 0, 0, (gridStart.GetMinute() % 30) * 60 + gridStart.GetSecond());
  gridStart -= offset; // tidy up gridstart time
  gridCursor = gridStart;
  gridDuration.SetDateTimeSpan(0, 0, m_numTimeBlocks*5, 0);
  blockDuration.SetDateTimeSpan(0, 0, 5 ,0);
  gridEnd = gridStart + gridDuration;

  itEPGRow itY = gridData.begin();
  bool unknown = false;

  std::vector< CGUIListItemPtr > rowItems;
  std::vector< CGUIListItemPtr > unknownItems;

  DWORD tick(timeGetTime());
  for (int y = 0 ; itY != gridData.end(); itY++, y++)
  {
    itEPGShow itX = itY->shows.begin();
    
    std::vector< int > rowIndex;
    int itemSize = 0;
    int itemIdx = 0;

    CGUIListItemPtr Item(new CGUIListItem());

    for (int x = 0; x < (m_numTimeBlocks + 1); x++)
    {
      CDateTime itemStart, itemEnd;
      CDateTimeSpan itemDuration;
      itemStart.SetFromDBDateTime((*itX)->GetProperty("StartTime").c_str());
      itemDuration.SetDateTimeSpan(0, 0, 0, (*itX)->GetPropertyInt("Duration"));
      itemEnd = itemStart + itemDuration;

      if (x == 32)
      {
        int breakpoint = 6;
      }
      if ((*itX) && gridCursor >= itemEnd || itemEnd >= gridEnd)
      {
        /* give each ListItem it's own unique layout */
        CGUIListItemLayout *pItemLayout = new CGUIListItemLayout(*m_layout);
        CGUIListItemLayout *pItemFocusedLayout = new CGUIListItemLayout(*m_focusedLayout);

        if (itemEnd >= gridEnd)
        {
          itemSize = m_numTimeBlocks - x + 1;
        }

        pItemLayout->SetWidth(itemSize*blockSize); // resize the layouts according to programme duration
        pItemFocusedLayout->SetWidth(itemSize*blockSize);
        Item->SetFocusedLayout(pItemFocusedLayout);
        Item->SetLayout(pItemLayout);

        rowItems.push_back(Item); // fill the channels vector

        if (itemEnd >= gridEnd)
          break; 

        itX++; // next programme

        if (itX == itY->shows.end())
        {
          // end of grid data for this channel
          break;
        }

        itemStart.SetFromDBDateTime((*itX)->GetProperty("StartTime").c_str());
        itemDuration.SetDateTimeSpan(0, 0 , 0, (*itX)->GetPropertyInt("Duration"));
        itemEnd = itemStart + itemDuration;
        itemSize = 0;
      }

      if (!(*itX) || (gridCursor < itemStart)) // there's a gap in the schedule
      {
        if (unknown)
        {
          rowIndex[itemIdx]++;
          itemSize++;
        }
        else
        {
          Item = CGUIListItemPtr();
          // need layout
          unknownItems.push_back(Item);
          Item->SetLabel("_?_");
          Item->SetProperty("Category", "Unknown");
          rowIndex[itemIdx] = 1;
          unknown = true;
        }
      }
      else
      {
        if (Item == *itX)
        {
          // we've already found this programme, increment size of item in blocks
          itemSize++;
          rowIndex[itemIdx] = itemSize;
        }
        else
        {
          Item = *itX; // found a new programme
          Item->SetLabel((*itX)->GetLabel());
          Item->SetProperty("StartTime", (*itX)->GetProperty("StartTime"));
          itemSize++;
          rowIndex.push_back(itemSize); // rowIndex[x] = final block of item 'x', currently set to
          unknown = false;
        }
      }
      gridCursor += blockDuration; // add 5 minutes
    }
    m_gridItems.push_back(rowItems);
    rowItems.clear();
    gridCursor = gridStart;
  }
  CLog::Log(LOGDEBUG, "%s completed successfully in %u ms", __FUNCTION__, timeGetTime()-tick);
}
      
  //    CDateTime itemStart; 
  //    itemStart.SetFromDBDateTime(Item->GetProperty("StartTime").c_str());

  //    /* calculate layout's width */
  //    float itemLength;

  //    if (itemStart < gridStart)
  //    {
  //      itemStart = gridStart;
  //    }

  //    itemDuration.SetDateTimeSpan(0, 0, 0, Item->GetPropertyInt("Duration"));
  //    CDateTime itemEnd = itemStart + itemDuration;

  //    /* truncate programmes that extend beyond the grid */
  //    if (itemEnd > gridEnd)
  //      itemLength = m_width - m_posX;
  //    else
  //      itemLength = (duration % 5) * blockSize;

  //    /* give each ListItem it's own unique layout */
  //    CGUIListItemLayout *pItemLayout = new CGUIListItemLayout(*m_layout);
  //    CGUIListItemLayout *pItemFocusedLayout = new CGUIListItemLayout(*m_focusedLayout);

  //    pItemLayout->SetWidth(itemLength); // resize the layouts according to programme duration
  //    pItemFocusedLayout->SetWidth(itemLength);
  //    Item->SetFocusedLayout(pItemFocusedLayout);
  //    Item->SetLayout(pItemLayout);

  //    rowItems.push_back(Item); // fill the channels vector
  //  }
  //  if (rowItems.size() > 0)
  //  {
  //    m_gridItems.push_back(rowItems); // add the channel if we have at least one programme
  //    /*m_channels.push_back()*/
  //  }
  //}
  //m_numChannels = (int)m_gridItems.size();
  //SetInvalid();  


bool CGUIEPGGridContainer::OnAction(const CAction &action)
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
    { 
      return true /*OnClick(action.wID)*/;
    }
  }
  return false;
}

bool CGUIEPGGridContainer::OnMessage(CGUIMessage& message)
{
  return CGUIControl::OnMessage(message);
}

void CGUIEPGGridContainer::OnUp()
{
  bool wrapAround = m_wrapAround;
  if (MoveUp(wrapAround))
    return;
  CGUIControl::OnUp();
}

void CGUIEPGGridContainer::OnDown()
{
  if (MoveDown(m_wrapAround))
    return;
  CGUIControl::OnDown();
}

void CGUIEPGGridContainer::OnLeft()
{
  if (MoveLeft(m_wrapAround))
    return;
  CGUIControl::OnLeft();
}

void CGUIEPGGridContainer::OnRight()
{
  if (MoveRight(m_wrapAround))
    return;
  CGUIControl::OnRight();
}

bool CGUIEPGGridContainer::MoveUp(bool wrapAround)
{
  if (m_curChannel > 0)
  {
    SetChannel(m_curChannel - 1);
  }
  else if (m_curChannel == 0 && m_chanOffset)
  {
    ScrollToOffset(m_chanOffset - m_channelsPerPage);
  }
  else if (m_wrapAround)
  {
    if (m_gridItems.size() > 0)
    {
      int offset = m_numChannels - m_channelsPerPage;
      if (offset < 0) offset = 0;
      SetChannel(m_numChannels - offset - 1);
      ScrollToOffset(offset);
    }
  }
  else
    return false;

  return true;
}

bool CGUIEPGGridContainer::MoveDown(bool wrapAround)
{
  if (m_chanOffset + m_curChannel + 1 < m_numChannels)
  {
    if (m_curChannel + 1 < m_channelsPerPage)
    {
      SetChannel(m_curChannel + 1);
    }
    else
    {
      ScrollToOffset(m_chanOffset + m_channelsPerPage);
    }
  }
  else if(wrapAround)
  { // move first item in list, and set our container moving in the "down" direction
    SetChannel(0);
    ScrollToOffset(0);
  }
  else
    return false;
  return true;
}
bool CGUIEPGGridContainer::MoveLeft(bool wrapAround)
{
  if (m_curItem > 0)
  {
    m_curItem--;
  }
  else if (m_curItem == 0 && m_chanOffset)
  {
    ScrollToOffset(m_chanOffset - m_channelsPerPage);
  }
  /*else if (m_wrapAround)
  {
    if (m_gridItems.size() > 0)
    {
      int offset = m_numChannels - m_channelsPerPage;
      if (offset < 0) offset = 0;
      SetChannel(m_numChannels - offset - 1);
      ScrollToOffset(offset);
    }
  }*/
  else
    return false;

  return true;
}
bool CGUIEPGGridContainer::MoveRight(bool wrapAround)
{
  CDateTime curItem;
  curItem.SetFromDBDateTime(m_gridItems[m_curChannel][m_curItem]->GetProperty("StartTime"));
  if (m_curItem < (int)m_gridItems[m_curChannel].size() - 1)
  {
    m_curItem++;
  }

  return true;
}

void CGUIEPGGridContainer::SetChannel(int channel)
{
  if (channel > m_channelsPerPage - 1) 
    channel = m_channelsPerPage - 1;
  if (channel < 0) 
    channel = 0;

  if (channel == m_curChannel)
    return;

  if (m_curItem == 0)
  {
    m_curChannel = channel;
    return;
  }

  CDateTime selectedDT;
  CDateTime targetDT;
  CDateTimeSpan targetLength;
  int item;

  if (!m_gridItems[m_curChannel][m_curItem])
    return;

  selectedDT.SetFromDBDateTime(m_gridItems[m_curChannel][m_curItem]->GetProperty("StartTime").c_str());
  for (item = 0; item < (int)m_gridItems[channel].size(); item++) // broken!!
  {
    if (!m_gridItems[channel][item])
      return;

    targetDT.SetFromDBDateTime(m_gridItems[channel][item]->GetProperty("StartTime").c_str());
    targetLength.SetDateTimeSpan(0, 0, 0, m_gridItems[channel][item]->GetPropertyInt("Duration"));

    if (targetDT == selectedDT)
    {
      break;
    }
    if (targetDT < selectedDT)
    {
       targetDT += targetLength;
       if (targetDT > selectedDT) 
         break; // target item overlaps the currently selected programme
    }
  }
  m_curChannel = channel;
  m_curItem    = item;
  CLog::Log(LOGDEBUG, "curChannel %u | chanOffset %u | curItem %u | itemOffset %u", m_curChannel, m_chanOffset, m_curItem, m_itemOffset);
}
void CGUIEPGGridContainer::Scroll(int amount)
{
}

int CGUIEPGGridContainer::GetSelectedItem() const
{
  return CorrectOffset(m_chanOffset, m_curChannel);
}

void CGUIEPGGridContainer::DoRender(DWORD currentTime)
{
  m_renderTime = currentTime;
  CGUIControl::DoRender(currentTime);
  m_wasReset = false;
}

void CGUIEPGGridContainer::ScrollToOffset(int offset)
{
  float size = m_layout->Size(VERTICAL);
  int range = m_channelsPerPage / 4;
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
  m_chanOffset = offset;
}

void CGUIEPGGridContainer::ValidateOffset()
{
  if (!m_layout) 
    return;
  if (m_chanOffset > m_numChannels - m_channelsPerPage)
  {
    m_chanOffset = m_numChannels - m_channelsPerPage;
    m_scrollOffset = m_chanOffset * m_layout->Size(VERTICAL);
  }
  if (m_chanOffset < 0)
  {
    m_chanOffset = 0;
    m_scrollOffset = 0;
  }
}
int CGUIEPGGridContainer::CorrectOffset(int offset, int cursor) const
{
  return offset + cursor;
}

void CGUIEPGGridContainer::LoadLayout(TiXmlElement *layout)
{
  TiXmlElement *itemElement = layout->FirstChildElement("itemlayout");
  while (itemElement)
  { // we have a new item layout
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, false);
    m_layout = new CGUIListItemLayout(itemLayout);
    itemElement = itemElement->NextSiblingElement("itemlayout");
  }
  itemElement = layout->FirstChildElement("focusedlayout");
  while (itemElement)
  { // we have a new item layout
    CGUIListItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, true);
    m_focusedLayout = new CGUIListItemLayout(itemLayout);
    itemElement = itemElement->NextSiblingElement("focusedlayout");
  }
}

void CGUIEPGGridContainer::UpdateLayout(bool updateAllItems)
{
  if (updateAllItems)
  { // free memory of items
    for (itChannels itC = m_gridItems.begin(); itC != m_gridItems.end(); itC++)
    {
      for (itShows itS = itC->begin(); itS != itC->end(); itS++)
      {
        (*itS)->FreeMemory();
      }
    }
  }
  // and recalculate the layout
  CalculateLayout();
}

void CGUIEPGGridContainer::CalculateLayout()
{
  // calculate the number of channels to display
  assert(m_focusedLayout && m_layout);
  if (!m_focusedLayout || !m_layout)
  {
    UpdateLayout(true);
    return;
  }
  m_channelsPerPage = (int)((m_height - m_focusedLayout->Size(VERTICAL)) / m_layout->Size(VERTICAL)) + 1;

  // ensure that the scroll offset is a multiple of our size
  m_scrollOffset = m_chanOffset * m_layout->Size(VERTICAL);
}

CStdString CGUIEPGGridContainer::GetDescription() const
{
  CStdString strLabel;
  unsigned item = GetSelectedItem();
  //if (item >= 0 && item < GetNumItems())
  //{
  //  /*CGUIListItem pItem = m_gridItems[0][item];*/
  //  //strLabel = pItem.GetLabel(); // get ptr
  //}
  return strLabel;
}

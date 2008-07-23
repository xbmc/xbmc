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
#include "GUIFontManager.h"

#define SHORTGAP     4
#define MINSPERBLOCK 5

CGUIEPGGridContainer::CGUIEPGGridContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, int scrollTime, int timeBlocks)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  ControlType = GUICONTAINER_EPGGRID;
  m_posX = posX;
  m_posY = posY;
  m_width = width;
  m_height = height;
  m_channel = 0;
  m_item = 0;
  m_block = 0;
  m_chanOffset = 0;
  m_blockOffset = 0;
  m_scrollTime = scrollTime ? scrollTime : 1;
  m_scrollOffset = 0;
  m_scrollSpeed = 0;
  m_channelsPerPage = 10;
  m_renderTime = 0;
  m_lastItem = NULL;
  m_blocksPerPage = timeBlocks;
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

  //g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height);
  /// disabled to check that griditems are correct size

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
      if (iC == m_channel)
        if(iS == m_item)
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

  RenderRuler();

  CGUIControl::Render();
  //g_graphicsContext.RestoreClipRegion();
}

void CGUIEPGGridContainer::RenderRuler()
{
  RESOLUTION res = g_graphicsContext.GetVideoResolution();
  g_graphicsContext.SetScalingResolution(res, 0, 0, false);

  CStdStringW wszText;

  wszText.Format(L"Item:%u, Block:%u, BlockOffset:%u, Channel:%u, ChannelOffset:%u", m_item, m_block, m_blockOffset, m_channel, m_chanOffset);

  float x = 0.30f * g_graphicsContext.GetWidth();
  float y = 0.96f * g_graphicsContext.GetHeight();
  CGUITextLayout::DrawOutlineText(g_fontManager.GetFont("font13"), x, y, 0xffffffff, 0xff000000, 2, wszText);
}
void CGUIEPGGridContainer::UpdateItems(EPGGrid &gridData, const CDateTime &start, const CDateTime &end)
{
  if (m_gridItems.size() > 0)
    m_gridItems.clear();

  float posX = m_posX;
  float posY = m_posY;
  float blockSize = m_width / m_blocksPerPage;

  CDateTime gridStart, gridEnd;
  CDateTimeSpan gridDuration, blockDuration, pageDuration;

  gridDuration = end - start;
  gridStart = start;
  pageDuration.SetDateTimeSpan(0, 0, MINSPERBLOCK*m_blocksPerPage, 0);
  gridEnd = start + pageDuration;

  m_blocks = (gridDuration.GetDays()*24*60 + gridDuration.GetHours()*60 + gridDuration.GetMinutes()) / MINSPERBLOCK;
  blockDuration.SetDateTimeSpan(0, 0, MINSPERBLOCK, 0);

  itEPGRow itY = gridData.begin();
  bool unknown = false;

  std::vector< int > rowIndex;
  std::vector< CGUIListItemPtr > rowItems;
  std::vector< CGUIListItemPtr > unknownItems;

  DWORD tick(timeGetTime());
  for (int y = 0 ; itY != gridData.end(); itY++, y++)
  {
    itEPGShow itX = itY->shows.begin();
    
    CDateTime gridCursor = gridStart;
    
    int itemSize = 0;
    int itemIdx = -1;
    int offset = 0;

    CGUIListItemPtr Item(new CGUIListItem());

    for (int x = 0; x < m_blocks; x++)
    {
      CDateTime itemStart, itemEnd;
      CDateTimeSpan itemDuration;
      itemStart.SetFromDBDateTime((*itX)->GetProperty("StartTime").c_str());
      itemDuration.SetDateTimeSpan(0, 0, 0, (*itX)->GetPropertyInt("Duration"));
      itemEnd = itemStart + itemDuration;

      if ((*itX) && gridCursor >= gridEnd)
      {
        /* give each ListItem it's own unique layout */
        CGUIListItemLayout *pItemLayout = new CGUIListItemLayout(*m_layout);
        CGUIListItemLayout *pItemFocusedLayout = new CGUIListItemLayout(*m_focusedLayout);

        pItemLayout->SetWidth(itemSize*blockSize);
        pItemFocusedLayout->SetWidth(itemSize*blockSize);
        Item->SetFocusedLayout(pItemFocusedLayout);
        Item->SetLayout(pItemLayout);

        if (itemEnd <= gridEnd)
        {
          itX++; // last program ended on the page edge, next programme
        }
        itemSize = 0;
      }

      if (!(*itX) || (gridCursor < itemStart)) // there's a gap in the schedule
      {
        if (unknown)
        {
          itemSize++;
          /// finalize layout for unknown item
        }
        else
        {
          Item = CGUIListItemPtr();
          Item->SetLabel("_?_");
          Item->SetProperty("Category", "Unknown");

          itemIdx++;
          unknown = true;
        }
      }
      else
      {
        if (Item == *itX)
        {
          // we've already found this programme, increment size of item in blocks
          itemSize++;
        }
        else
        {
          if (*itX != *itY->shows.end())
          Item = *itX; // Found a new programme
          rowItems.push_back(Item); // fill the channels vector
          itemSize = 1;
          itemIdx++;
          unknown = false;
        }
      }
      rowIndex.push_back(itemIdx);
      gridCursor += blockDuration; // add 5 minutes

      offset++;
      if (offset - m_blocksPerPage == 0)
      {
        // new page
        gridStart += pageDuration;
        gridEnd   += pageDuration;
        offset = 0;
      }
    }
    if (rowItems.size() > 0)
    {
      m_gridItems.push_back(rowItems);
      m_gridIndex.push_back(rowIndex);
    }
    rowItems.clear();
    rowIndex.clear();
  }


  if (m_gridItems.size() < 1)
  {
    CLog::Log(LOGDEBUG, "%s No data found", __FUNCTION__);
    return; 
  }

  CLog::Log(LOGDEBUG, "%s completed successfully in %u ms", __FUNCTION__, timeGetTime()-tick);
  m_channels = (int)m_gridItems.size();

  SetInvalid(); 
}

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
  if (m_channel > 0)
  {
    SetChannel(m_channel - 1);
  }
  else if (m_channel == 0 && m_chanOffset)
  {
    ScrollToChannelOffset(m_chanOffset - m_channelsPerPage);
  }
  else if (m_wrapAround)
  {
    if (m_gridItems.size() > 0)
    {
      int offset = m_channels - m_channelsPerPage;
      if (offset < 0) offset = 0;
      SetChannel(m_channels - offset - 1);
      ScrollToChannelOffset(offset);
    }
  }
  else
    return false;

  return true;
}

bool CGUIEPGGridContainer::MoveDown(bool wrapAround)
{
  int channel = m_channel + 1;
  if (m_chanOffset + channel < m_channels)
  {
    if (channel < m_channelsPerPage)
    {
      SetChannel(channel);
      
      /*m_block   = (GetBlock(m_item) - m_block) > (;*/
    }
    else
    {
      SetChannel(channel);
      ScrollToChannelOffset(m_chanOffset + m_channelsPerPage);
      m_channel = 0;
    }
  }
  else if(wrapAround)
  { // move first item in list, and set our container moving in the "down" direction
    SetChannel(0);
    ScrollToChannelOffset(0);
  }
  else
    return false;

  return true;
}
bool CGUIEPGGridContainer::MoveLeft(bool wrapAround)
{
  if (m_item != m_gridIndex[m_channel][m_blockOffset] )
  {
    // this is not first item on page
    m_block = GetBlock(--m_item, m_channel);
  }
  else if (m_block == 0 && m_blockOffset)
  {
    // at left edge and 2+ pages
    m_block = GetBlock(--m_item, m_channel);
    ScrollToBlockOffset(1);
  }
  else if (m_wrapAround)
  {
    if (m_gridItems.size() > 0)
    {
      int offset = m_blocks - m_blocksPerPage;
      if (offset < 0) offset = 0;
      /*SetItr(m_channels - offset - 1);*/
      ScrollToBlockOffset(offset);
    }
  }
  else
    return false;

  CLog::Log(LOGDEBUG, "Block: %u | Item: %u", m_block+m_blockOffset, m_item);
  return true;
}

bool CGUIEPGGridContainer::MoveRight(bool wrapAround)
{
  
  if ( m_item != m_gridIndex[m_channel][m_blocksPerPage + m_blockOffset] )
  {
    // this is not last item on page
    m_block = GetBlock(++m_item, m_channel);
  }
  CLog::Log(LOGDEBUG, "Block: %u | Item: %u", m_block+m_blockOffset, m_item);
  return true;
}

void CGUIEPGGridContainer::SetChannel(int channel)
{
  if (channel > m_channels - 1) 
    channel = m_channels - 1;
  if (channel < 0) 
    channel = 0;

  if (channel == m_channel)
    return;

  if (m_block == 0 || m_block == m_blocksPerPage)
  {
    m_channel = channel;
    m_item = GetItem(channel);
    m_block = GetBlock(m_item, channel);
    return;
  }

  // basic checks failed, need to correctly identify nearest item
  m_item = GetClosestItem(channel);
  m_channel = channel;
  m_block = GetBlock(m_item, m_channel);

  CLog::Log(LOGDEBUG, "*********************************");
  for (int i = 0; i < 36; i++)
  {
    CLog::Log(LOGDEBUG, "%u: %u", i, m_gridIndex[m_channel][i]);
  }

}
void CGUIEPGGridContainer::Scroll(int amount)
{

}


int CGUIEPGGridContainer::GetSelectedItem() const
{
  return CorrectOffset(m_chanOffset, m_channel);
}

int CGUIEPGGridContainer::GetClosestItem(const int &channel)
{

  int item = GetItem(channel);
  int block = GetBlock(item, channel) - m_blockOffset;
  int left, right;
  bool forward = true;

  if (block == m_block) // item & m_item start together
    return item;

  if (block > m_block)  // item starts after m_item
  {
    left = m_block - GetBlock(item - 1, channel);
    right = block - m_block;
  }
  else
  {
    left = m_block - block;
    right = GetBlock(item + 1, channel) - m_block;
  }

  if (right > SHORTGAP)
  {
    return item;
  }
  else
  {
    if (right < left) 
      return m_gridIndex[channel][block + right + m_blockOffset];
    else
      return m_gridIndex[channel][block - left  + m_blockOffset];
  }
}

int CGUIEPGGridContainer::GetBlock(const int &item, const int &channel)
{
  int i = 0;
  while (m_gridIndex[channel][i] != item)
    i++;
  return i;
}

int CGUIEPGGridContainer::GetItem(const int &channel)
{
  return m_gridIndex[channel][m_block+m_blockOffset];
}

void CGUIEPGGridContainer::DoRender(DWORD currentTime)
{
  m_renderTime = currentTime;
  CGUIControl::DoRender(currentTime);
  m_wasReset = false;
}

void CGUIEPGGridContainer::ScrollToChannelOffset(int offset)
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

void CGUIEPGGridContainer::ScrollToBlockOffset(int offset)
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
  if (m_chanOffset > m_channels - m_channelsPerPage)
  {
    m_chanOffset = m_channels - m_channelsPerPage;
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

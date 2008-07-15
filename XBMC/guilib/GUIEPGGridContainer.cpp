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

CGUIEPGGridContainer::CGUIEPGGridContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, int scrollTime)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  ControlType = GUICONTAINER_EPGGRID;
  m_posX = posX;
  m_posY = posY;
  m_width = width;
  m_height = height;
  m_cursor = 0;
  m_offset = 0;
  m_scrollTime = scrollTime ? scrollTime : 1;
  m_scrollOffset = 0;
  m_scrollSpeed = 0;
  m_channelsPerPage = 10;
  m_renderTime = 0;
  m_lastItem = NULL;
  m_layout = NULL;
  m_focusedLayout = NULL;
  m_wrapAround = true; /* get from settings */
}

CGUIEPGGridContainer::~CGUIEPGGridContainer(void)
{
}

void CGUIEPGGridContainer::RenderItem(float posX, float posY, CGUIEPGGridItem *item, bool focused)
{
  if (!m_focusedLayout || !m_layout) return;

  // set the origin
  g_graphicsContext.SetOrigin(posX, posY);

  if (m_bInvalidated)
    item->SetInvalid();
  if (focused)
  {
    item->SetFocusedLayout(m_focusedLayout);
    if (item->GetFocusedLayout())
    {
      if (item != m_lastItem || !HasFocus())
      {
        item->GetFocusedLayout()->ResetScrolling();
        item->GetFocusedLayout()->SetFocus(0);
      }
      if (item != m_lastItem && HasFocus())
      {
        item->GetFocusedLayout()->ResetAnimation(ANIM_TYPE_UNFOCUS);      
        unsigned int subItem = 1;
        if (m_lastItem && m_lastItem->GetFocusedLayout())
          subItem = m_lastItem->GetFocusedLayout()->GetFocus();
        item->GetFocusedLayout()->SetFocus(subItem ? subItem : 1);
      }
      item->GetFocusedLayout()->Render(item, m_dwParentID, m_renderTime);
    }
    m_lastItem = item;
  }
  else
  {
    //if (item->GetFocusedLayout())
    //  item->GetFocusedLayout()->SetFocus(0);  // focus is not set
    item->SetLayout(m_layout);
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

  /*m_scrollOffset*/

  g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height);
  int offset = 0;
  float posX = m_posX;
  float posY = m_posY;
  float focusedPosX = 0;
  float focusedPosY = 0;
  CGUIEPGGridItem* focusedItem;
  int current = offset;
  while (posX < m_posX + m_width && posY < m_posY + m_height && m_numChannels)
  {
    for (itChannels itC = m_gridItems.begin(); itC != m_gridItems.end(); itC++)
    {
      for (itShows itS = itC->begin(); itS != itC->end(); itC++)
      {
        CGUIEPGGridItem* item = *itS;
        bool focused = (current == m_offset + m_cursor);
        if (focused)
        {
          focusedPosX = posX;
          focusedPosY = posY;
          focusedItem = item;
        }
        else
          RenderItem(posX, posY, item, focused);

        // increment our horizontal position
        posX += focused ? m_focusedLayout->Size(VERTICAL) : m_layout->Size(VERTICAL);

        current++;
      }
    }
  }

  if (focusedItem)
    RenderItem(focusedPosX, focusedPosY, focusedItem, true);

  g_graphicsContext.RestoreClipRegion();
}

void CGUIEPGGridContainer::UpdateItems(EPGGrid &gridData)
{
  float posX = m_posX;
  float posY = m_posY;

  itEPGRow itY = gridData.begin();

  for ( ; itY != gridData.end(); itY++) /* for each channel */
  {
    itEPGShow itX = itY->shows.begin();
    std::vector< CGUIEPGGridItem* > channelRow;
    for ( ; itX != itY->shows.end(); itX++) /* for each unique broadcast */
    {
      CGUIEPGGridItem *pItem = new CGUIEPGGridItem();
      pItem->SetLabel((*itX)->GetLabel());
      pItem->SetShortDesc((*itX)->GetProperty("ShortDesc"));
      pItem->SetStartTime((*itX)->GetProperty("StartTime"));
      pItem->SetDuration((*itX)->GetPropertyInt("Duration"));
      pItem->SetLayout(m_layout); /* hack? */
      channelRow.push_back(pItem); /* fill the channels vector */
    }
    m_gridItems.push_back(channelRow); /* add the channel */
  }
  m_numChannels = (int)m_gridItems.size();
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
      return true;
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
  return true;
}
bool CGUIEPGGridContainer::MoveUp(bool wrapAround)
{
  if (m_cursor > 0)
  {
    SetCursor(m_cursor - 1);
  }
  else if (m_cursor == 0 && m_offset)
  {
    ScrollToOffset(m_offset -1);
  }
  else if (m_wrapAround)
  {
    /*if (m_epgItems.pEPGChannels.size() > 0)
    {
      int offset = m_epgItems.pEPGChannels.size() - m_channelsPerPage;
      if (offset < 0) offset = 0;
      SetCursor(m_epgItems.pEPGChannels.size() - offset - 1);
      ScrollToOffset(offset);
    }*/
  }
  else
    return false;
  return true;
}

bool CGUIEPGGridContainer::MoveDown(bool wrapAround)
{
  return true;
}
bool CGUIEPGGridContainer::MoveLeft(bool wrapAround)
{
  return true;
}
bool CGUIEPGGridContainer::MoveRight(bool wrapAround)
{
  return true;
}

void CGUIEPGGridContainer::SetCursor(int cursor)
{
  if (cursor > m_channelsPerPage - 1) cursor = m_channelsPerPage - 1;
  if (cursor < 0) cursor = 0;
  m_cursor = cursor;
}
void CGUIEPGGridContainer::Scroll(int amount)
{
}

int CGUIEPGGridContainer::GetSelectedItem() const
{
  return CorrectOffset(m_offset, m_cursor);
}

void CGUIEPGGridContainer::SaveStates(std::vector<CControlState> &states)
{
  states.push_back(CControlState(GetID(), GetSelectedItem()));
}

void CGUIEPGGridContainer::DoRender(DWORD currentTime)
{
  m_renderTime = currentTime;
  CGUIControl::DoRender(currentTime);
  m_wasReset = false;
}

void CGUIEPGGridContainer::ScrollToOffset(int offset)
{
  float size = 0;
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
  /*if (!m_wasReset)
    g_infoManager.SetContainerMoving(GetID(), offset - m_offset);*/
  m_offset = offset;
}

void CGUIEPGGridContainer::ValidateOffset()
{
  if (!m_layout) 
    return;
  if (m_offset > m_numChannels - m_channelsPerPage)
  {
    m_offset = m_numChannels - m_channelsPerPage;
    m_scrollOffset = m_offset * m_layout->Size(VERTICAL);
  }
  if (m_offset < 0)
  {
    m_offset = 0;
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
    CGUIEPGGridItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, false);
    m_layout = &itemLayout;
    itemElement = itemElement->NextSiblingElement("itemlayout");
  }
  itemElement = layout->FirstChildElement("focusedlayout");
  while (itemElement)
  { // we have a new item layout
    CGUIEPGGridItemLayout itemLayout;
    itemLayout.LoadLayout(itemElement, true);
    m_focusedLayout = &itemLayout;
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
  /*if (m_pageControl)
  {
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, GetRows());
    SendWindowMessage(msg);
  }*/
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
  m_scrollOffset = m_offset * m_layout->Size(VERTICAL);
}

CStdString CGUIEPGGridContainer::GetDescription() const
{
  CStdString strLabel;
  unsigned item = GetSelectedItem();
  if (item >= 0 && item < GetNumItems())
  {
    /*CGUIEPGGridItem pItem = m_gridItems[0][item];*/
    //strLabel = pItem.GetLabel(); // get ptr
  }
  return strLabel;
}
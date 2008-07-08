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
#include "GUIListItem.h"

CGUIEPGGridContainer::CGUIEPGGridContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime)
    : CGUIBaseContainer(dwParentID, dwControlId, posX, posY, width, height, orientation, scrollTime)
{
  ControlType = GUICONTAINER_EPGGRID;
  m_type = VIEW_TYPE_NONE;
  m_orientation = VERTICAL;
}

CGUIEPGGridContainer::~CGUIEPGGridContainer(void)
{
}

void CGUIEPGGridContainer::Render()
{
  ValidateOffset();

  if (m_bInvalidated)
    UpdateLayout();

  if (!m_layout || !m_focusedLayout) return;

  m_scrollOffset += m_scrollSpeed * (m_renderTime - m_scrollLastTime);
  if ((m_scrollSpeed < 0 && m_scrollOffset < m_offset * m_layout->Size(m_orientation)) ||
      (m_scrollSpeed > 0 && m_scrollOffset > m_offset * m_layout->Size(m_orientation)))
  {
    m_scrollOffset = m_offset * m_layout->Size(m_orientation);
    m_scrollSpeed = 0;
  }
  m_scrollLastTime = m_renderTime;

  int offset = (int)(m_scrollOffset / m_layout->Size(m_orientation));


}

bool CGUIEPGGridContainer::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_MOVE_LEFT:
  case ACTION_MOVE_RIGHT:
  case ACTION_MOVE_DOWN:
  case ACTION_MOVE_UP:
    {
      return true; // todo
    }
    break;
  case ACTION_PAGE_UP:
    {
      if (m_offset == 0)
      { // already on the first page, so move to the first item
        SetCursor(0);
      }
      else
      { // scroll up to the previous page
        Scroll( -m_itemsPerPage);
      }
      return true;
    }
    break;
  //case ACTION_PAGE_DOWN:
  //  {
  //    if (m_offset == (int)m_items.size() - m_itemsPerPage || (int)m_items.size() < m_itemsPerPage)
  //    { // already at the last page, so move to the last item.
  //      SetCursor(m_items.size() - m_offset - 1);
  //    }
  //    else
  //    { // scroll down to the next page
  //      Scroll(m_itemsPerPage);
  //    }
  //    return true;
  //  }
  //  break;
    // smooth scrolling (for analog controls)
  case ACTION_SCROLL_UP:
    {
      m_analogScrollCount += action.fAmount1 * action.fAmount1;
      bool handled = false;
      while (m_analogScrollCount > 0.4)
      {
        handled = true;
        m_analogScrollCount -= 0.4f;
        if (m_offset > 0 && m_cursor <= m_itemsPerPage / 2)
        {
          Scroll(-1);
        }
        else if (m_cursor > 0)
        {
          SetCursor(m_cursor - 1);
        }
      }
      return handled;
    }
    break;
  case ACTION_SCROLL_DOWN:
    {
      m_analogScrollCount += action.fAmount1 * action.fAmount1;
      bool handled = false;
      while (m_analogScrollCount > 0.4)
      {
        handled = true;
        m_analogScrollCount -= 0.4f;
        if (m_offset + m_itemsPerPage < (int)m_items.size() && m_cursor >= m_itemsPerPage / 2)
        {
          Scroll(1);
        }
        else if (m_cursor < m_itemsPerPage - 1 && m_offset + m_cursor < (int)m_items.size() - 1)
        {
          SetCursor(m_cursor + 1);
        }
      }
      return handled;
    }
    break;
  }
  return CGUIBaseContainer::OnAction(action);
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
  else if (wrapAround)
  {
    /*if (m_epgItems.pEPGChannels.size() > 0)
    {
      int offset = m_epgItems.pEPGChannels.size() - m_itemsPerPage;
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
  if (cursor > m_itemsPerPage - 1) cursor = m_itemsPerPage - 1;
  if (cursor < 0) cursor = 0;
  m_cursor = cursor;
}
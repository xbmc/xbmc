/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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

#include "GUIDragAndDropManager.h"
#include "input/MouseStat.h"
#include "FileItem.h"
#include "utils/Variant.h"


void CGUIDragAndDropManager::DraggingStart(CGUIListItemPtr draggedFileItem, int windowID) 
{ 
  DraggingStop();
  m_dndInfo = boost::shared_ptr<DragAndDropInfo>(new DragAndDropInfo());
  m_dndInfo->m_dragStartWindowID = windowID;
  m_dndInfo->m_draggedFileItem = draggedFileItem;
  g_Mouse.SetState(MOUSE_STATE_DRAG);
  if (m_dndInfo->m_draggedFileItem)
    m_dndInfo->m_draggedFileItem->SetProperty(ITEM_IS_DRAGGED_FLAG, true);
}

void CGUIDragAndDropManager::DraggingStop() 
{ 
  if (m_dndInfo)
  {
    if (m_dndInfo->m_draggedFileItem)
      m_dndInfo->m_draggedFileItem->ClearProperty(ITEM_IS_DRAGGED_FLAG);
  
    m_dndInfo = boost::shared_ptr<DragAndDropInfo>();
  }
  g_Mouse.SetState(MOUSE_STATE_NORMAL);
}

bool CGUIDragAndDropManager::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() != GUI_MSG_NOTIFY_ALL)
    return false;

  if (message.GetParam1() == GUI_DND_ITEM_START)
  {
    DraggingStart(message.GetItem(), message.GetSenderId());
    return true;
  }
  if (message.GetParam1() == GUI_DND_STOP)
  {
    DraggingStop();
    return true;
  }
  return false;
}

CGUIDragAndDropManager g_dragAndDropManager;

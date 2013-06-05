/*
 *      Copyright (C) 2005-2013 Team XBMC
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
#include "GUIListDragHandler.h"
#include "GUIInfoManager.h"
#include "GUIWindowManager.h"
#include "utils/Variant.h"

CGUIListDragHandler::CGUIListDragHandler(bool internal, 
                                         bool reorderable,
                                         bool dropable, 
                                         boost::shared_ptr<CGUIControl> dragHint, 
                                         CGUIBaseContainer *container)
: m_bInternal(internal),
  m_bReorderable(reorderable),
  //if this is a in-list-dragging and we are not reorderable, 
  //then we shouldn't accept the item, no matter what everyone else says
  m_bDropable((internal && !reorderable) ? false : dropable), 
  m_dragHintOffset((dragHint) ? CPoint(dragHint->GetXPosition(), dragHint->GetYPosition()) : CPoint()),
  m_dragHint(dragHint),
  m_container(container)
{
  ASSERT(m_container);
  
  m_draggedOrigPosition = -1;
  m_draggedNewPosition = -2;
  m_draggedScrollDirection = 0;
  m_draggedAway = false; 
}


CGUIListDragHandler::~CGUIListDragHandler()
{
  ClearDragHint();
}

void CGUIListDragHandler::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) //all public
{
    //Do we need to scroll?
  if(m_draggedScrollDirection && !m_container->m_scroller.IsScrolling())
  {
    m_container->Scroll(m_draggedScrollDirection);
    if(m_container->m_scroller.IsScrolling())
      m_container->SetCursor(m_container->GetCursor()-m_draggedScrollDirection); //our dragged item should be kept focused
  }
  if(m_bDropable)
  {
    //If we have a drag handle, we need to process it
    if(m_dragHint && m_bDropable)
    {
      m_dragHint->UpdateVisibility();
      unsigned int oldDirty = dirtyregions.size();
      m_dragHint->DoProcess(currentTime, dirtyregions);
      if (m_dragHint->IsVisible() || (oldDirty != dirtyregions.size())) // visible or dirty (was visible?)
        m_container->m_renderRegion.Union(m_dragHint->GetRenderRegion());
    }
  }
}


void CGUIListDragHandler::Render()
{
  if(m_dragHint && m_bDropable)
  {
    m_dragHint->DoRender();
  }
}

void CGUIListDragHandler::DragStart(const CPoint& point)
{
  if(m_bInternal)
  {
    //Store the item that the user wants to drag for later use
    CFileItemPtr draggedItem;
      // int selected = m_container->GetCursorFromPoint(point) + m_container->GetRealOffset() ;
    int selected = m_container->GetDropPositionFromPoint(point - CPoint(m_container->GetXPosition(), m_container->GetYPosition()));
    if (selected >= 0 && selected < (int)m_container->m_items.Size())
    {
      draggedItem = m_container->m_items[selected];
      m_draggedOrigPosition = m_draggedNewPosition = selected;
    }
      //Let the skinner have access to drag&drop info stuff
    g_infoManager.DraggingStart(draggedItem, m_container, g_windowManager.GetActiveWindow());
  }
  else //Dragging started earlier, but now we're finaly hovered!
  {
    if(!m_bDropable)
      return;
    
    CFileItemPtr draggedItem = g_infoManager.GetDraggedFileItem();
    
    if (!m_container->m_items.IsDropDuplicate(draggedItem)) //The user drags an item onto this list, that isn't already on this list ;)
    {
      if (m_bReorderable)
      {
        
        m_draggedNewPosition = getDroppedItem(point);
        if (m_dragHint)
        { //set draghint position
          DragHintInfo insertRect = m_container->GetDragHintInfo(m_draggedNewPosition);
          CalcDragHint(point, insertRect, m_draggedNewPosition);
          ShowDragHint();
          m_container->SetCursor(-1);
        }
        else 
        { //insert the item at the correct position
          AddExternalItem();
          m_container->SetCursor(m_draggedNewPosition -  m_container->GetRealOffset());
        }
      }
      else 
      { // we are not reorderable, so we need to sort the item in
        m_insertedCopy = CFileItemPtr(new CFileItem(*draggedItem));;
        m_insertedCopy->ClearProperty(ITEM_IS_DRAGGED_FLAG);
        m_insertedCopy->SetProperty(ITEM_IS_DROPPED_FLAG, CVariant(true));
        
          //TODO: this part could do with some major optimization
        m_container->m_items.Add(m_insertedCopy);      
        SortDescription sorting = SortUtils::TranslateOldSortMethod(m_container->m_items.GetSortMethod());
        sorting.sortOrder = m_container->m_items.GetSortOrder();
        m_container->m_items.Sort(sorting);
        
          //now find the position of our draggedItem
        m_draggedNewPosition = m_container->m_items.Find(draggedItem->GetPath());
        
        if (m_dragHint)
        {
          m_container->m_items.Remove(m_draggedNewPosition); //remove the item again... we only wanted to get the sorted position

          DragHintInfo insertRect = m_container->GetDragHintInfo(m_draggedNewPosition);
          m_dragHintPosition.x = insertRect.m_rect.x1;
          m_dragHintPosition.y = insertRect.m_rect.y1;
          ShowDragHint();
          
          m_container->SetCursor(-1);
        }
        else
          m_container->SetCursor(m_draggedNewPosition -  m_container->GetRealOffset());
      }
    }
    else 
    {
        //user tries to drag an item onto this list, that is already in this list!
      m_draggedOrigPosition = m_container->m_items.Find(m_container->m_items.CreateDropDummy(g_infoManager.GetDraggedFileItem())->GetPath());
      m_container->SetCursor(m_draggedOrigPosition - m_container->GetRealOffset());
      m_draggedNewPosition = getDroppedItem(point);
      if (m_bReorderable)
      {
        
        if (m_dragHint)
        { //set draghint position
          if (m_draggedNewPosition < m_draggedOrigPosition)
            m_draggedNewPosition++;
          
          DragHintInfo insertRect = m_container->GetDragHintInfo(m_draggedNewPosition);
          CalcDragHint(point, insertRect, m_draggedNewPosition);
          ShowDragHint();
          
          m_container->SetCursor(m_draggedOrigPosition - m_container->GetRealOffset());
        }
        else
        {
          m_container->MoveItemInternally(m_draggedOrigPosition, m_draggedNewPosition);
          m_container->SetCursor(m_draggedNewPosition - m_container->GetRealOffset());
        }
      }
      else 
      {
        m_container->SelectItem(m_draggedNewPosition);
      }
      m_bInternal = true;
      
    }
  }
}

EVENT_RESULT CGUIListDragHandler::DragMove(const CPoint &point)
{
  CRect insertPoint;
  int newPosition = getDroppedItem(point);
  

  if (m_bDropable)
  {
      //Let the skinner know, that we are currently the drop target
    g_infoManager.DragHover(m_container);
    
    if (newPosition<-1)
    { //The user is currently only hovering our border
      DraggedAway(); //This will make sure, we remove all visual hints etc...
      m_draggedNewPosition = -2;
    }
    else if (m_bReorderable)
    {
      if (m_dragHint) //we have a drag hint, so let's calculate it's position and  make it visible
      {
        m_draggedNewPosition = newPosition;

        DragHintInfo insertPoint = m_container->GetDragHintInfo(m_draggedNewPosition);
        CalcDragHint(point, insertPoint, m_draggedNewPosition);
        
        ShowDragHint();
      } 
      else //we don't have a drag hint, so lets reorder immediately
      {
        m_container->MoveItemInternally(m_draggedNewPosition, newPosition);
        m_container->SetCursor(newPosition - m_container->GetRealOffset());
        m_draggedNewPosition=newPosition;
      }      
    }
  }
 
  
  m_draggedScrollDirection = m_container->NeedsScrolling(point);
  
  return (m_bDropable) ? EVENT_RESULT_HANDLED : EVENT_RESULT_UNHANDLED;
} 

void CGUIListDragHandler::DraggedAway()
{
  if (m_bDropable)
  {
    if(!m_dragHint) //no drag hint, so the item was added or moved to our list... revert that
    {
      if (m_bInternal)
      { //move item back to it's original position
        m_container->MoveItemInternally(m_draggedNewPosition, m_draggedOrigPosition);
        m_draggedNewPosition = m_draggedOrigPosition;
        m_container->SetCursor(m_draggedNewPosition - m_container->GetRealOffset()); //focus the dragged object again
      }
      else 
      {
        if(m_draggedNewPosition >= 0 && m_draggedNewPosition < m_container->m_items.Size());
        {
          //remove item from our list
          if(!m_dragHint)
            m_container->m_items.Remove(m_draggedNewPosition);
          
          if(m_insertedCopy)
          {
            m_insertedCopy->ClearProperty(ITEM_IS_DROPPED_FLAG);
            m_insertedCopy = CFileItemPtr();
          }
        }
      }
    }
    else 
    {
      ClearDragHint();
    }
  }
  
  m_insertedCopy = CFileItemPtr();
  if(!m_bInternal)
  {
    m_draggedOrigPosition = -1;
    m_draggedNewPosition = -2;
  } 
  else
  {
    m_draggedNewPosition = m_draggedOrigPosition;
  }

  
  m_draggedAway = true;
  
    //Disable any scrolling
  m_draggedScrollDirection = 0;
}
  
EVENT_RESULT CGUIListDragHandler::OnDrop()
{
  
  //Disable any scrolling
  m_draggedScrollDirection = 0;
  
  if (m_bReorderable && m_bInternal)
  {
      //Valid positions from -1 (move item to the beginning of the list) to list.size
    if (m_draggedNewPosition>-2)  //make sure the item was dropped on our list and not on our border
    {
      if (m_dragHint) //Item is still at it's original position, so move it to it's destination
      {
        if (m_draggedNewPosition<m_draggedOrigPosition)
          ++m_draggedNewPosition;
        m_container->MoveItemInternally(m_draggedOrigPosition, m_draggedNewPosition);
        m_container->SetCursor(m_draggedNewPosition - m_container->GetRealOffset());
      }
      
      if (m_draggedNewPosition!=m_draggedOrigPosition) //Nothing to do, if there was no actual movement
      {
        if(m_container->m_items.OnDropMove(m_draggedOrigPosition, m_draggedNewPosition))
        {
            //the focused item might be set wrong after dragging is done...
          m_container->SetCursor(m_draggedNewPosition - m_container->GetRealOffset());
        }
        else //Dropping failed, let's revert our list changes
        {
          m_container->MoveItemInternally(m_draggedNewPosition, m_draggedOrigPosition);
          m_container->SetCursor(m_draggedOrigPosition - m_container->GetRealOffset());
        }

      }
        
    }
  }
  
  if (!m_bInternal && m_draggedNewPosition>-2 && m_bDropable)
  { 
    if (m_dragHint) //add our item!
    {
      AddExternalItem();
    }
    
    if(!m_container->m_items.OnDropAdd(m_insertedCopy, m_draggedNewPosition))
    {
      m_container->m_items.Remove(m_draggedNewPosition);
      m_container->SetCursor(-1);
    }
  }
  
  ClearDragHint();
  
  if (m_insertedCopy)
    m_insertedCopy->ClearProperty(ITEM_IS_DROPPED_FLAG);
  
    //Update the cache of our list
  if (m_container->m_items.CacheToDiscAlways())
    m_container->m_items.Save(g_windowManager.GetActiveWindow());
   
    //Notifie the skin, that dragging has stopped
  g_infoManager.DraggingStop();
  
  return EVENT_RESULT_HANDLED;
}
  

void CGUIListDragHandler::ClearDragHint()
{
  if (m_dragHint)
  {
    m_dragHint->SetVisible(false);
    m_dragHint->SetPosition(m_dragHintOffset.x, m_dragHintOffset.y);
  }
}

void CGUIListDragHandler::CalcDragHint(const CPoint& mouse, const DragHintInfo& hoveredArea, int& Pos)
{
  if (!m_dragHint)
    return;
  
  if (hoveredArea.m_orientation == VERTICAL)
  {
      //check if the drag hint is supposed to be above or under the item
    if(mouse.y < hoveredArea.m_rect.y1 + (hoveredArea.m_rect.y2-hoveredArea.m_rect.y1) / 2) 
    {
      if (m_bReorderable && Pos!=m_draggedOrigPosition)
        Pos--;
      m_dragHintPosition.y = hoveredArea.m_rect.y1;
    }
    else 
    {
      m_dragHintPosition.y = hoveredArea.m_rect.y2;
    }
    
    m_dragHintPosition.x = hoveredArea.m_rect.x1;
    
  }
  else
  {
      //check if the drag hint is supposed to be on the left, or right
    if (mouse.x < hoveredArea.m_rect.x1 + (hoveredArea.m_rect.x2-hoveredArea.m_rect.x1) / 2) 
    {
      if(m_bReorderable  && Pos!=m_draggedOrigPosition)
        Pos--;
      m_dragHintPosition.x = hoveredArea.m_rect.x1;
    }
    else 
    {
      m_dragHintPosition.x = hoveredArea.m_rect.x2;
    }
    m_dragHintPosition.y = hoveredArea.m_rect.y1;
  }
}


void CGUIListDragHandler::AddExternalItem()
{
  if (m_bInternal) 
    return;

  CFileItemPtr draggedItem = m_container->m_items.CreateDropDummy(g_infoManager.GetDraggedFileItem());
  
  m_insertedCopy = CFileItemPtr(new CFileItem(*draggedItem));
  m_insertedCopy->ClearProperty(ITEM_IS_DRAGGED_FLAG);
  m_insertedCopy->SetProperty(ITEM_IS_DROPPED_FLAG, CVariant(true));
  
  if (m_draggedNewPosition==-2)
  {
    m_draggedNewPosition = m_container->m_items.Size();
    m_container->m_items.Add(m_insertedCopy);
  }
  else
  {
    m_container->m_items.AddFront(m_insertedCopy, m_draggedNewPosition);
  }
  m_container->SetCursor(m_draggedNewPosition - m_container->GetRealOffset());
}

void CGUIListDragHandler::ShowDragHint()
{
  if (!m_dragHint)
    return; //Nothing to do
  
  if (!m_bInternal 
      || ( m_draggedNewPosition != m_draggedOrigPosition   //we don't add our item after itself
        && m_draggedNewPosition != m_draggedOrigPosition-1 //we don't add our item before itself
        && m_draggedNewPosition>-2                         //we have a valid position
        )) 
  {
    m_dragHint->SetVisible(true);
    m_dragHint->SetPosition(m_dragHintPosition.x+m_dragHintOffset.x, 
                            m_dragHintPosition.y+m_dragHintOffset.y);
    m_dragHint->SetInvalid();
  }
  else 
  {
    ClearDragHint();
  }
}

int CGUIListDragHandler::getDroppedItem(const CPoint& point)
{
  int result = m_container->GetDropPositionFromPoint(point - CPoint(m_container->GetXPosition(), m_container->GetYPosition()));
  if (result<0) //we are not hovering anything
  {
    if (m_container->OverEmptySpace(point)) //perhabs we are hovering empty space at the end of the list?
      result = m_container->GetNumItems()-1;
    else
      result = -2; //GetCursorFromPoint returns -1 if no item was found
                   //but for us, -1 means add it to the front of the list, 
                   //so we need to manually set it to -2
  }
  
  return result;
}

/*!
\file GUIListContainer.h
\brief
*/

#pragma once

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

#include "GUIBaseContainer.h"

struct DragHintInfo
{
  DragHintInfo(CRect rect, ORIENTATION orientation) : m_rect(rect), m_orientation(orientation) {}
  DragHintInfo() {}
  CRect m_rect;
  ORIENTATION m_orientation;
};

struct CGUIListDragHandler
{
  CGUIListDragHandler(bool internal, 
                      bool reorderable, 
                      bool dropable,
                      boost::shared_ptr<CGUIControl> dragHint, 
                      CGUIBaseContainer *container);
  ~CGUIListDragHandler();
    

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  /*! \brief Responsible for rendering the drag hint during drag&drop
   */
  void Render();
    
  /*! \brief Responsible for setting everything up at the start of drag&drop
   Will be called on the container where the dragging started, ans also on the container once they are
   hovered during drag&drop
   \param point current mouse position
   */
  void DragStart(const CPoint& point);
  /*! \brief Responsible for handling mouse movement during drag&drop
      \param point new mouse position
   */
  EVENT_RESULT DragMove(const CPoint &point);
  /**
   Called when the user no longer hovers this item during drag&drop
   NOTE: this function will not be called, when the user droped on us!
   **/
  void DraggedAway();
    
  /**
   This function will be called, when the user dropped the fileitem on us
   \returns EVENT_RESULT_HANDLED if it actually performed an action. EVENT_RESULT_UNHANDLED otherwise
   **/
  EVENT_RESULT OnDrop();
  
    //States that do not change during drag&drop
  bool m_bInternal;          //true: it is in-list-drag&drop, false: the users tries to drag an item from another list onto our list
  const bool m_bReorderable; //true: we use the mouse position to get the position where we want to drop the item
                             //false: we use the containers sort method to get the position
  const bool m_bDropable;    //true, when the dragged file item can be dropped on os
                             //always false, when m_binternal and !m_bReorderable
protected:
  const CPoint m_dragHintOffset;
  bool m_draggedAway;
  /*! \brief Adds the currently dragged FileItem on to our container at position m_draggedNewPosition
   */
  void AddExternalItem();
  /*! \brief Hides the drag hint */
  void ClearDragHint();
  /*! \brief Shows the drag hint.
   NOTE: has no effect if the skinner didn't define a drag hint for the container
   */
  void ShowDragHint();
  /*! \brief Calculates the position of the drag hint
   NOTE: the resulting position will be stored in m_dragHintPosition
   Algorithm:
   if(orientation==vertical)
    if(mouse is in top part of the bounding box)
      set position to the top left corner of the bounding box
    else
      set position to the bottom left corner
   
   \param mouse - current mouse position
   \param hoveredArea - the bounding box of the hovered item and the orientation the algorithm will use
   \param pos - index of the hovered element
   */
  void CalcDragHint(const CPoint& mouse, const DragHintInfo& hoveredArea, int& Pos);
  int getDroppedItem(const CPoint& point);
  
  const boost::shared_ptr<CGUIControl> m_dragHint;
  CGUIBaseContainer* const m_container;
  short m_draggedScrollDirection; 
  CPoint m_dragHintPosition;
  int m_draggedNewPosition; 
  int m_draggedOrigPosition; 
  CFileItemPtr m_insertedCopy;
};

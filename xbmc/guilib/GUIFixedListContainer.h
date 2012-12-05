/*!
\file GUIFixedListContainer.h
\brief
*/

#pragma once

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

/*!
 \ingroup controls
 \brief
 */
class CGUIFixedListContainer : public CGUIBaseContainer
{
public:
  CGUIFixedListContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems, int fixedPosition, int cursorRange);
  virtual ~CGUIFixedListContainer(void);
  virtual CGUIFixedListContainer *Clone() const { return new CGUIFixedListContainer(*this); };

  virtual bool OnAction(const CAction &action);

protected:
  virtual void Scroll(int amount);
  virtual bool MoveDown(bool wrapAround);
  virtual bool MoveUp(bool wrapAround);
  virtual bool GetOffsetRange(int &minOffset, int &maxOffset) const;
  virtual void ValidateOffset();
  virtual bool SelectItemFromPoint(const CPoint &point);
  virtual int GetCursorFromPoint(const CPoint &point, CPoint *itemPoint = NULL) const;
  virtual void SelectItem(int item);
  virtual bool HasNextPage() const;
  virtual bool HasPreviousPage() const;
  virtual int GetCurrentPage() const;

private:
  /*!
   \brief Get the minimal and maximal cursor positions in the list

   As the fixed list cursor position may vary at the ends of the list based
   on the cursor range, this function computes the minimal and maximal positions
   based on the number of items in the list
   \param minCursor the minimal cursor position
   \param maxCursor the maximal cursor position
   \sa m_fixedCursor, m_cursorRange
   */
  void GetCursorRange(int &minCursor, int &maxCursor) const;

  int m_fixedCursor;    ///< default position the skinner wishes to use for the focused item
  int m_cursorRange;    ///< range that the focused item can vary when at the ends of the list
};


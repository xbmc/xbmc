/*!
\file GUIListContainer.h
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
class CGUIWrappingListContainer : public CGUIBaseContainer
{
public:
  CGUIWrappingListContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems, int fixedPosition);
  virtual ~CGUIWrappingListContainer(void);
  virtual CGUIWrappingListContainer *Clone() const { return new CGUIWrappingListContainer(*this); };

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual int GetSelectedItem() const;

protected:
  virtual void Scroll(int amount);
  virtual bool MoveDown(bool wrapAround);
  virtual bool MoveUp(bool wrapAround);
  virtual bool GetOffsetRange(int &minOffset, int &maxOffset) const;
  virtual void ValidateOffset();
  virtual int  CorrectOffset(int offset, int cursor) const;
  virtual bool SelectItemFromPoint(const CPoint &point);
  virtual void SelectItem(int item);
  virtual void Reset();
  virtual unsigned int GetNumItems() const { return m_items.size() - m_extraItems; };
  virtual int GetCurrentPage() const;
  virtual void SetPageControlRange();
  virtual void UpdatePageControl(int offset);

  void ResetExtraItems();
  unsigned int m_extraItems;
};


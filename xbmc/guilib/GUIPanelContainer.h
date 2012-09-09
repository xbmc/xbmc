/*!
\file GUIPanelContainer.h
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
class CGUIPanelContainer : public CGUIBaseContainer
{
public:
  CGUIPanelContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems);
  virtual ~CGUIPanelContainer(void);
  virtual CGUIPanelContainer *Clone() const { return new CGUIPanelContainer(*this); };

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnLeft();
  virtual void OnRight();
  virtual void OnUp();
  virtual void OnDown();
  virtual bool GetCondition(int condition, int data) const;
protected:
  virtual bool MoveUp(bool wrapAround);
  virtual bool MoveDown(bool wrapAround);
  virtual bool MoveLeft(bool wrapAround);
  virtual bool MoveRight(bool wrapAround);
  virtual void Scroll(int amount);
  float AnalogScrollSpeed() const;
  virtual void ValidateOffset();
  virtual void CalculateLayout();
  unsigned int GetRows() const;
  virtual int  CorrectOffset(int offset, int cursor) const;
  virtual bool SelectItemFromPoint(const CPoint &point);
  virtual int GetCursorFromPoint(const CPoint &point, CPoint *itemPoint = NULL) const;
  void SetCursor(int cursor);
  virtual void SelectItem(int item);
  virtual bool HasPreviousPage() const;
  virtual bool HasNextPage() const;

  int m_itemsPerRow;
};


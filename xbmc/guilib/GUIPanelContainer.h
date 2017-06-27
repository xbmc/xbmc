/*!
\file GUIPanelContainer.h
\brief
*/

#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "GUIBaseContainer.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIPanelContainer : public CGUIBaseContainer
{
public:
  CGUIPanelContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems);
  ~CGUIPanelContainer(void) override;
  CGUIPanelContainer *Clone() const override { return new CGUIPanelContainer(*this); };

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  bool OnAction(const CAction &action) override;
  bool OnMessage(CGUIMessage& message) override;
  void OnLeft() override;
  void OnRight() override;
  void OnUp() override;
  void OnDown() override;
  bool GetCondition(int condition, int data) const override;
  std::string GetLabel(int info) const override;
protected:
  bool MoveUp(bool wrapAround) override;
  bool MoveDown(bool wrapAround) override;
  virtual bool MoveLeft(bool wrapAround);
  virtual bool MoveRight(bool wrapAround);
  void Scroll(int amount) override;
  float AnalogScrollSpeed() const;
  void ValidateOffset() override;
  void CalculateLayout() override;
  unsigned int GetRows() const override;
  int  CorrectOffset(int offset, int cursor) const override;
  bool SelectItemFromPoint(const CPoint &point) override;
  int GetCursorFromPoint(const CPoint &point, CPoint *itemPoint = NULL) const override;
  void SetCursor(int cursor) override;
  void SelectItem(int item) override;
  bool HasPreviousPage() const override;
  bool HasNextPage() const override;

  int GetCurrentRow() const;
  int GetCurrentColumn() const;

  int m_itemsPerRow;
};


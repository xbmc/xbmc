/*!
\file GUIListContainer.h
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
class CGUIWrappingListContainer : public CGUIBaseContainer
{
public:
  CGUIWrappingListContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems, int fixedPosition);
  ~CGUIWrappingListContainer(void) override;
  CGUIWrappingListContainer *Clone() const override { return new CGUIWrappingListContainer(*this); };

  bool OnAction(const CAction &action) override;
  bool OnMessage(CGUIMessage& message) override;
  int GetSelectedItem() const override;

protected:
  void Scroll(int amount) override;
  bool MoveDown(bool wrapAround) override;
  bool MoveUp(bool wrapAround) override;
  bool GetOffsetRange(int &minOffset, int &maxOffset) const override;
  void ValidateOffset() override;
  int  CorrectOffset(int offset, int cursor) const override;
  bool SelectItemFromPoint(const CPoint &point) override;
  void SelectItem(int item) override;
  void Reset() override;
  unsigned int GetNumItems() const override { return m_items.size() - m_extraItems; };
  int GetCurrentPage() const override;
  void SetPageControlRange() override;
  void UpdatePageControl(int offset) override;

  void ResetExtraItems();
  unsigned int m_extraItems;
};


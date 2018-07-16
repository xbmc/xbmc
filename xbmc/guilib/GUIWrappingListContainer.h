/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIListContainer.h
\brief
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
  size_t GetNumItems() const override { return m_items.size() - m_extraItems; };
  int GetCurrentPage() const override;
  void SetPageControlRange() override;
  void UpdatePageControl(int offset) override;

  void ResetExtraItems();
  unsigned int m_extraItems;
};


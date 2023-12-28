/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIPanelContainer.h
\brief
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
  CGUIPanelContainer* Clone() const override { return new CGUIPanelContainer(*this); }

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
  void ScrollToOffset(int offset) override;

  int GetCurrentRow() const;
  int GetCurrentColumn() const;

  int m_itemsPerRow;
};


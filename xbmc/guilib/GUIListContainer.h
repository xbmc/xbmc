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

class CLabelInfo;
class CTextureInfo;

/*!
 \ingroup controls
 \brief
 */
class CGUIListContainer : public CGUIBaseContainer
{
public:
  CGUIListContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems);
  explicit CGUIListContainer(const CGUIListContainer& other);
  //#ifdef GUILIB_PYTHON_COMPATIBILITY
  CGUIListContainer(int parentID, int controlID, float posX, float posY, float width, float height,
                         const CLabelInfo& labelInfo, const CLabelInfo& labelInfo2,
                         const CTextureInfo& textureButton, const CTextureInfo& textureButtonFocus,
                         float textureHeight, float itemWidth, float itemHeight, float spaceBetweenItems);
  //#endif
  ~CGUIListContainer(void) override;
  CGUIListContainer* Clone() const override { return new CGUIListContainer(*this); }

  bool OnAction(const CAction &action) override;
  bool OnMessage(CGUIMessage& message) override;

  bool HasNextPage() const override;
  bool HasPreviousPage() const override;

protected:
  void Scroll(int amount) override;
  void SetCursor(int cursor) override;
  bool MoveDown(bool wrapAround) override;
  bool MoveUp(bool wrapAround) override;
  void ValidateOffset() override;
  void SelectItem(int item) override;
  bool SelectItemFromPoint(const CPoint &point) override;
  int GetCursorFromPoint(const CPoint &point, CPoint *itemPoint = NULL) const override;
};


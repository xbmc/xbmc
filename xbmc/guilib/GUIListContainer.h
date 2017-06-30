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
class CGUIListContainer : public CGUIBaseContainer
{
public:
  CGUIListContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems);
//#ifdef GUILIB_PYTHON_COMPATIBILITY
  CGUIListContainer(int parentID, int controlID, float posX, float posY, float width, float height,
                         const CLabelInfo& labelInfo, const CLabelInfo& labelInfo2,
                         const CTextureInfo& textureButton, const CTextureInfo& textureButtonFocus,
                         float textureHeight, float itemWidth, float itemHeight, float spaceBetweenItems);
//#endif
  ~CGUIListContainer(void) override;
  CGUIListContainer *Clone() const override { return new CGUIListContainer(*this); };

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


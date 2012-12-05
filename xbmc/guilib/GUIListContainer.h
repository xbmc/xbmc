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
class CGUIListContainer : public CGUIBaseContainer
{
public:
  CGUIListContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems);
//#ifdef PRE_SKIN_VERSION_9_10_COMPATIBILITY
  CGUIListContainer(int parentID, int controlID, float posX, float posY, float width, float height,
                         const CLabelInfo& labelInfo, const CLabelInfo& labelInfo2,
                         const CTextureInfo& textureButton, const CTextureInfo& textureButtonFocus,
                         float textureHeight, float itemWidth, float itemHeight, float spaceBetweenItems);
//#endif
  virtual ~CGUIListContainer(void);
  virtual CGUIListContainer *Clone() const { return new CGUIListContainer(*this); };

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);

  virtual bool HasNextPage() const;
  virtual bool HasPreviousPage() const;

protected:
  virtual void Scroll(int amount);
  void SetCursor(int cursor);
  virtual bool MoveDown(bool wrapAround);
  virtual bool MoveUp(bool wrapAround);
  virtual void ValidateOffset();
  virtual void SelectItem(int item);
  virtual bool SelectItemFromPoint(const CPoint &point);
  virtual int GetCursorFromPoint(const CPoint &point, CPoint *itemPoint = NULL) const;
};


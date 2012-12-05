/*!
\file GUIListGroup.h
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

#include "GUIControlGroup.h"

/*!
 \ingroup controls
 \brief a group of controls within a list/panel container
 */
class CGUIListGroup : public CGUIControlGroup
{
public:
  CGUIListGroup(int parentID, int controlID, float posX, float posY, float width, float height);
  CGUIListGroup(const CGUIListGroup &right);
  virtual ~CGUIListGroup(void);
  virtual CGUIListGroup *Clone() const { return new CGUIListGroup(*this); };

  virtual void AddControl(CGUIControl *control, int position = -1);

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void ResetAnimation(ANIMATION_TYPE type);
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);
  virtual void UpdateInfo(const CGUIListItem *item);
  virtual void SetInvalid();

  void EnlargeWidth(float difference);
  void EnlargeHeight(float difference);
  void SetFocusedItem(unsigned int subfocus);
  unsigned int GetFocusedItem() const;
  bool MoveLeft();
  bool MoveRight();
  void SetState(bool selected, bool focused);
  void SelectItemFromPoint(const CPoint &point);

protected:
  const CGUIListItem *m_item;
};


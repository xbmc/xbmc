/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

/*!
\file GUIListGroup.h
\brief
*/

#include "GUIControlGroup.h"

/*!
 \ingroup controls
 \brief a group of controls within a list/panel container
 */
class CGUIListGroup final : public CGUIControlGroup
{
public:
  CGUIListGroup(int parentID, int controlID, float posX, float posY, float width, float height);
  CGUIListGroup(const CGUIListGroup &right);
  ~CGUIListGroup(void) override;
  CGUIListGroup *Clone() const override { return new CGUIListGroup(*this); };

  void AddControl(CGUIControl *control, int position = -1) override;

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void ResetAnimation(ANIMATION_TYPE type) override;
  void UpdateVisibility(const CGUIListItem *item = NULL) override;
  void UpdateInfo(const CGUIListItem *item) override;
  void SetInvalid() override;

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


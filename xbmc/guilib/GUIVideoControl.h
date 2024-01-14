/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIVideoControl.h
\brief
*/

#include "GUIControl.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIVideoControl :
      public CGUIControl
{
public:
  CGUIVideoControl(int parentID, int controlID, float posX, float posY, float width, float height);
  ~CGUIVideoControl(void) override;
  CGUIVideoControl* Clone() const override { return new CGUIVideoControl(*this); }

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  void RenderEx() override;
  EVENT_RESULT OnMouseEvent(const CPoint& point, const KODI::MOUSE::CMouseEvent& event) override;
  bool CanFocus() const override;
  bool CanFocusFromPoint(const CPoint &point) const override;
};


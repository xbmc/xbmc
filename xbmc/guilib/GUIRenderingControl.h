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

#include "GUIControl.h"

class IRenderingCallback;

class CGUIRenderingControl : public CGUIControl
{
public:
  CGUIRenderingControl(int parentID, int controlID, float posX, float posY, float width, float height);
  CGUIRenderingControl(const CGUIRenderingControl &from);
  CGUIRenderingControl *Clone() const override { return new CGUIRenderingControl(*this); }; //! @todo check for naughties

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  void UpdateVisibility(const CGUIListItem *item = NULL) override;
  void FreeResources(bool immediately = false) override;
  bool CanFocus() const override { return false; }
  bool CanFocusFromPoint(const CPoint &point) const override;
  bool InitCallback(IRenderingCallback *callback);

protected:
  CCriticalSection m_rendering;
  IRenderingCallback *m_callback;
};

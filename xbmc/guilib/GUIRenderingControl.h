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

#include "GUIControl.h"

class IRenderingCallback;

class CGUIRenderingControl : public CGUIControl
{
public:
  CGUIRenderingControl(int parentID, int controlID, float posX, float posY, float width, float height);
  CGUIRenderingControl(const CGUIRenderingControl &from);
  virtual CGUIRenderingControl *Clone() const { return new CGUIRenderingControl(*this); }; //! @todo check for naughties

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);
  virtual void FreeResources(bool immediately = false);
  virtual bool CanFocus() const { return false; }
  virtual bool CanFocusFromPoint(const CPoint &point) const;
  bool InitCallback(IRenderingCallback *callback);

protected:
  CCriticalSection m_rendering;
  IRenderingCallback *m_callback;
};

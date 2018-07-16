/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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

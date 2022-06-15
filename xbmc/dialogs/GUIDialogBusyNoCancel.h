/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

class CGUIDialogBusyNoCancel: public CGUIDialog
{
public:
  CGUIDialogBusyNoCancel(void);
  ~CGUIDialogBusyNoCancel(void) override;
  void DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;

protected:
  void Open_Internal(bool bProcessRenderLoop, const std::string& param = "") override;
  bool m_bLastVisible = false;
};

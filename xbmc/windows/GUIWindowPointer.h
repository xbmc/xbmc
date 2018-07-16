/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

class CGUIWindowPointer :
      public CGUIDialog
{
public:
  CGUIWindowPointer(void);
  ~CGUIWindowPointer(void) override;
  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
protected:
  void SetPointer(int pointer);
  void OnWindowLoaded() override;
  void UpdateVisibility() override;
private:
  int m_pointer;
  bool m_active;
};

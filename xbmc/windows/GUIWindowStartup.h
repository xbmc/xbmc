/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"

class CGUIWindowStartup :
      public CGUIWindow
{
public:
  CGUIWindowStartup(void);
  ~CGUIWindowStartup(void) override;
  bool OnAction(const CAction &action) override;

  // specialization of CGUIWindow
  void OnDeinitWindow(int nextWindowID) override;
};

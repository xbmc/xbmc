/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

class CGUIDialogSubMenu :
      public CGUIDialog
{
public:
  CGUIDialogSubMenu(int id = WINDOW_DIALOG_SUB_MENU, const std::string &xmlFile = "DialogSubMenu.xml");
  ~CGUIDialogSubMenu(void) override;
  bool OnMessage(CGUIMessage &message) override;
};

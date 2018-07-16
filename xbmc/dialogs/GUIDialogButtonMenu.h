/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

class CGUIDialogButtonMenu :
      public CGUIDialog
{
public:
  CGUIDialogButtonMenu(int id = WINDOW_DIALOG_BUTTON_MENU, const std::string &xmlFile = "DialogButtonMenu.xml");
  ~CGUIDialogButtonMenu(void) override;
  bool OnMessage(CGUIMessage &message) override;
  void FrameMove() override;
};

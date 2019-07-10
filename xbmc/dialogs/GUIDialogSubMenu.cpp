/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSubMenu.h"

#include "guilib/GUIMessage.h"

CGUIDialogSubMenu::CGUIDialogSubMenu(int id, const std::string &xmlFile)
    : CGUIDialog(id, xmlFile.c_str())
{
}

CGUIDialogSubMenu::~CGUIDialogSubMenu(void) = default;

bool CGUIDialogSubMenu::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    // someone has been clicked - deinit...
    CGUIDialog::OnMessage(message);
    Close();
    return true;
  }
  return CGUIDialog::OnMessage(message);
}

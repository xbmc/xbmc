/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogButtonMenu.h"

#include "guilib/GUIMessage.h"

#define CONTROL_BUTTON_LABEL  3100

CGUIDialogButtonMenu::CGUIDialogButtonMenu(int id, const std::string &xmlFile)
: CGUIDialog(id, xmlFile.c_str())
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogButtonMenu::~CGUIDialogButtonMenu(void) = default;

bool CGUIDialogButtonMenu::OnMessage(CGUIMessage &message)
{
  bool bRet = CGUIDialog::OnMessage(message);
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    // someone has been clicked - deinit...
    Close();
    return true;
  }
  return bRet;
}

void CGUIDialogButtonMenu::FrameMove()
{
  // get the active control, and put it's label into the label control
  const CGUIControl *pControl = GetFocusedControl();
  if (pControl && (pControl->GetControlType() == CGUIControl::GUICONTROL_BUTTON || pControl->GetControlType() == CGUIControl::GUICONTROL_TOGGLEBUTTON))
  {
    SET_CONTROL_LABEL(CONTROL_BUTTON_LABEL, pControl->GetDescription());
  }
  CGUIDialog::FrameMove();
}

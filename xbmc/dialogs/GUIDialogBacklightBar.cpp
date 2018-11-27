/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogBacklightBar.h"

#include "guilib/GUIMessage.h"
#include "input/Key.h"

namespace
{
constexpr int BACKLIGHT_BAR_DISPLAY_TIME{1000};
}

CGUIDialogBacklightBar::CGUIDialogBacklightBar()
  : CGUIDialog(WINDOW_DIALOG_BACKLIGHT_BAR, "DialogBacklightBar.xml", DialogModalityType::MODELESS)
{
  m_loadType = LOAD_ON_GUI_INIT;
}

bool CGUIDialogBacklightBar::OnAction(const CAction& action)
{
  if (action.GetID() == ACTION_BACKLIGHT_BRIGHTNESS_UP ||
      action.GetID() == ACTION_BACKLIGHT_BRIGHTNESS_DOWN)
  {
    // reset the timer, as we've changed the brightness level
    SetAutoClose(BACKLIGHT_BAR_DISPLAY_TIME);
    return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBacklightBar::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    case GUI_MSG_WINDOW_DEINIT:
      return CGUIDialog::OnMessage(message);
  }

  return false; // don't process anything other than what we need!
}

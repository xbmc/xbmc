/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameOSDHelp.h"

#include "DialogGameOSD.h"
#include "guilib/GUIMessage.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "utils/StringUtils.h"

using namespace KODI;
using namespace GAME;

namespace
{
constexpr char HELP_COMBO[] =
    "[B]$FEATURE[select,game.controller.snes] + $FEATURE[x,game.controller.default][/B]";
}

const int CDialogGameOSDHelp::CONTROL_ID_HELP_TEXT = 1101;

CDialogGameOSDHelp::CDialogGameOSDHelp(CDialogGameOSD& dialog) : m_dialog(dialog)
{
}

void CDialogGameOSDHelp::OnInitWindow()
{
  // Set help text
  // "Press {0:s} to open the menu."
  std::string helpText = StringUtils::Format(g_localizeStrings.Get(35235), HELP_COMBO);

  CGUIMessage msg(GUI_MSG_LABEL_SET, WINDOW_DIALOG_GAME_OSD, CONTROL_ID_HELP_TEXT);
  msg.SetLabel(helpText);
  m_dialog.OnMessage(msg);
}

bool CDialogGameOSDHelp::IsVisible()
{
  return IsVisible(CONTROL_ID_HELP_TEXT);
}

bool CDialogGameOSDHelp::IsVisible(int windowId)
{
  CGUIControl* control = m_dialog.GetControl(windowId);
  if (control != nullptr)
    return control->IsVisible();

  return false;
}

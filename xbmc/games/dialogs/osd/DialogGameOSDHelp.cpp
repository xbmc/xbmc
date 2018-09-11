/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameOSDHelp.h"
#include "DialogGameOSD.h"
#include "games/controllers/guicontrols/GUIGameController.h"
#include "games/GameServices.h"
#include "guilib/GUIMessage.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "utils/StringUtils.h"
#include "ServiceBroker.h"

using namespace KODI;
using namespace GAME;

const int CDialogGameOSDHelp::CONTROL_ID_HELP_TEXT = 1101;
const int CDialogGameOSDHelp::CONTROL_ID_GAME_CONTROLLER = 1102;

CDialogGameOSDHelp::CDialogGameOSDHelp(CDialogGameOSD &dialog) :
  m_dialog(dialog)
{
}

void CDialogGameOSDHelp::OnInitWindow()
{
  // Set help text
  //! @todo Define Select + X combo elsewhere
  // "Press {0:s} to open the menu."
  std::string helpText = StringUtils::Format(g_localizeStrings.Get(35235), "Select + X");

  CGUIMessage msg(GUI_MSG_LABEL_SET, WINDOW_DIALOG_GAME_OSD, CONTROL_ID_HELP_TEXT);
  msg.SetLabel(helpText);
  m_dialog.OnMessage(msg);

  // Set controller
  if (CServiceBroker::IsServiceManagerUp())
  {
    CGameServices& gameServices = CServiceBroker::GetGameServices();

    //! @todo Define SNES controller elsewhere
    ControllerPtr controller = gameServices.GetController("game.controller.snes");
    if (controller)
    {
      //! @todo Activate controller for all game controller controls
      CGUIGameController* guiController = dynamic_cast<CGUIGameController*>(m_dialog.GetControl(CONTROL_ID_GAME_CONTROLLER));
      if (guiController != nullptr)
        guiController->ActivateController(controller);
    }
  }
}

bool CDialogGameOSDHelp::IsVisible()
{
  return IsVisible(CONTROL_ID_HELP_TEXT) ||
         IsVisible(CONTROL_ID_GAME_CONTROLLER);
}

bool CDialogGameOSDHelp::IsVisible(int windowId)
{
  CGUIControl *control = m_dialog.GetControl(windowId);
  if (control != nullptr)
    return control->IsVisible();

  return false;
}

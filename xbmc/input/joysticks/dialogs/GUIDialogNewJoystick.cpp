/*
 *  Copyright (C) 2016-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogNewJoystick.h"

#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "messaging/helpers/DialogHelper.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

using namespace KODI;
using namespace JOYSTICK;

CGUIDialogNewJoystick::CGUIDialogNewJoystick() : CThread("NewJoystickDlg")
{
}

void CGUIDialogNewJoystick::ShowAsync()
{
  bool bShow = true;

  if (IsRunning())
    bShow = false;
  else if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
               CSettings::SETTING_INPUT_ASKNEWCONTROLLERS))
    bShow = false;
  else if (CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(
               WINDOW_DIALOG_GAME_CONTROLLERS, false))
    bShow = false;

  if (bShow)
    Create();
}

void CGUIDialogNewJoystick::Process()
{
  using namespace MESSAGING::HELPERS;

  // "New controller detected"
  // "A new controller has been detected. Configuration can be done at any time in "Settings ->
  // System Settings -> Input". Would you like to configure it now?"
  if (ShowYesNoDialogText(CVariant{35011}, CVariant{35012}) == DialogResponse::CHOICE_YES)
  {
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_DIALOG_GAME_CONTROLLERS);
  }
  else
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(
        CSettings::SETTING_INPUT_ASKNEWCONTROLLERS, false);
  }
}

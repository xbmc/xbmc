/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogNewJoystick.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "messaging/helpers/DialogHelper.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"

using namespace KODI;
using namespace JOYSTICK;

CGUIDialogNewJoystick::CGUIDialogNewJoystick() :
  CThread("NewJoystickDlg")
{
}

void CGUIDialogNewJoystick::ShowAsync(const std::string& deviceName)
{
  bool bShow = true;

  if (IsRunning())
    bShow = false;
  else if (!CServiceBroker::GetSettings()->GetBool(CSettings::SETTING_INPUT_ASKNEWCONTROLLERS))
    bShow = false;
  else if (CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_DIALOG_GAME_CONTROLLERS, false))
    bShow = false;

  if (bShow)
  {
    m_strDeviceName = deviceName;
    Create();
  }
}

void CGUIDialogNewJoystick::Process()
{
  using namespace MESSAGING::HELPERS;

  // "Unknown controller detected"
  // "Would you like to setup \"%s\"?"
  std::string dialogText = StringUtils::Format(g_localizeStrings.Get(35012).c_str(), m_strDeviceName.c_str());
  if (ShowYesNoDialogText(CVariant{ 35011 }, CVariant{ std::move(dialogText) }) == DialogResponse::YES)
  {
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_DIALOG_GAME_CONTROLLERS);
  }
  else
  {
    CServiceBroker::GetSettings()->SetBool(CSettings::SETTING_INPUT_ASKNEWCONTROLLERS, false);
  }
}

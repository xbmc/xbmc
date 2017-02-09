/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIDialogNewJoystick.h"
#include "ServiceBroker.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "messaging/helpers/DialogHelper.h"
#include "settings/Settings.h"

using namespace KODI;
using namespace JOYSTICK;

CGUIDialogNewJoystick::CGUIDialogNewJoystick() :
  CThread("NewJoystickDlg")
{
}

void CGUIDialogNewJoystick::ShowAsync()
{
  bool bShow = true;

  if (IsRunning())
    bShow = false;
  else if (!CServiceBroker::GetSettings().GetBool(CSettings::SETTING_INPUT_ASKNEWCONTROLLERS))
    bShow = false;
  else if (g_windowManager.IsWindowActive(WINDOW_DIALOG_GAME_CONTROLLERS, false))
    bShow = false;

  if (bShow)
    Create();
}

void CGUIDialogNewJoystick::Process()
{
  using namespace MESSAGING::HELPERS;

  // "New controller detected"
  // "A new controller has been detected. Configuration can be done at any time in "Settings -> System Settings -> Input". Would you like to configure it now?"
  if (ShowYesNoDialogText(CVariant{ 35011 }, CVariant{ 35012 }) == DialogResponse::YES)
  {
    g_windowManager.ActivateWindow(WINDOW_DIALOG_GAME_CONTROLLERS);
  }
  else
  {
    CServiceBroker::GetSettings().SetBool(CSettings::SETTING_INPUT_ASKNEWCONTROLLERS, false);
  }
}

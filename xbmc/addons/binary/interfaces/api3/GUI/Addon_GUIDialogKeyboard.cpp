/*
 *      Copyright (C) 2015-2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Addon_GUIDialogKeyboard.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/GUI/Addon_GUIDialogKeyboard.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"

using namespace ADDON;

namespace V3
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnDialog_Keyboard::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Dialogs.Keyboard.ShowAndGetInputWithHead           = V2::KodiAPI::GUI::CAddOnDialog_Keyboard::ShowAndGetInputWithHead;
  interfaces->GUI.Dialogs.Keyboard.ShowAndGetInput                   = V2::KodiAPI::GUI::CAddOnDialog_Keyboard::ShowAndGetInput;
  interfaces->GUI.Dialogs.Keyboard.ShowAndGetNewPasswordWithHead     = V2::KodiAPI::GUI::CAddOnDialog_Keyboard::ShowAndGetNewPasswordWithHead;
  interfaces->GUI.Dialogs.Keyboard.ShowAndGetNewPassword             = V2::KodiAPI::GUI::CAddOnDialog_Keyboard::ShowAndGetNewPassword;
  interfaces->GUI.Dialogs.Keyboard.ShowAndVerifyNewPasswordWithHead  = V2::KodiAPI::GUI::CAddOnDialog_Keyboard::ShowAndVerifyNewPasswordWithHead;
  interfaces->GUI.Dialogs.Keyboard.ShowAndVerifyNewPassword          = V2::KodiAPI::GUI::CAddOnDialog_Keyboard::ShowAndVerifyNewPassword;
  interfaces->GUI.Dialogs.Keyboard.ShowAndVerifyPassword             = V2::KodiAPI::GUI::CAddOnDialog_Keyboard::ShowAndVerifyPassword;
  interfaces->GUI.Dialogs.Keyboard.ShowAndGetFilter                  = V2::KodiAPI::GUI::CAddOnDialog_Keyboard::ShowAndGetFilter;
  interfaces->GUI.Dialogs.Keyboard.SendTextToActiveKeyboard          = V2::KodiAPI::GUI::CAddOnDialog_Keyboard::SendTextToActiveKeyboard;
  interfaces->GUI.Dialogs.Keyboard.isKeyboardActivated               = V2::KodiAPI::GUI::CAddOnDialog_Keyboard::isKeyboardActivated;
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V3 */

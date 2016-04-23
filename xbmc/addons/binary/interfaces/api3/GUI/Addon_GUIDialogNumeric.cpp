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

#include "Addon_GUIDialogNumeric.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/GUI/Addon_GUIDialogNumeric.h"
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

void CAddOnDialog_Numeric::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Dialogs.Numeric.ShowAndVerifyNewPassword      = V2::KodiAPI::GUI::CAddOnDialog_Numeric::ShowAndVerifyNewPassword;
  interfaces->GUI.Dialogs.Numeric.ShowAndVerifyPassword         = V2::KodiAPI::GUI::CAddOnDialog_Numeric::ShowAndVerifyPassword;
  interfaces->GUI.Dialogs.Numeric.ShowAndVerifyInput            = V2::KodiAPI::GUI::CAddOnDialog_Numeric::ShowAndVerifyInput;
  interfaces->GUI.Dialogs.Numeric.ShowAndGetTime                = V2::KodiAPI::GUI::CAddOnDialog_Numeric::ShowAndGetTime;
  interfaces->GUI.Dialogs.Numeric.ShowAndGetDate                = V2::KodiAPI::GUI::CAddOnDialog_Numeric::ShowAndGetDate;
  interfaces->GUI.Dialogs.Numeric.ShowAndGetIPAddress           = V2::KodiAPI::GUI::CAddOnDialog_Numeric::ShowAndGetIPAddress;
  interfaces->GUI.Dialogs.Numeric.ShowAndGetNumber              = V2::KodiAPI::GUI::CAddOnDialog_Numeric::ShowAndGetNumber;
  interfaces->GUI.Dialogs.Numeric.ShowAndGetSeconds             = V2::KodiAPI::GUI::CAddOnDialog_Numeric::ShowAndGetSeconds;
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V3 */

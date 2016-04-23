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

#include "Addon_GUIControlEdit.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/GUI/Addon_GUIControlEdit.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"

namespace V3
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnControl_Edit::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Control.Edit.SetVisible              = V2::KodiAPI::GUI::CAddOnControl_Edit::SetVisible;
  interfaces->GUI.Control.Edit.SetEnabled              = V2::KodiAPI::GUI::CAddOnControl_Edit::SetEnabled;
  interfaces->GUI.Control.Edit.SetLabel                = V2::KodiAPI::GUI::CAddOnControl_Edit::SetLabel;
  interfaces->GUI.Control.Edit.GetLabel                = V2::KodiAPI::GUI::CAddOnControl_Edit::GetLabel;
  interfaces->GUI.Control.Edit.SetText                 = V2::KodiAPI::GUI::CAddOnControl_Edit::SetText;
  interfaces->GUI.Control.Edit.GetText                 = V2::KodiAPI::GUI::CAddOnControl_Edit::GetText;
  interfaces->GUI.Control.Edit.SetCursorPosition       = V2::KodiAPI::GUI::CAddOnControl_Edit::SetCursorPosition;
  interfaces->GUI.Control.Edit.GetCursorPosition       = V2::KodiAPI::GUI::CAddOnControl_Edit::GetCursorPosition;
  interfaces->GUI.Control.Edit.SetInputType            = V2::KodiAPI::GUI::CAddOnControl_Edit::SetInputType;
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V3 */

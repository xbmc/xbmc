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

#include "Addon_GUIControlRadioButton.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/GUI/Addon_GUIControlRadioButton.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"

namespace V3
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnControl_RadioButton::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Control.RadioButton.SetVisible   = V2::KodiAPI::GUI::CAddOnControl_RadioButton::SetVisible;
  interfaces->GUI.Control.RadioButton.SetEnabled   = V2::KodiAPI::GUI::CAddOnControl_RadioButton::SetEnabled;

  interfaces->GUI.Control.RadioButton.SetLabel     = V2::KodiAPI::GUI::CAddOnControl_RadioButton::SetLabel;
  interfaces->GUI.Control.RadioButton.GetLabel     = V2::KodiAPI::GUI::CAddOnControl_RadioButton::GetLabel;

  interfaces->GUI.Control.RadioButton.SetSelected  = V2::KodiAPI::GUI::CAddOnControl_RadioButton::SetSelected;
  interfaces->GUI.Control.RadioButton.IsSelected   = V2::KodiAPI::GUI::CAddOnControl_RadioButton::IsSelected;
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V3 */

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

#include "Addon_GUIControlSpin.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/GUI/Addon_GUIControlSpin.h"
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

void CAddOnControl_Spin::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Control.Spin.SetVisible        = V2::KodiAPI::GUI::CAddOnControl_Spin::SetVisible;
  interfaces->GUI.Control.Spin.SetEnabled        = V2::KodiAPI::GUI::CAddOnControl_Spin::SetEnabled;

  interfaces->GUI.Control.Spin.SetText           = V2::KodiAPI::GUI::CAddOnControl_Spin::SetText;
  interfaces->GUI.Control.Spin.Reset             = V2::KodiAPI::GUI::CAddOnControl_Spin::Reset;
  interfaces->GUI.Control.Spin.SetType           = V2::KodiAPI::GUI::CAddOnControl_Spin::SetType;

  interfaces->GUI.Control.Spin.AddStringLabel    = V2::KodiAPI::GUI::CAddOnControl_Spin::AddStringLabel;
  interfaces->GUI.Control.Spin.SetStringValue    = V2::KodiAPI::GUI::CAddOnControl_Spin::SetStringValue;
  interfaces->GUI.Control.Spin.GetStringValue    = V2::KodiAPI::GUI::CAddOnControl_Spin::GetStringValue;

  interfaces->GUI.Control.Spin.AddIntLabel       = V2::KodiAPI::GUI::CAddOnControl_Spin::AddIntLabel;
  interfaces->GUI.Control.Spin.SetIntRange       = V2::KodiAPI::GUI::CAddOnControl_Spin::SetIntRange;
  interfaces->GUI.Control.Spin.SetIntValue       = V2::KodiAPI::GUI::CAddOnControl_Spin::SetIntValue;
  interfaces->GUI.Control.Spin.GetIntValue       = V2::KodiAPI::GUI::CAddOnControl_Spin::GetIntValue;

  interfaces->GUI.Control.Spin.SetFloatRange     = V2::KodiAPI::GUI::CAddOnControl_Spin::SetFloatRange;
  interfaces->GUI.Control.Spin.SetFloatValue     = V2::KodiAPI::GUI::CAddOnControl_Spin::SetFloatValue;
  interfaces->GUI.Control.Spin.GetFloatValue     = V2::KodiAPI::GUI::CAddOnControl_Spin::GetFloatValue;
  interfaces->GUI.Control.Spin.SetFloatInterval  = V2::KodiAPI::GUI::CAddOnControl_Spin::SetFloatInterval;
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V3 */

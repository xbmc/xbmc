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

#include "Addon_GUIControlSlider.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/GUI/Addon_GUIControlSlider.h"
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

void CAddOnControl_Slider::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Control.Slider.SetVisible        = V2::KodiAPI::GUI::CAddOnControl_Slider::SetVisible;
  interfaces->GUI.Control.Slider.SetEnabled        = V2::KodiAPI::GUI::CAddOnControl_Slider::SetEnabled;

  interfaces->GUI.Control.Slider.Reset             = V2::KodiAPI::GUI::CAddOnControl_Slider::Reset;
  interfaces->GUI.Control.Slider.GetDescription    = V2::KodiAPI::GUI::CAddOnControl_Slider::GetDescription;

  interfaces->GUI.Control.Slider.SetIntRange       = V2::KodiAPI::GUI::CAddOnControl_Slider::SetIntRange;
  interfaces->GUI.Control.Slider.SetIntValue       = V2::KodiAPI::GUI::CAddOnControl_Slider::SetIntValue;
  interfaces->GUI.Control.Slider.GetIntValue       = V2::KodiAPI::GUI::CAddOnControl_Slider::GetIntValue;
  interfaces->GUI.Control.Slider.SetIntInterval    = V2::KodiAPI::GUI::CAddOnControl_Slider::SetIntInterval;

  interfaces->GUI.Control.Slider.SetPercentage     = V2::KodiAPI::GUI::CAddOnControl_Slider::SetPercentage;
  interfaces->GUI.Control.Slider.GetPercentage     = V2::KodiAPI::GUI::CAddOnControl_Slider::GetPercentage;

  interfaces->GUI.Control.Slider.SetFloatRange     = V2::KodiAPI::GUI::CAddOnControl_Slider::SetFloatRange;
  interfaces->GUI.Control.Slider.SetFloatValue     = V2::KodiAPI::GUI::CAddOnControl_Slider::SetFloatValue;
  interfaces->GUI.Control.Slider.GetFloatValue     = V2::KodiAPI::GUI::CAddOnControl_Slider::GetFloatValue;
  interfaces->GUI.Control.Slider.SetFloatInterval  = V2::KodiAPI::GUI::CAddOnControl_Slider::SetFloatInterval;
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V3 */

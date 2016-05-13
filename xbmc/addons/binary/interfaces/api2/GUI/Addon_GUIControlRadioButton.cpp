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

#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "guilib/GUIRadioButtonControl.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnControl_RadioButton::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Control.RadioButton.SetVisible   = CAddOnControl_RadioButton::SetVisible;
  interfaces->GUI.Control.RadioButton.SetEnabled   = CAddOnControl_RadioButton::SetEnabled;

  interfaces->GUI.Control.RadioButton.SetLabel     = CAddOnControl_RadioButton::SetLabel;
  interfaces->GUI.Control.RadioButton.GetLabel     = CAddOnControl_RadioButton::GetLabel;

  interfaces->GUI.Control.RadioButton.SetSelected  = CAddOnControl_RadioButton::SetSelected;
  interfaces->GUI.Control.RadioButton.IsSelected   = CAddOnControl_RadioButton::IsSelected;
}

void CAddOnControl_RadioButton::SetVisible(void *addonData, void* handle, bool visible)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_RadioButton - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIRadioButtonControl*>(handle)->SetVisible(visible);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_RadioButton::SetEnabled(void *addonData, void* handle, bool enabled)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_RadioButton - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIRadioButtonControl*>(handle)->SetEnabled(enabled);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_RadioButton::SetLabel(void *addonData, void* handle, const char *label)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_RadioButton - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIRadioButtonControl*>(handle)->SetLabel(label);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_RadioButton::GetLabel(void *addonData, void* handle, char &text, unsigned int &iMaxStringSize)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_RadioButton - %s - invalid handler data", __FUNCTION__);

    CGUIRadioButtonControl* pRadioButton = static_cast<CGUIRadioButtonControl *>(handle);
    strncpy(&text, pRadioButton->GetLabel().c_str(), iMaxStringSize);
    iMaxStringSize = pRadioButton->GetLabel().length();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_RadioButton::SetSelected(void *addonData, void* handle, bool selected)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_RadioButton - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIRadioButtonControl*>(handle)->SetSelected(selected);
  }
  HANDLE_ADDON_EXCEPTION
}

bool CAddOnControl_RadioButton::IsSelected(void *addonData, void* handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_RadioButton - %s - invalid handler data", __FUNCTION__);

    return static_cast<CGUIRadioButtonControl *>(handle)->IsSelected();
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */

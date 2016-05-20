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

#include "Addon_GUIControlButton.h"

#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "guilib/GUIButtonControl.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnControl_Button::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Control.Button.SetVisible = V2::KodiAPI::GUI::CAddOnControl_Button::SetVisible;
  interfaces->GUI.Control.Button.SetEnabled = V2::KodiAPI::GUI::CAddOnControl_Button::SetEnabled;

  interfaces->GUI.Control.Button.SetLabel = V2::KodiAPI::GUI::CAddOnControl_Button::SetLabel;
  interfaces->GUI.Control.Button.GetLabel = V2::KodiAPI::GUI::CAddOnControl_Button::GetLabel;

  interfaces->GUI.Control.Button.SetLabel2 = V2::KodiAPI::GUI::CAddOnControl_Button::SetLabel2;
  interfaces->GUI.Control.Button.GetLabel2 = V2::KodiAPI::GUI::CAddOnControl_Button::GetLabel2;
}

void CAddOnControl_Button::SetVisible(void *addonData, void* handle, bool yesNo)
{
  try
  {
    if (handle)
      static_cast<CGUIButtonControl*>(handle)->SetVisible(yesNo);
    else
      throw ADDON::WrongValueException("CAddOnControl_Button - %s - invalid handler data", __FUNCTION__);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Button::SetEnabled(void *addonData, void* handle, bool yesNo)
{
  try
  {
    if (handle)
      static_cast<CGUIButtonControl*>(handle)->SetEnabled(yesNo);
    else
      throw ADDON::WrongValueException("CAddOnControl_Button - %s - invalid handler data", __FUNCTION__);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Button::SetLabel(void *addonData, void* handle, const char *label)
{
  try
  {
    if (handle)
      static_cast<CGUIButtonControl *>(handle)->SetLabel(label);
    else
      throw ADDON::WrongValueException("CAddOnControl_Button - %s - invalid handler data", __FUNCTION__);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Button::GetLabel(void *addonData, void* handle, char &label, unsigned int &iMaxStringSize)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Button - %s - invalid handler data", __FUNCTION__);

    CGUIButtonControl* pButton = static_cast<CGUIButtonControl *>(handle);
    std::string text = pButton->GetLabel();
    strncpy(&label, text.c_str(), iMaxStringSize);
    iMaxStringSize = text.length();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Button::SetLabel2(void *addonData, void* handle, const char *label)
{
  try
  {
    if (handle)
      static_cast<CGUIButtonControl *>(handle)->SetLabel2(label);
    else
      throw ADDON::WrongValueException("CAddOnControl_Button - %s - invalid handler data", __FUNCTION__);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Button::GetLabel2(void *addonData, void* handle, char &label, unsigned int &iMaxStringSize)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Button - %s - invalid handler data", __FUNCTION__);

    CGUIButtonControl* pButton = static_cast<CGUIButtonControl *>(handle);
    std::string text = pButton->GetLabel2();
    strncpy(&label, text.c_str(), iMaxStringSize);
    iMaxStringSize = text.length();
  }
  HANDLE_ADDON_EXCEPTION
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */

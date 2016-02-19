/*
 *      Copyright (C) 2015 Team KODI
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

#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/callbacks/AddonCallbacks.h"
#include "addons/binary/callbacks/api2/AddonCallbacksBase.h"
#include "guilib/GUISliderControl.h"
#include "guilib/GUIWindowManager.h"

#include "AddonGUIControlSlider.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnControl_Slider::Init(::V2::KodiAPI::CB_AddOnLib *callbacks)
{
  callbacks->GUI.Control.Slider.SetVisible        = CAddOnControl_Slider::SetVisible;
  callbacks->GUI.Control.Slider.SetEnabled        = CAddOnControl_Slider::SetEnabled;

  callbacks->GUI.Control.Slider.Reset             = CAddOnControl_Slider::Reset;
  callbacks->GUI.Control.Slider.GetDescription    = CAddOnControl_Slider::GetDescription;

  callbacks->GUI.Control.Slider.SetIntRange       = CAddOnControl_Slider::SetIntRange;
  callbacks->GUI.Control.Slider.SetIntValue       = CAddOnControl_Slider::SetIntValue;
  callbacks->GUI.Control.Slider.GetIntValue       = CAddOnControl_Slider::GetIntValue;
  callbacks->GUI.Control.Slider.SetIntInterval    = CAddOnControl_Slider::SetIntInterval;

  callbacks->GUI.Control.Slider.SetPercentage     = CAddOnControl_Slider::SetPercentage;
  callbacks->GUI.Control.Slider.GetPercentage     = CAddOnControl_Slider::GetPercentage;

  callbacks->GUI.Control.Slider.SetFloatRange     = CAddOnControl_Slider::SetFloatRange;
  callbacks->GUI.Control.Slider.SetFloatValue     = CAddOnControl_Slider::SetFloatValue;
  callbacks->GUI.Control.Slider.GetFloatValue     = CAddOnControl_Slider::GetFloatValue;
  callbacks->GUI.Control.Slider.SetFloatInterval  = CAddOnControl_Slider::SetFloatInterval;
}

void CAddOnControl_Slider::SetVisible(void *addonData, GUIHANDLE handle, bool visible)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Slider - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISliderControl*>(handle)->SetVisible(visible);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Slider::SetEnabled(void *addonData, GUIHANDLE handle, bool enabled)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Slider - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISliderControl*>(handle)->SetEnabled(enabled);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Slider::Reset(void *addonData, GUIHANDLE handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Slider - %s - invalid handler data", __FUNCTION__);

    CGUISliderControl* pControl = static_cast<CGUISliderControl*>(handle);

    CGUIMessage msg(GUI_MSG_LABEL_RESET, pControl->GetParentID(), pControl->GetID());
    g_windowManager.SendThreadMessage(msg, pControl->GetParentID());
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Slider::GetDescription(void *addonData, GUIHANDLE handle, char &label, unsigned int &iMaxStringSize)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Slider - %s - invalid handler data", __FUNCTION__);

    std::string text = static_cast<CGUISliderControl*>(handle)->GetDescription();
    strncpy(&label, text.c_str(), iMaxStringSize);
    iMaxStringSize = text.length();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Slider::SetIntRange(void *addonData, GUIHANDLE handle, int iStart, int iEnd)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Slider - %s - invalid handler data", __FUNCTION__);

    CGUISliderControl* pControl = static_cast<CGUISliderControl *>(handle);
    pControl->SetType(SLIDER_CONTROL_TYPE_INT);
    pControl->SetRange(iStart, iEnd);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Slider::SetIntValue(void *addonData, GUIHANDLE handle, int iValue)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Slider - %s - invalid handler data", __FUNCTION__);

    CGUISliderControl* pControl = static_cast<CGUISliderControl *>(handle);
    pControl->SetType(SLIDER_CONTROL_TYPE_INT);
    pControl->SetIntValue(iValue);
  }
  HANDLE_ADDON_EXCEPTION
}

int CAddOnControl_Slider::GetIntValue(void *addonData, GUIHANDLE handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Slider - %s - invalid handler data", __FUNCTION__);

    return static_cast<CGUISliderControl*>(handle)->GetIntValue();
  }
  HANDLE_ADDON_EXCEPTION

  return 0;
}

void CAddOnControl_Slider::SetIntInterval(void *addonData, GUIHANDLE handle, int iInterval)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Slider - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISliderControl*>(handle)->SetIntInterval(iInterval);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Slider::SetPercentage(void *addonData, GUIHANDLE handle, float fPercent)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Slider - %s - invalid handler data", __FUNCTION__);

    CGUISliderControl* pControl = static_cast<CGUISliderControl *>(handle);
    pControl->SetType(SLIDER_CONTROL_TYPE_PERCENTAGE);
    pControl->SetPercentage(fPercent);
  }
  HANDLE_ADDON_EXCEPTION
}

float CAddOnControl_Slider::GetPercentage(void *addonData, GUIHANDLE handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Slider - %s - invalid handler data", __FUNCTION__);

    return static_cast<CGUISliderControl*>(handle)->GetPercentage();
  }
  HANDLE_ADDON_EXCEPTION

  return 0.0f;
}

void CAddOnControl_Slider::SetFloatRange(void *addonData, GUIHANDLE handle, float fStart, float fEnd)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Slider - %s - invalid handler data", __FUNCTION__);

    CGUISliderControl* pControl = static_cast<CGUISliderControl *>(handle);
    pControl->SetType(SLIDER_CONTROL_TYPE_FLOAT);
    pControl->SetFloatRange(fStart, fEnd);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Slider::SetFloatValue(void *addonData, GUIHANDLE handle, float iValue)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Slider - %s - invalid handler data", __FUNCTION__);

    CGUISliderControl* pControl = static_cast<CGUISliderControl *>(handle);
    pControl->SetType(SLIDER_CONTROL_TYPE_FLOAT);
    pControl->SetFloatValue(iValue);
  }
  HANDLE_ADDON_EXCEPTION
}

float CAddOnControl_Slider::GetFloatValue(void *addonData, GUIHANDLE handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Slider - %s - invalid handler data", __FUNCTION__);

    return static_cast<CGUISliderControl*>(handle)->GetFloatValue();
  }
  HANDLE_ADDON_EXCEPTION

  return 0.0f;
}

void CAddOnControl_Slider::SetFloatInterval(void *addonData, GUIHANDLE handle, float fInterval)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Slider - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISliderControl*>(handle)->SetFloatInterval(fInterval);
  }
  HANDLE_ADDON_EXCEPTION
}

}; /* extern "C" */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */

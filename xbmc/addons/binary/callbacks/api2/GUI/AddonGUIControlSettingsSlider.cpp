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
#include "guilib/GUISettingsSliderControl.h"
#include "guilib/GUIWindowManager.h"

#include "AddonGUIControlSettingsSlider.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnControl_SettingsSlider::Init(::V2::KodiAPI::CB_AddOnLib *callbacks)
{
  callbacks->GUI.Control.SettingsSlider.SetVisible            = CAddOnControl_SettingsSlider::SetVisible;
  callbacks->GUI.Control.SettingsSlider.SetEnabled            = CAddOnControl_SettingsSlider::SetEnabled;

  callbacks->GUI.Control.SettingsSlider.SetText               = CAddOnControl_SettingsSlider::SetText;
  callbacks->GUI.Control.SettingsSlider.Reset                 = CAddOnControl_SettingsSlider::Reset;

  callbacks->GUI.Control.SettingsSlider.SetIntRange           = CAddOnControl_SettingsSlider::SetIntRange;
  callbacks->GUI.Control.SettingsSlider.SetIntValue           = CAddOnControl_SettingsSlider::SetIntValue;
  callbacks->GUI.Control.SettingsSlider.GetIntValue           = CAddOnControl_SettingsSlider::GetIntValue;
  callbacks->GUI.Control.SettingsSlider.SetIntInterval        = CAddOnControl_SettingsSlider::SetIntInterval;

  callbacks->GUI.Control.SettingsSlider.SetPercentage         = CAddOnControl_SettingsSlider::SetPercentage;
  callbacks->GUI.Control.SettingsSlider.GetPercentage         = CAddOnControl_SettingsSlider::GetPercentage;

  callbacks->GUI.Control.SettingsSlider.SetFloatRange         = CAddOnControl_SettingsSlider::SetFloatRange;
  callbacks->GUI.Control.SettingsSlider.SetFloatValue         = CAddOnControl_SettingsSlider::SetFloatValue;
  callbacks->GUI.Control.SettingsSlider.GetFloatValue         = CAddOnControl_SettingsSlider::GetFloatValue;
  callbacks->GUI.Control.SettingsSlider.SetFloatInterval      = CAddOnControl_SettingsSlider::SetFloatInterval;
}

void CAddOnControl_SettingsSlider::SetVisible(void *addonData, GUIHANDLE handle, bool visible)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_SettingsSlider - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISettingsSliderControl*>(handle)->SetVisible(visible);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_SettingsSlider::SetEnabled(void *addonData, GUIHANDLE handle, bool enabled)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_SettingsSlider - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISettingsSliderControl*>(handle)->SetEnabled(enabled);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_SettingsSlider::SetText(void *addonData, GUIHANDLE handle, const char *text)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_SettingsSlider - %s - invalid handler data", __FUNCTION__);

    CGUISettingsSliderControl* pControl = static_cast<CGUISettingsSliderControl*>(handle);

    // create message
    CGUIMessage msg(GUI_MSG_LABEL_SET, pControl->GetParentID(), pControl->GetID());
    msg.SetLabel(text);

    // send message
    g_windowManager.SendThreadMessage(msg, pControl->GetParentID());
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_SettingsSlider::Reset(void *addonData, GUIHANDLE handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_SettingsSlider - %s - invalid handler data", __FUNCTION__);

    CGUISettingsSliderControl* pControl = static_cast<CGUISettingsSliderControl*>(handle);

    CGUIMessage msg(GUI_MSG_LABEL_RESET, pControl->GetParentID(), pControl->GetID());
    g_windowManager.SendThreadMessage(msg, pControl->GetParentID());
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_SettingsSlider::SetIntRange(void *addonData, GUIHANDLE handle, int iStart, int iEnd)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_SettingsSlider - %s - invalid handler data", __FUNCTION__);

    CGUISettingsSliderControl* pControl = static_cast<CGUISettingsSliderControl *>(handle);
    pControl->SetType(SLIDER_CONTROL_TYPE_INT);
    pControl->SetRange(iStart, iEnd);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_SettingsSlider::SetIntValue(void *addonData, GUIHANDLE handle, int iValue)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_SettingsSlider - %s - invalid handler data", __FUNCTION__);

    CGUISettingsSliderControl* pControl = static_cast<CGUISettingsSliderControl *>(handle);
    pControl->SetType(SLIDER_CONTROL_TYPE_INT);
    pControl->SetIntValue(iValue);
  }
  HANDLE_ADDON_EXCEPTION
}

int CAddOnControl_SettingsSlider::GetIntValue(void *addonData, GUIHANDLE handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_SettingsSlider - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISettingsSliderControl*>(handle)->GetIntValue();
  }
  HANDLE_ADDON_EXCEPTION

  return -1;
}

void CAddOnControl_SettingsSlider::SetIntInterval(void *addonData, GUIHANDLE handle, int iInterval)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_SettingsSlider - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISettingsSliderControl*>(handle)->SetIntInterval(iInterval);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_SettingsSlider::SetPercentage(void *addonData, GUIHANDLE handle, float fPercent)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_SettingsSlider - %s - invalid handler data", __FUNCTION__);

    CGUISettingsSliderControl* pControl = static_cast<CGUISettingsSliderControl *>(handle);
    pControl->SetType(SLIDER_CONTROL_TYPE_PERCENTAGE);
    pControl->SetPercentage(fPercent);
  }
  HANDLE_ADDON_EXCEPTION
}

float CAddOnControl_SettingsSlider::GetPercentage(void *addonData, GUIHANDLE handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_SettingsSlider - %s - invalid handler data", __FUNCTION__);

    return static_cast<CGUISettingsSliderControl*>(handle)->GetPercentage();
  }
  HANDLE_ADDON_EXCEPTION

  return 0.0f;
}

void CAddOnControl_SettingsSlider::SetFloatRange(void *addonData, GUIHANDLE handle, float fStart, float fEnd)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_SettingsSlider - %s - invalid handler data", __FUNCTION__);

    CGUISettingsSliderControl* pControl = static_cast<CGUISettingsSliderControl *>(handle);
    pControl->SetType(SLIDER_CONTROL_TYPE_FLOAT);
    pControl->SetFloatRange(fStart, fEnd);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_SettingsSlider::SetFloatValue(void *addonData, GUIHANDLE handle, float fValue)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_SettingsSlider - %s - invalid handler data", __FUNCTION__);

    CGUISettingsSliderControl* pControl = static_cast<CGUISettingsSliderControl *>(handle);
    pControl->SetType(SLIDER_CONTROL_TYPE_FLOAT);
    pControl->SetFloatValue(fValue);
  }
  HANDLE_ADDON_EXCEPTION
}

float CAddOnControl_SettingsSlider::GetFloatValue(void *addonData, GUIHANDLE handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_SettingsSlider - %s - invalid handler data", __FUNCTION__);

    return static_cast<CGUISettingsSliderControl*>(handle)->GetFloatValue();
  }
  HANDLE_ADDON_EXCEPTION

  return 0.0f;
}

void CAddOnControl_SettingsSlider::SetFloatInterval(void *addonData, GUIHANDLE handle, float fInterval)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_SettingsSlider - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISettingsSliderControl*>(handle)->SetFloatInterval(fInterval);
  }
  HANDLE_ADDON_EXCEPTION
}

}; /* extern "C" */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */

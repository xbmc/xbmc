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

#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "guilib/GUISpinControlEx.h"
#include "guilib/GUIWindowManager.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnControl_Spin::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Control.Spin.SetVisible        = CAddOnControl_Spin::SetVisible;
  interfaces->GUI.Control.Spin.SetEnabled        = CAddOnControl_Spin::SetEnabled;

  interfaces->GUI.Control.Spin.SetText           = CAddOnControl_Spin::SetText;
  interfaces->GUI.Control.Spin.Reset             = CAddOnControl_Spin::Reset;
  interfaces->GUI.Control.Spin.SetType           = CAddOnControl_Spin::SetType;

  interfaces->GUI.Control.Spin.AddStringLabel    = CAddOnControl_Spin::AddStringLabel;
  interfaces->GUI.Control.Spin.SetStringValue    = CAddOnControl_Spin::SetStringValue;
  interfaces->GUI.Control.Spin.GetStringValue    = CAddOnControl_Spin::GetStringValue;

  interfaces->GUI.Control.Spin.AddIntLabel       = CAddOnControl_Spin::AddIntLabel;
  interfaces->GUI.Control.Spin.SetIntRange       = CAddOnControl_Spin::SetIntRange;
  interfaces->GUI.Control.Spin.SetIntValue       = CAddOnControl_Spin::SetIntValue;
  interfaces->GUI.Control.Spin.GetIntValue       = CAddOnControl_Spin::GetIntValue;

  interfaces->GUI.Control.Spin.SetFloatRange     = CAddOnControl_Spin::SetFloatRange;
  interfaces->GUI.Control.Spin.SetFloatValue     = CAddOnControl_Spin::SetFloatValue;
  interfaces->GUI.Control.Spin.GetFloatValue     = CAddOnControl_Spin::GetFloatValue;
  interfaces->GUI.Control.Spin.SetFloatInterval  = CAddOnControl_Spin::SetFloatInterval;
}

void CAddOnControl_Spin::SetVisible(void *addonData, void* handle, bool visible)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->SetVisible(visible);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Spin::SetEnabled(void *addonData, void* handle, bool enabled)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->SetEnabled(enabled);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Spin::SetText(void *addonData, void* handle, const char *text)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    CGUISpinControlEx* pControl = static_cast<CGUISpinControlEx*>(handle);

    // create message
    CGUIMessage msg(GUI_MSG_LABEL_SET, pControl->GetParentID(), pControl->GetID());
    msg.SetLabel(text);

    // send message
    g_windowManager.SendThreadMessage(msg, pControl->GetParentID());
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Spin::Reset(void *addonData, void* handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    CGUISpinControlEx* pControl = static_cast<CGUISpinControlEx*>(handle);

    CGUIMessage msg(GUI_MSG_LABEL_RESET, pControl->GetParentID(), pControl->GetID());
    g_windowManager.SendThreadMessage(msg, pControl->GetParentID());
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Spin::SetType(void *addonData, void* handle, int type)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->SetType(type);
  }
  HANDLE_ADDON_EXCEPTION
}


void CAddOnControl_Spin::AddStringLabel(void *addonData, void* handle, const char* label, const char* value)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->AddLabel(std::string(label), std::string(value));
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Spin::SetStringValue(void *addonData, void* handle, const char* value)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->SetStringValue(std::string(value));
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Spin::GetStringValue(void *addonData, void* handle, char &value, unsigned int &maxStringSize)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    std::string text = static_cast<CGUISpinControlEx*>(handle)->GetStringValue();
    strncpy(&value, text.c_str(), maxStringSize);
    maxStringSize = text.length();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Spin::AddIntLabel(void *addonData, void* handle, const char* label, int value)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->AddLabel(std::string(label), value);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Spin::SetIntRange(void *addonData, void* handle, int start, int end)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->SetRange(start, end);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Spin::SetIntValue(void *addonData, void* handle, int value)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->SetValue(value);
  }
  HANDLE_ADDON_EXCEPTION
}

int CAddOnControl_Spin::GetIntValue(void *addonData, void* handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    return static_cast<CGUISpinControlEx*>(handle)->GetValue();
  }
  HANDLE_ADDON_EXCEPTION

  return 0;
}

void CAddOnControl_Spin::SetFloatRange(void *addonData, void* handle, float start, float end)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->SetFloatRange(start, end);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Spin::SetFloatValue(void *addonData, void* handle, float value)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->SetFloatValue(value);
  }
  HANDLE_ADDON_EXCEPTION
}

float CAddOnControl_Spin::GetFloatValue(void *addonData, void* handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    return static_cast<CGUISpinControlEx*>(handle)->GetFloatValue();
  }
  HANDLE_ADDON_EXCEPTION

  return 0.0f;
}

void CAddOnControl_Spin::SetFloatInterval(void *addonData, void* handle, float interval)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->SetFloatInterval(interval);
  }
  HANDLE_ADDON_EXCEPTION
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */

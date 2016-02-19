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
#include "guilib/GUISpinControlEx.h"
#include "guilib/GUIWindowManager.h"

#include "AddonGUIControlSpin.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnControl_Spin::Init(::V2::KodiAPI::CB_AddOnLib *callbacks)
{
  callbacks->GUI.Control.Spin.SetVisible        = CAddOnControl_Spin::SetVisible;
  callbacks->GUI.Control.Spin.SetEnabled        = CAddOnControl_Spin::SetEnabled;

  callbacks->GUI.Control.Spin.SetText           = CAddOnControl_Spin::SetText;
  callbacks->GUI.Control.Spin.Reset             = CAddOnControl_Spin::Reset;
  callbacks->GUI.Control.Spin.SetType           = CAddOnControl_Spin::SetType;

  callbacks->GUI.Control.Spin.AddStringLabel    = CAddOnControl_Spin::AddStringLabel;
  callbacks->GUI.Control.Spin.SetStringValue    = CAddOnControl_Spin::SetStringValue;
  callbacks->GUI.Control.Spin.GetStringValue    = CAddOnControl_Spin::GetStringValue;

  callbacks->GUI.Control.Spin.AddIntLabel       = CAddOnControl_Spin::AddIntLabel;
  callbacks->GUI.Control.Spin.SetIntRange       = CAddOnControl_Spin::SetIntRange;
  callbacks->GUI.Control.Spin.SetIntValue       = CAddOnControl_Spin::SetIntValue;
  callbacks->GUI.Control.Spin.GetIntValue       = CAddOnControl_Spin::GetIntValue;

  callbacks->GUI.Control.Spin.SetFloatRange     = CAddOnControl_Spin::SetFloatRange;
  callbacks->GUI.Control.Spin.SetFloatValue     = CAddOnControl_Spin::SetFloatValue;
  callbacks->GUI.Control.Spin.GetFloatValue     = CAddOnControl_Spin::GetFloatValue;
  callbacks->GUI.Control.Spin.SetFloatInterval  = CAddOnControl_Spin::SetFloatInterval;
}

void CAddOnControl_Spin::SetVisible(void *addonData, GUIHANDLE handle, bool visible)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->SetVisible(visible);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Spin::SetEnabled(void *addonData, GUIHANDLE handle, bool enabled)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->SetEnabled(enabled);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Spin::SetText(void *addonData, GUIHANDLE handle, const char *text)
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

void CAddOnControl_Spin::Reset(void *addonData, GUIHANDLE handle)
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

void CAddOnControl_Spin::SetType(void *addonData, GUIHANDLE handle, int type)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->SetType(type);
  }
  HANDLE_ADDON_EXCEPTION
}


void CAddOnControl_Spin::AddStringLabel(void *addonData, GUIHANDLE handle, const char* label, const char* value)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->AddLabel(std::string(label), std::string(value));
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Spin::SetStringValue(void *addonData, GUIHANDLE handle, const char* value)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->SetStringValue(std::string(value));
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Spin::GetStringValue(void *addonData, GUIHANDLE handle, char &value, unsigned int &maxStringSize)
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

void CAddOnControl_Spin::AddIntLabel(void *addonData, GUIHANDLE handle, const char* label, int value)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->AddLabel(std::string(label), value);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Spin::SetIntRange(void *addonData, GUIHANDLE handle, int start, int end)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->SetRange(start, end);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Spin::SetIntValue(void *addonData, GUIHANDLE handle, int value)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->SetValue(value);
  }
  HANDLE_ADDON_EXCEPTION
}

int CAddOnControl_Spin::GetIntValue(void *addonData, GUIHANDLE handle)
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

void CAddOnControl_Spin::SetFloatRange(void *addonData, GUIHANDLE handle, float start, float end)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->SetFloatRange(start, end);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Spin::SetFloatValue(void *addonData, GUIHANDLE handle, float value)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->SetFloatValue(value);
  }
  HANDLE_ADDON_EXCEPTION
}

float CAddOnControl_Spin::GetFloatValue(void *addonData, GUIHANDLE handle)
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

void CAddOnControl_Spin::SetFloatInterval(void *addonData, GUIHANDLE handle, float interval)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Spin - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUISpinControlEx*>(handle)->SetFloatInterval(interval);
  }
  HANDLE_ADDON_EXCEPTION
}

}; /* extern "C" */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */

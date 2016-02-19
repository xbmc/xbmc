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
#include "guilib/GUIEditControl.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

#include "AddonGUIControlEdit.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnControl_Edit::Init(::V2::KodiAPI::CB_AddOnLib *callbacks)
{
  callbacks->GUI.Control.Edit.SetVisible              = CAddOnControl_Edit::SetVisible;
  callbacks->GUI.Control.Edit.SetEnabled              = CAddOnControl_Edit::SetEnabled;
  callbacks->GUI.Control.Edit.SetLabel                = CAddOnControl_Edit::SetLabel;
  callbacks->GUI.Control.Edit.GetLabel                = CAddOnControl_Edit::GetLabel;
  callbacks->GUI.Control.Edit.SetText                 = CAddOnControl_Edit::SetText;
  callbacks->GUI.Control.Edit.GetText                 = CAddOnControl_Edit::GetText;
  callbacks->GUI.Control.Edit.SetCursorPosition       = CAddOnControl_Edit::SetCursorPosition;
  callbacks->GUI.Control.Edit.GetCursorPosition       = CAddOnControl_Edit::GetCursorPosition;
  callbacks->GUI.Control.Edit.SetInputType            = CAddOnControl_Edit::SetInputType;
}

void CAddOnControl_Edit::SetVisible(void *addonData, GUIHANDLE handle, bool yesNo)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Edit - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIEditControl*>(handle)->SetVisible(yesNo);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Edit::SetEnabled(void *addonData, GUIHANDLE handle, bool yesNo)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Edit - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIEditControl*>(handle)->SetEnabled(yesNo);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Edit::SetLabel(void *addonData, GUIHANDLE handle, const char *label)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Edit - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIEditControl*>(handle)->SetLabel(label);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Edit::GetLabel(void *addonData, GUIHANDLE handle, char &label, unsigned int &iMaxStringSize)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Edit - %s - invalid handler data", __FUNCTION__);

    std::string text = static_cast<CGUIEditControl*>(handle)->GetLabel();
    strncpy(&label, text.c_str(), iMaxStringSize);
    iMaxStringSize = text.length();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Edit::SetText(void *addonData, GUIHANDLE handle, const char* text)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Edit - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIEditControl*>(handle)->SetLabel2(text);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Edit::GetText(void *addonData, GUIHANDLE handle, char& text, unsigned int &iMaxStringSize)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Edit - %s - invalid handler data", __FUNCTION__);

    std::string textTmp = static_cast<CGUIEditControl*>(handle)->GetLabel2();
    strncpy(&text, textTmp.c_str(), iMaxStringSize);
    iMaxStringSize = textTmp.length();
  }
  HANDLE_ADDON_EXCEPTION
}

unsigned int CAddOnControl_Edit::GetCursorPosition(void *addonData, GUIHANDLE handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Edit - %s - invalid handler data", __FUNCTION__);

    return static_cast<CGUIEditControl*>(handle)->GetCursorPosition();
  }
  HANDLE_ADDON_EXCEPTION

  return 0;
}

void CAddOnControl_Edit::SetCursorPosition(void *addonData, GUIHANDLE handle, unsigned int iPosition)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Edit - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIEditControl*>(handle)->SetCursorPosition(iPosition);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Edit::SetInputType(void *addonData, GUIHANDLE handle, AddonGUIInputType type, const char *heading)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Edit - %s - invalid handler data", __FUNCTION__);

    CGUIEditControl::INPUT_TYPE kodiType;
    switch (type)
    {
      case ADDON_INPUT_TYPE_TEXT:
        kodiType = CGUIEditControl::INPUT_TYPE_TEXT;
        break;
      case ADDON_INPUT_TYPE_NUMBER:
        kodiType = CGUIEditControl::INPUT_TYPE_NUMBER;
        break;
      case ADDON_INPUT_TYPE_SECONDS:
        kodiType = CGUIEditControl::INPUT_TYPE_SECONDS;
        break;
      case ADDON_INPUT_TYPE_TIME:
        kodiType = CGUIEditControl::INPUT_TYPE_TIME;
        break;
      case ADDON_INPUT_TYPE_DATE:
        kodiType = CGUIEditControl::INPUT_TYPE_DATE;
        break;
      case ADDON_INPUT_TYPE_IPADDRESS:
        kodiType = CGUIEditControl::INPUT_TYPE_IPADDRESS;
        break;
      case ADDON_INPUT_TYPE_PASSWORD:
        kodiType = CGUIEditControl::INPUT_TYPE_PASSWORD;
        break;
      case ADDON_INPUT_TYPE_PASSWORD_MD5:
        kodiType = CGUIEditControl::INPUT_TYPE_PASSWORD_MD5;
        break;
      case ADDON_INPUT_TYPE_SEARCH:
        kodiType = CGUIEditControl::INPUT_TYPE_SEARCH;
        break;
      case ADDON_INPUT_TYPE_FILTER:
        kodiType = CGUIEditControl::INPUT_TYPE_FILTER;
        break;
      case ADDON_INPUT_TYPE_READONLY:
      default:
        kodiType = CGUIEditControl::INPUT_TYPE_PASSWORD_NUMBER_VERIFY_NEW;
    }

    static_cast<CGUIEditControl*>(handle)->SetInputType(kodiType, heading);
  }
  HANDLE_ADDON_EXCEPTION
}

}; /* extern "C" */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */

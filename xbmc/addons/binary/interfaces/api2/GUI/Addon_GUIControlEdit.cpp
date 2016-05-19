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

#include "Addon_GUIControlEdit.h"

#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnControl_Edit::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Control.Edit.SetVisible = V2::KodiAPI::GUI::CAddOnControl_Edit::SetVisible;
  interfaces->GUI.Control.Edit.SetEnabled = V2::KodiAPI::GUI::CAddOnControl_Edit::SetEnabled;
  interfaces->GUI.Control.Edit.SetLabel = V2::KodiAPI::GUI::CAddOnControl_Edit::SetLabel;
  interfaces->GUI.Control.Edit.GetLabel = V2::KodiAPI::GUI::CAddOnControl_Edit::GetLabel;
  interfaces->GUI.Control.Edit.SetText = V2::KodiAPI::GUI::CAddOnControl_Edit::SetText;
  interfaces->GUI.Control.Edit.GetText = V2::KodiAPI::GUI::CAddOnControl_Edit::GetText;
  interfaces->GUI.Control.Edit.SetCursorPosition = V2::KodiAPI::GUI::CAddOnControl_Edit::SetCursorPosition;
  interfaces->GUI.Control.Edit.GetCursorPosition = V2::KodiAPI::GUI::CAddOnControl_Edit::GetCursorPosition;
  interfaces->GUI.Control.Edit.SetInputType = V2::KodiAPI::GUI::CAddOnControl_Edit::SetInputType;
}

void CAddOnControl_Edit::SetVisible(void *addonData, void* handle, bool yesNo)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Edit - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIEditControl*>(handle)->SetVisible(yesNo);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Edit::SetEnabled(void *addonData, void* handle, bool yesNo)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Edit - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIEditControl*>(handle)->SetEnabled(yesNo);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Edit::SetLabel(void *addonData, void* handle, const char *label)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Edit - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIEditControl*>(handle)->SetLabel(label);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Edit::GetLabel(void *addonData, void* handle, char &label, unsigned int &iMaxStringSize)
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

void CAddOnControl_Edit::SetText(void *addonData, void* handle, const char* text)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Edit - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIEditControl*>(handle)->SetLabel2(text);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Edit::GetText(void *addonData, void* handle, char& text, unsigned int &iMaxStringSize)
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

unsigned int CAddOnControl_Edit::GetCursorPosition(void *addonData, void* handle)
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

void CAddOnControl_Edit::SetCursorPosition(void *addonData, void* handle, unsigned int iPosition)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_Edit - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIEditControl*>(handle)->SetCursorPosition(iPosition);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_Edit::SetInputType(void *addonData, void* handle, int type, const char *heading)
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

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */

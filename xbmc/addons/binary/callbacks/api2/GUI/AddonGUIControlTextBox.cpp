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
#include "guilib/GUITextBox.h"
#include "guilib/GUIWindowManager.h"

#include "AddonGUIControlTextBox.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnControl_TextBox::Init(::V2::KodiAPI::CB_AddOnLib *callbacks)
{
  callbacks->GUI.Control.TextBox.SetVisible       = CAddOnControl_TextBox::SetVisible;
  callbacks->GUI.Control.TextBox.Reset            = CAddOnControl_TextBox::Reset;
  callbacks->GUI.Control.TextBox.SetText          = CAddOnControl_TextBox::SetText;
  callbacks->GUI.Control.TextBox.GetText          = CAddOnControl_TextBox::GetText;
  callbacks->GUI.Control.TextBox.Scroll           = CAddOnControl_TextBox::Scroll;
  callbacks->GUI.Control.TextBox.SetAutoScrolling = CAddOnControl_TextBox::SetAutoScrolling;
}

void CAddOnControl_TextBox::SetVisible(void *addonData, GUIHANDLE handle, bool visible)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_TextBox - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUITextBox*>(handle)->SetVisible(visible);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_TextBox::Reset(void *addonData, GUIHANDLE handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_TextBox - %s - invalid handler data", __FUNCTION__);

    CGUITextBox* textBox = static_cast<CGUITextBox*>(handle);

    CGUIMessage msg(GUI_MSG_LABEL_RESET, textBox->GetParentID(), textBox->GetID());
    g_windowManager.SendThreadMessage(msg, textBox->GetParentID());
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_TextBox::SetText(void *addonData, GUIHANDLE handle, const char *text)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_TextBox - %s - invalid handler data", __FUNCTION__);

    CGUITextBox* textBox = static_cast<CGUITextBox*>(handle);

    // create message
    CGUIMessage msg(GUI_MSG_LABEL_SET, textBox->GetParentID(), textBox->GetID());
    msg.SetLabel(text);

    // send message
    g_windowManager.SendThreadMessage(msg, textBox->GetParentID());
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_TextBox::GetText(void *addonData, GUIHANDLE handle, char &text, unsigned int &iMaxStringSize)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_TextBox - %s - invalid handler data", __FUNCTION__);

    std::string tmpText = static_cast<CGUITextBox*>(handle)->GetDescription();
    strncpy(&text, tmpText.c_str(), iMaxStringSize);
    iMaxStringSize = tmpText.size();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_TextBox::Scroll(void *addonData, GUIHANDLE handle, unsigned int position)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_TextBox - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUITextBox*>(handle)->Scroll(position);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_TextBox::SetAutoScrolling(void *addonData, GUIHANDLE handle, int delay, int time, int repeat)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_TextBox - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUITextBox*>(handle)->SetAutoScrolling(delay, time, repeat);
  }
  HANDLE_ADDON_EXCEPTION
}

}; /* extern "C" */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */

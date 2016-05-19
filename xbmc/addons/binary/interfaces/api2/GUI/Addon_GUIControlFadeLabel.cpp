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

#include "Addon_GUIControlFadeLabel.h"

#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "guilib/GUIFadeLabelControl.h"
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

void CAddOnControl_FadeLabel::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Control.FadeLabel.SetVisible = CAddOnControl_FadeLabel::SetVisible;
  interfaces->GUI.Control.FadeLabel.AddLabel = CAddOnControl_FadeLabel::AddLabel;
  interfaces->GUI.Control.FadeLabel.GetLabel = CAddOnControl_FadeLabel::GetLabel;
  interfaces->GUI.Control.FadeLabel.SetScrolling = CAddOnControl_FadeLabel::SetScrolling;
  interfaces->GUI.Control.FadeLabel.Reset = CAddOnControl_FadeLabel::Reset;
}

void CAddOnControl_FadeLabel::SetVisible(void *addonData, void* handle, bool visible)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_FadeLabel - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIFadeLabelControl*>(handle)->SetVisible(visible);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_FadeLabel::AddLabel(void *addonData, void* handle, const char *label)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_FadeLabel - %s - invalid handler data", __FUNCTION__);

    CGUIMessage msg(GUI_MSG_LABEL_ADD, static_cast<CGUIFadeLabelControl*>(handle)->GetParentID(), static_cast<CGUIFadeLabelControl*>(handle)->GetID());
    msg.SetLabel(label);
    static_cast<CGUIFadeLabelControl*>(handle)->OnMessage(msg);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_FadeLabel::GetLabel(void *addonData, void* handle, char &label, unsigned int &iMaxStringSize)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_FadeLabel - %s - invalid handler data", __FUNCTION__);

    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, static_cast<CGUIFadeLabelControl*>(handle)->GetParentID(), static_cast<CGUIFadeLabelControl*>(handle)->GetID());
    static_cast<CGUIFadeLabelControl*>(handle)->OnMessage(msg);
    std::string text = msg.GetLabel();
    strncpy(&label, text.c_str(), iMaxStringSize);
    iMaxStringSize = text.length();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_FadeLabel::SetScrolling(void *addonData, void* handle, bool scroll)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_FadeLabel - %s - invalid handler data", __FUNCTION__);

    static_cast<CGUIFadeLabelControl*>(handle)->SetScrolling(scroll);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnControl_FadeLabel::Reset(void *addonData, void* handle)
{
  try
  {
    if (!handle)
      throw ADDON::WrongValueException("CAddOnControl_FadeLabel - %s - invalid handler data", __FUNCTION__);

    CGUIMessage msg(GUI_MSG_LABEL_RESET, static_cast<CGUIFadeLabelControl*>(handle)->GetParentID(), static_cast<CGUIFadeLabelControl*>(handle)->GetID());
    static_cast<CGUIFadeLabelControl*>(handle)->OnMessage(msg);
  }
  HANDLE_ADDON_EXCEPTION
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */

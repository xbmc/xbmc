/*
 *      Copyright (C) 2005-2017 Team Kodi
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

#include "TextBox.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/controls/TextBox.h"

#include "addons/AddonDll.h"
#include "guilib/GUITextBox.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

extern "C"
{
namespace ADDON
{

void Interface_GUIControlTextBox::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->control_text_box = static_cast<AddonToKodiFuncTable_kodi_gui_control_text_box*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_control_text_box)));

  addonInterface->toKodi->kodi_gui->control_text_box->set_visible = set_visible;
  addonInterface->toKodi->kodi_gui->control_text_box->reset = reset;
  addonInterface->toKodi->kodi_gui->control_text_box->set_text = set_text;
  addonInterface->toKodi->kodi_gui->control_text_box->get_text = get_text;
  addonInterface->toKodi->kodi_gui->control_text_box->scroll = scroll;
  addonInterface->toKodi->kodi_gui->control_text_box->set_auto_scrolling = set_auto_scrolling;
}

void Interface_GUIControlTextBox::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->control_text_box);
}

void Interface_GUIControlTextBox::set_visible(void* kodiBase, void* handle, bool visible)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUITextBox* control = static_cast<CGUITextBox*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlTextBox::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetVisible(visible);
}

void Interface_GUIControlTextBox::reset(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUITextBox* control = static_cast<CGUITextBox*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlTextBox::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  CGUIMessage msg(GUI_MSG_LABEL_RESET, control->GetParentID(), control->GetID());
  g_windowManager.SendThreadMessage(msg, control->GetParentID());
}

void Interface_GUIControlTextBox::set_text(void* kodiBase, void* handle, const char* text)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUITextBox* control = static_cast<CGUITextBox*>(handle);
  if (!addon || !control || !text)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlTextBox::%s - invalid handler data (kodiBase='%p', handle='%p', text='%p') on addon '%s'",
                          __FUNCTION__, addon, control, text, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  CGUIMessage msg(GUI_MSG_LABEL_SET, control->GetParentID(), control->GetID());
  msg.SetLabel(text);
  g_windowManager.SendThreadMessage(msg, control->GetParentID());
}

char* Interface_GUIControlTextBox::get_text(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUITextBox* control = static_cast<CGUITextBox*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlTextBox::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return nullptr;
  }

  return strdup(control->GetDescription().c_str());
}

void Interface_GUIControlTextBox::scroll(void* kodiBase, void* handle, unsigned int position)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUITextBox* control = static_cast<CGUITextBox*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlTextBox::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->Scroll(position);
}

void Interface_GUIControlTextBox::set_auto_scrolling(void* kodiBase, void* handle, int delay, int time, int repeat)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUITextBox* control = static_cast<CGUITextBox*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlTextBox::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetAutoScrolling(delay, time, repeat);
}

} /* namespace ADDON */
} /* extern "C" */

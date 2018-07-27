/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextBox.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/controls/TextBox.h"

#include "addons/binary-addons/AddonDll.h"
#include "guilib/GUITextBox.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"
#include "ServiceBroker.h"

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
                          __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
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
                          __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  CGUIMessage msg(GUI_MSG_LABEL_RESET, control->GetParentID(), control->GetID());
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, control->GetParentID());
}

void Interface_GUIControlTextBox::set_text(void* kodiBase, void* handle, const char* text)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUITextBox* control = static_cast<CGUITextBox*>(handle);
  if (!addon || !control || !text)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlTextBox::%s - invalid handler data (kodiBase='%p', handle='%p', text='%p') on addon '%s'",
                          __FUNCTION__, kodiBase, handle, text, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  CGUIMessage msg(GUI_MSG_LABEL_SET, control->GetParentID(), control->GetID());
  msg.SetLabel(text);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, control->GetParentID());
}

char* Interface_GUIControlTextBox::get_text(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUITextBox* control = static_cast<CGUITextBox*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlTextBox::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
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
                          __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
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
                          __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetAutoScrolling(delay, time, repeat);
}

} /* namespace ADDON */
} /* extern "C" */

/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextBox.h"

#include "ServiceBroker.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/gui/controls/TextBox.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUITextBox.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

namespace ADDON
{

void Interface_GUIControlTextBox::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->control_text_box =
      new AddonToKodiFuncTable_kodi_gui_control_text_box();

  addonInterface->toKodi->kodi_gui->control_text_box->set_visible = set_visible;
  addonInterface->toKodi->kodi_gui->control_text_box->reset = reset;
  addonInterface->toKodi->kodi_gui->control_text_box->set_text = set_text;
  addonInterface->toKodi->kodi_gui->control_text_box->get_text = get_text;
  addonInterface->toKodi->kodi_gui->control_text_box->scroll = scroll;
  addonInterface->toKodi->kodi_gui->control_text_box->set_auto_scrolling = set_auto_scrolling;
}

void Interface_GUIControlTextBox::DeInit(AddonGlobalInterface* addonInterface)
{
  delete addonInterface->toKodi->kodi_gui->control_text_box;
}

void Interface_GUIControlTextBox::set_visible(KODI_HANDLE kodiBase,
                                              KODI_GUI_CONTROL_HANDLE handle,
                                              bool visible)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUITextBox* control = static_cast<CGUITextBox*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlTextBox::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetVisible(visible);
}

void Interface_GUIControlTextBox::reset(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUITextBox* control = static_cast<CGUITextBox*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlTextBox::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  CGUIMessage msg(GUI_MSG_LABEL_RESET, control->GetParentID(), control->GetID());
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, control->GetParentID());
}

void Interface_GUIControlTextBox::set_text(KODI_HANDLE kodiBase,
                                           KODI_GUI_CONTROL_HANDLE handle,
                                           const char* text)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUITextBox* control = static_cast<CGUITextBox*>(handle);
  if (!addon || !control || !text)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlTextBox::{} - invalid handler data (kodiBase='{}', "
              "handle='{}', text='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(text),
              addon ? addon->ID() : "unknown");
    return;
  }

  CGUIMessage msg(GUI_MSG_LABEL_SET, control->GetParentID(), control->GetID());
  msg.SetLabel(text);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, control->GetParentID());
}

char* Interface_GUIControlTextBox::get_text(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUITextBox* control = static_cast<CGUITextBox*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlTextBox::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return nullptr;
  }

  return strdup(control->GetDescription().c_str());
}

void Interface_GUIControlTextBox::scroll(KODI_HANDLE kodiBase,
                                         KODI_GUI_CONTROL_HANDLE handle,
                                         unsigned int position)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUITextBox* control = static_cast<CGUITextBox*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlTextBox::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->Scroll(position);
}

void Interface_GUIControlTextBox::set_auto_scrolling(
    KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, int delay, int time, int repeat)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUITextBox* control = static_cast<CGUITextBox*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlTextBox::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetAutoScrolling(delay, time, repeat);
}

} /* namespace ADDON */

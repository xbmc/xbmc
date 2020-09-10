/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Spin.h"

#include "ServiceBroker.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/gui/controls/Spin.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUISpinControlEx.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

namespace ADDON
{

void Interface_GUIControlSpin::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->control_spin = new AddonToKodiFuncTable_kodi_gui_control_spin();

  addonInterface->toKodi->kodi_gui->control_spin->set_visible = set_visible;
  addonInterface->toKodi->kodi_gui->control_spin->set_enabled = set_enabled;

  addonInterface->toKodi->kodi_gui->control_spin->set_text = set_text;
  addonInterface->toKodi->kodi_gui->control_spin->reset = reset;
  addonInterface->toKodi->kodi_gui->control_spin->set_type = set_type;

  addonInterface->toKodi->kodi_gui->control_spin->add_string_label = add_string_label;
  addonInterface->toKodi->kodi_gui->control_spin->set_string_value = set_string_value;
  addonInterface->toKodi->kodi_gui->control_spin->get_string_value = get_string_value;

  addonInterface->toKodi->kodi_gui->control_spin->add_int_label = add_int_label;
  addonInterface->toKodi->kodi_gui->control_spin->set_int_range = set_int_range;
  addonInterface->toKodi->kodi_gui->control_spin->set_int_value = set_int_value;
  addonInterface->toKodi->kodi_gui->control_spin->get_int_value = get_int_value;

  addonInterface->toKodi->kodi_gui->control_spin->set_float_range = set_float_range;
  addonInterface->toKodi->kodi_gui->control_spin->set_float_value = set_float_value;
  addonInterface->toKodi->kodi_gui->control_spin->get_float_value = get_float_value;
  addonInterface->toKodi->kodi_gui->control_spin->set_float_interval = set_float_interval;
}

void Interface_GUIControlSpin::DeInit(AddonGlobalInterface* addonInterface)
{
  delete addonInterface->toKodi->kodi_gui->control_spin;
}

void Interface_GUIControlSpin::set_visible(KODI_HANDLE kodiBase,
                                           KODI_GUI_CONTROL_HANDLE handle,
                                           bool visible)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSpin::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetVisible(visible);
}

void Interface_GUIControlSpin::set_enabled(KODI_HANDLE kodiBase,
                                           KODI_GUI_CONTROL_HANDLE handle,
                                           bool enabled)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSpin::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetEnabled(enabled);
}

void Interface_GUIControlSpin::set_text(KODI_HANDLE kodiBase,
                                        KODI_GUI_CONTROL_HANDLE handle,
                                        const char* text)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control || !text)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSpin::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "text='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(text),
              addon ? addon->ID() : "unknown");
    return;
  }

  CGUIMessage msg(GUI_MSG_LABEL_SET, control->GetParentID(), control->GetID());
  msg.SetLabel(text);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, control->GetParentID());
}

void Interface_GUIControlSpin::reset(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSpin::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  CGUIMessage msg(GUI_MSG_LABEL_RESET, control->GetParentID(), control->GetID());
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, control->GetParentID());
}

void Interface_GUIControlSpin::set_type(KODI_HANDLE kodiBase,
                                        KODI_GUI_CONTROL_HANDLE handle,
                                        int type)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSpin::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetType(type);
}

void Interface_GUIControlSpin::add_string_label(KODI_HANDLE kodiBase,
                                                KODI_GUI_CONTROL_HANDLE handle,
                                                const char* label,
                                                const char* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control || !label || !value)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSpin::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "label='{}', value='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(label),
              static_cast<const void*>(value), addon ? addon->ID() : "unknown");
    return;
  }

  control->AddLabel(std::string(label), std::string(value));
}

void Interface_GUIControlSpin::set_string_value(KODI_HANDLE kodiBase,
                                                KODI_GUI_CONTROL_HANDLE handle,
                                                const char* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control || !value)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSpin::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "value='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(value),
              addon ? addon->ID() : "unknown");
    return;
  }

  control->SetStringValue(std::string(value));
}

char* Interface_GUIControlSpin::get_string_value(KODI_HANDLE kodiBase,
                                                 KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSpin::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return nullptr;
  }

  return strdup(control->GetStringValue().c_str());
}

void Interface_GUIControlSpin::add_int_label(KODI_HANDLE kodiBase,
                                             KODI_GUI_CONTROL_HANDLE handle,
                                             const char* label,
                                             int value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control || !label)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSpin::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "label='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(label),
              addon ? addon->ID() : "unknown");
    return;
  }

  control->AddLabel(std::string(label), value);
}

void Interface_GUIControlSpin::set_int_range(KODI_HANDLE kodiBase,
                                             KODI_GUI_CONTROL_HANDLE handle,
                                             int start,
                                             int end)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSpin::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetRange(start, end);
}

void Interface_GUIControlSpin::set_int_value(KODI_HANDLE kodiBase,
                                             KODI_GUI_CONTROL_HANDLE handle,
                                             int value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSpin::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetValue(value);
}

int Interface_GUIControlSpin::get_int_value(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSpin::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return -1;
  }

  return control->GetValue();
}

void Interface_GUIControlSpin::set_float_range(KODI_HANDLE kodiBase,
                                               KODI_GUI_CONTROL_HANDLE handle,
                                               float start,
                                               float end)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSpin::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetFloatRange(start, end);
}

void Interface_GUIControlSpin::set_float_value(KODI_HANDLE kodiBase,
                                               KODI_GUI_CONTROL_HANDLE handle,
                                               float value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSpin::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetFloatValue(value);
}

float Interface_GUIControlSpin::get_float_value(KODI_HANDLE kodiBase,
                                                KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSpin::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return 0.0f;
  }

  return control->GetFloatValue();
}

void Interface_GUIControlSpin::set_float_interval(KODI_HANDLE kodiBase,
                                                  KODI_GUI_CONTROL_HANDLE handle,
                                                  float interval)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSpin::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetFloatInterval(interval);
}

} /* namespace ADDON */

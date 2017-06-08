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

#include "Spin.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/controls/Spin.h"

#include "addons/AddonDll.h"
#include "guilib/GUISpinControlEx.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

extern "C"
{
namespace ADDON
{

void Interface_GUIControlSpin::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->control_spin = static_cast<AddonToKodiFuncTable_kodi_gui_control_spin*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_control_spin)));

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
  free(addonInterface->toKodi->kodi_gui->control_spin);
}

void Interface_GUIControlSpin::set_visible(void* kodiBase, void* handle, bool visible)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSpin::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetVisible(visible);
}

void Interface_GUIControlSpin::set_enabled(void* kodiBase, void* handle, bool enabled)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSpin::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetEnabled(enabled);
}

void Interface_GUIControlSpin::set_text(void* kodiBase, void* handle, const char *text)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control || !text)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSpin::%s - invalid handler data (kodiBase='%p', handle='%p', text='%p') on addon '%s'",
                          __FUNCTION__, addon, control, text, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  CGUIMessage msg(GUI_MSG_LABEL_SET, control->GetParentID(), control->GetID());
  msg.SetLabel(text);
  g_windowManager.SendThreadMessage(msg, control->GetParentID());
}

void Interface_GUIControlSpin::reset(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSpin::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  CGUIMessage msg(GUI_MSG_LABEL_RESET, control->GetParentID(), control->GetID());
  g_windowManager.SendThreadMessage(msg, control->GetParentID());
}

void Interface_GUIControlSpin::set_type(void* kodiBase, void* handle, int type)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSpin::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetType(type);
}

void Interface_GUIControlSpin::add_string_label(void* kodiBase, void* handle, const char* label, const char* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control || !label || !value)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSpin::%s - invalid handler data (kodiBase='%p', handle='%p', label='%p', value='%p') on addon '%s'",
                          __FUNCTION__, addon, control, label, value, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->AddLabel(std::string(label), std::string(value));
}

void Interface_GUIControlSpin::set_string_value(void* kodiBase, void* handle, const char* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control || !value)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSpin::%s - invalid handler data (kodiBase='%p', handle='%p', value='%p') on addon '%s'",
                          __FUNCTION__, addon, control, value, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetStringValue(std::string(value));
}

char* Interface_GUIControlSpin::get_string_value(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSpin::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return nullptr;
  }

  return strdup(control->GetStringValue().c_str());
}

void Interface_GUIControlSpin::add_int_label(void* kodiBase, void* handle, const char* label, int value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control || !label)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSpin::%s - invalid handler data (kodiBase='%p', handle='%p', label='%p') on addon '%s'",
                          __FUNCTION__, addon, control, label, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->AddLabel(std::string(label), value);
}

void Interface_GUIControlSpin::set_int_range(void* kodiBase, void* handle, int start, int end)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSpin::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetRange(start, end);
}

void Interface_GUIControlSpin::set_int_value(void* kodiBase, void* handle, int value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSpin::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetValue(value);
}

int Interface_GUIControlSpin::get_int_value(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSpin::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return -1;
  }

  return control->GetValue();
}

void Interface_GUIControlSpin::set_float_range(void* kodiBase, void* handle, float start, float end)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSpin::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetFloatRange(start, end);
}

void Interface_GUIControlSpin::set_float_value(void* kodiBase, void* handle, float value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSpin::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetFloatValue(value);
}

float Interface_GUIControlSpin::get_float_value(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSpin::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return 0.0f;
  }

  return control->GetFloatValue();
}

void Interface_GUIControlSpin::set_float_interval(void* kodiBase, void* handle, float interval)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISpinControlEx* control = static_cast<CGUISpinControlEx*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSpin::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetFloatInterval(interval);
}

} /* namespace ADDON */
} /* extern "C" */

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

#include "SettingsSlider.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/controls/SettingsSlider.h"

#include "addons/AddonDll.h"
#include "guilib/GUISettingsSliderControl.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

extern "C"
{
namespace ADDON
{

void Interface_GUIControlSettingsSlider::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->control_settings_slider = static_cast<AddonToKodiFuncTable_kodi_gui_control_settings_slider*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_control_settings_slider)));

  addonInterface->toKodi->kodi_gui->control_settings_slider->set_visible = set_visible;
  addonInterface->toKodi->kodi_gui->control_settings_slider->set_enabled = set_enabled;

  addonInterface->toKodi->kodi_gui->control_settings_slider->set_text = set_text;
  addonInterface->toKodi->kodi_gui->control_settings_slider->reset = reset;

  addonInterface->toKodi->kodi_gui->control_settings_slider->set_int_range = set_int_range;
  addonInterface->toKodi->kodi_gui->control_settings_slider->set_int_value = set_int_value;
  addonInterface->toKodi->kodi_gui->control_settings_slider->get_int_value = get_int_value;
  addonInterface->toKodi->kodi_gui->control_settings_slider->set_int_interval = set_int_interval;

  addonInterface->toKodi->kodi_gui->control_settings_slider->set_percentage = set_percentage;
  addonInterface->toKodi->kodi_gui->control_settings_slider->get_percentage = get_percentage;

  addonInterface->toKodi->kodi_gui->control_settings_slider->set_float_range = set_float_range;
  addonInterface->toKodi->kodi_gui->control_settings_slider->set_float_value = set_float_value;
  addonInterface->toKodi->kodi_gui->control_settings_slider->get_float_value = get_float_value;
  addonInterface->toKodi->kodi_gui->control_settings_slider->set_float_interval = set_float_interval;
}

void Interface_GUIControlSettingsSlider::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->control_settings_slider);
}

void Interface_GUIControlSettingsSlider::set_visible(void* kodiBase, void* handle, bool visible)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISettingsSliderControl* control = static_cast<CGUISettingsSliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSettingsSlider::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetVisible(visible);
}

void Interface_GUIControlSettingsSlider::set_enabled(void* kodiBase, void* handle, bool enabled)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISettingsSliderControl* control = static_cast<CGUISettingsSliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSettingsSlider::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetEnabled(enabled);
}

void Interface_GUIControlSettingsSlider::set_text(void* kodiBase, void* handle, const char *text)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISettingsSliderControl* control = static_cast<CGUISettingsSliderControl*>(handle);
  if (!addon || !control || !text)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSettingsSlider::%s - invalid handler data (kodiBase='%p', handle='%p', text='%p') on addon '%s'",
                          __FUNCTION__, addon, control, text, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  CGUIMessage msg(GUI_MSG_LABEL_SET, control->GetParentID(), control->GetID());
  msg.SetLabel(text);
  g_windowManager.SendThreadMessage(msg, control->GetParentID());
}

void Interface_GUIControlSettingsSlider::reset(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISettingsSliderControl* control = static_cast<CGUISettingsSliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSettingsSlider::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  CGUIMessage msg(GUI_MSG_LABEL_RESET, control->GetParentID(), control->GetID());
  g_windowManager.SendThreadMessage(msg, control->GetParentID());
}

void Interface_GUIControlSettingsSlider::set_int_range(void* kodiBase, void* handle, int start, int end)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISettingsSliderControl* control = static_cast<CGUISettingsSliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSettingsSlider::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetType(SLIDER_CONTROL_TYPE_INT);
  control->SetRange(start, end);
}

void Interface_GUIControlSettingsSlider::set_int_value(void* kodiBase, void* handle, int value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISettingsSliderControl* control = static_cast<CGUISettingsSliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSettingsSlider::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetType(SLIDER_CONTROL_TYPE_INT);
  control->SetIntValue(value);
}

int Interface_GUIControlSettingsSlider::get_int_value(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISettingsSliderControl* control = static_cast<CGUISettingsSliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSettingsSlider::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return -1;
  }

  return control->GetIntValue();
}

void Interface_GUIControlSettingsSlider::set_int_interval(void* kodiBase, void* handle, int interval)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISettingsSliderControl* control = static_cast<CGUISettingsSliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSettingsSlider::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetIntInterval(interval);
}

void Interface_GUIControlSettingsSlider::set_percentage(void* kodiBase, void* handle, float percent)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISettingsSliderControl* control = static_cast<CGUISettingsSliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSettingsSlider::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetType(SLIDER_CONTROL_TYPE_PERCENTAGE);
  control->SetPercentage(percent);
}

float Interface_GUIControlSettingsSlider::get_percentage(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISettingsSliderControl* control = static_cast<CGUISettingsSliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSettingsSlider::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return 0.0f;
  }

  return control->GetPercentage();
}

void Interface_GUIControlSettingsSlider::set_float_range(void* kodiBase, void* handle, float start, float end)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISettingsSliderControl* control = static_cast<CGUISettingsSliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSettingsSlider::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetType(SLIDER_CONTROL_TYPE_FLOAT);
  control->SetFloatRange(start, end);
}

void Interface_GUIControlSettingsSlider::set_float_value(void* kodiBase, void* handle, float value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISettingsSliderControl* control = static_cast<CGUISettingsSliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSettingsSlider::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetType(SLIDER_CONTROL_TYPE_FLOAT);
  control->SetFloatValue(value);
}

float Interface_GUIControlSettingsSlider::get_float_value(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISettingsSliderControl* control = static_cast<CGUISettingsSliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSettingsSlider::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return 0.0f;
  }

  return control->GetFloatValue();
}

void Interface_GUIControlSettingsSlider::set_float_interval(void* kodiBase, void* handle, float interval)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISettingsSliderControl* control = static_cast<CGUISettingsSliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlSettingsSlider::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetFloatInterval(interval);
}

} /* namespace ADDON */
} /* extern "C" */

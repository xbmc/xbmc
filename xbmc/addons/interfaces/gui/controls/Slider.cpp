/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Slider.h"

#include "ServiceBroker.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/gui/controls/Slider.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUISliderControl.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

namespace ADDON
{

void Interface_GUIControlSlider::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->control_slider =
      new AddonToKodiFuncTable_kodi_gui_control_slider();

  addonInterface->toKodi->kodi_gui->control_slider->set_visible = set_visible;
  addonInterface->toKodi->kodi_gui->control_slider->set_enabled = set_enabled;

  addonInterface->toKodi->kodi_gui->control_slider->reset = reset;
  addonInterface->toKodi->kodi_gui->control_slider->get_description = get_description;

  addonInterface->toKodi->kodi_gui->control_slider->set_int_range = set_int_range;
  addonInterface->toKodi->kodi_gui->control_slider->set_int_value = set_int_value;
  addonInterface->toKodi->kodi_gui->control_slider->get_int_value = get_int_value;
  addonInterface->toKodi->kodi_gui->control_slider->set_int_interval = set_int_interval;

  addonInterface->toKodi->kodi_gui->control_slider->set_percentage = set_percentage;
  addonInterface->toKodi->kodi_gui->control_slider->get_percentage = get_percentage;

  addonInterface->toKodi->kodi_gui->control_slider->set_float_range = set_float_range;
  addonInterface->toKodi->kodi_gui->control_slider->set_float_value = set_float_value;
  addonInterface->toKodi->kodi_gui->control_slider->get_float_value = get_float_value;
  addonInterface->toKodi->kodi_gui->control_slider->set_float_interval = set_float_interval;
}

void Interface_GUIControlSlider::DeInit(AddonGlobalInterface* addonInterface)
{
  delete addonInterface->toKodi->kodi_gui->control_slider;
}

void Interface_GUIControlSlider::set_visible(KODI_HANDLE kodiBase,
                                             KODI_GUI_CONTROL_HANDLE handle,
                                             bool visible)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISliderControl* control = static_cast<CGUISliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSlider::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetVisible(visible);
}

void Interface_GUIControlSlider::set_enabled(KODI_HANDLE kodiBase,
                                             KODI_GUI_CONTROL_HANDLE handle,
                                             bool enabled)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISliderControl* control = static_cast<CGUISliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSlider::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetEnabled(enabled);
}

void Interface_GUIControlSlider::reset(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISliderControl* control = static_cast<CGUISliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSlider::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  CGUIMessage msg(GUI_MSG_LABEL_RESET, control->GetParentID(), control->GetID());
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, control->GetParentID());
}

char* Interface_GUIControlSlider::get_description(KODI_HANDLE kodiBase,
                                                  KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISliderControl* control = static_cast<CGUISliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSlider::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return nullptr;
  }

  return strdup(control->GetDescription().c_str());
}

void Interface_GUIControlSlider::set_int_range(KODI_HANDLE kodiBase,
                                               KODI_GUI_CONTROL_HANDLE handle,
                                               int start,
                                               int end)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISliderControl* control = static_cast<CGUISliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSlider::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetType(SLIDER_CONTROL_TYPE_INT);
  control->SetRange(start, end);
}

void Interface_GUIControlSlider::set_int_value(KODI_HANDLE kodiBase,
                                               KODI_GUI_CONTROL_HANDLE handle,
                                               int value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISliderControl* control = static_cast<CGUISliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSlider::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetType(SLIDER_CONTROL_TYPE_INT);
  control->SetIntValue(value);
}

int Interface_GUIControlSlider::get_int_value(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISliderControl* control = static_cast<CGUISliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSlider::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return -1;
  }

  return control->GetIntValue();
}

void Interface_GUIControlSlider::set_int_interval(KODI_HANDLE kodiBase,
                                                  KODI_GUI_CONTROL_HANDLE handle,
                                                  int interval)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISliderControl* control = static_cast<CGUISliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSlider::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetIntInterval(interval);
}

void Interface_GUIControlSlider::set_percentage(KODI_HANDLE kodiBase,
                                                KODI_GUI_CONTROL_HANDLE handle,
                                                float percent)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISliderControl* control = static_cast<CGUISliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSlider::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetType(SLIDER_CONTROL_TYPE_PERCENTAGE);
  control->SetPercentage(percent);
}

float Interface_GUIControlSlider::get_percentage(KODI_HANDLE kodiBase,
                                                 KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISliderControl* control = static_cast<CGUISliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSlider::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return 0.0f;
  }

  return control->GetPercentage();
}

void Interface_GUIControlSlider::set_float_range(KODI_HANDLE kodiBase,
                                                 KODI_GUI_CONTROL_HANDLE handle,
                                                 float start,
                                                 float end)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISliderControl* control = static_cast<CGUISliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSlider::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetType(SLIDER_CONTROL_TYPE_FLOAT);
  control->SetFloatRange(start, end);
}

void Interface_GUIControlSlider::set_float_value(KODI_HANDLE kodiBase,
                                                 KODI_GUI_CONTROL_HANDLE handle,
                                                 float value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISliderControl* control = static_cast<CGUISliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSlider::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetType(SLIDER_CONTROL_TYPE_FLOAT);
  control->SetFloatValue(value);
}

float Interface_GUIControlSlider::get_float_value(KODI_HANDLE kodiBase,
                                                  KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISliderControl* control = static_cast<CGUISliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSlider::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return 0.0f;
  }

  return control->GetFloatValue();
}

void Interface_GUIControlSlider::set_float_interval(KODI_HANDLE kodiBase,
                                                    KODI_GUI_CONTROL_HANDLE handle,
                                                    float interval)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUISliderControl* control = static_cast<CGUISliderControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlSlider::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetFloatInterval(interval);
}

} /* namespace ADDON */

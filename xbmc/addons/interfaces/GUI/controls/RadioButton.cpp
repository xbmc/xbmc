/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RadioButton.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/controls/RadioButton.h"

#include "addons/binary-addons/AddonDll.h"
#include "guilib/GUIRadioButtonControl.h"
#include "utils/log.h"

extern "C"
{
namespace ADDON
{

void Interface_GUIControlRadioButton::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->control_radio_button = static_cast<AddonToKodiFuncTable_kodi_gui_control_radio_button*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_control_radio_button)));

  addonInterface->toKodi->kodi_gui->control_radio_button->set_visible = set_visible;
  addonInterface->toKodi->kodi_gui->control_radio_button->set_enabled = set_enabled;

  addonInterface->toKodi->kodi_gui->control_radio_button->set_label = set_label;
  addonInterface->toKodi->kodi_gui->control_radio_button->get_label = get_label;

  addonInterface->toKodi->kodi_gui->control_radio_button->set_selected = set_selected;
  addonInterface->toKodi->kodi_gui->control_radio_button->is_selected = is_selected;
}

void Interface_GUIControlRadioButton::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->control_radio_button);
}

void Interface_GUIControlRadioButton::set_visible(void* kodiBase, void* handle, bool visible)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIRadioButtonControl* control = static_cast<CGUIRadioButtonControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlRadioButton::%s - invalid handler data (kodiBase='%p', "
              "handle='%p') on addon '%s'",
              __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetVisible(visible);
}

void Interface_GUIControlRadioButton::set_enabled(void* kodiBase, void* handle, bool enabled)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIRadioButtonControl* control = static_cast<CGUIRadioButtonControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlRadioButton::%s - invalid handler data (kodiBase='%p', "
              "handle='%p') on addon '%s'",
              __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetEnabled(enabled);
}

void Interface_GUIControlRadioButton::set_label(void* kodiBase, void* handle, const char *label)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIRadioButtonControl* control = static_cast<CGUIRadioButtonControl*>(handle);
  if (!addon || !control || !label)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlRadioButton::%s - invalid handler data (kodiBase='%p', "
              "handle='%p', label='%p') on addon '%s'",
              __FUNCTION__, kodiBase, handle, label, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetLabel(label);
}

char* Interface_GUIControlRadioButton::get_label(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIRadioButtonControl* control = static_cast<CGUIRadioButtonControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlRadioButton::%s - invalid handler data (kodiBase='%p', "
              "handle='%p') on addon '%s'",
              __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
    return nullptr;
  }

  return strdup(control->GetLabel().c_str());
}

void Interface_GUIControlRadioButton::set_selected(void* kodiBase, void* handle, bool selected)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIRadioButtonControl* control = static_cast<CGUIRadioButtonControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlRadioButton::%s - invalid handler data (kodiBase='%p', "
              "handle='%p') on addon '%s'",
              __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetSelected(selected);
}

bool Interface_GUIControlRadioButton::is_selected(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIRadioButtonControl* control = static_cast<CGUIRadioButtonControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlRadioButton::%s - invalid handler data (kodiBase='%p', "
              "handle='%p') on addon '%s'",
              __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
    return false;
  }

  return control->IsSelected();
}

} /* namespace ADDON */
} /* extern "C" */

/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RadioButton.h"

#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/gui/controls/RadioButton.h"
#include "guilib/GUIRadioButtonControl.h"
#include "utils/log.h"

namespace ADDON
{

void Interface_GUIControlRadioButton::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->control_radio_button =
      new AddonToKodiFuncTable_kodi_gui_control_radio_button();

  addonInterface->toKodi->kodi_gui->control_radio_button->set_visible = set_visible;
  addonInterface->toKodi->kodi_gui->control_radio_button->set_enabled = set_enabled;

  addonInterface->toKodi->kodi_gui->control_radio_button->set_label = set_label;
  addonInterface->toKodi->kodi_gui->control_radio_button->get_label = get_label;

  addonInterface->toKodi->kodi_gui->control_radio_button->set_selected = set_selected;
  addonInterface->toKodi->kodi_gui->control_radio_button->is_selected = is_selected;
}

void Interface_GUIControlRadioButton::DeInit(AddonGlobalInterface* addonInterface)
{
  delete addonInterface->toKodi->kodi_gui->control_radio_button;
}

void Interface_GUIControlRadioButton::set_visible(KODI_HANDLE kodiBase,
                                                  KODI_GUI_CONTROL_HANDLE handle,
                                                  bool visible)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIRadioButtonControl* control = static_cast<CGUIRadioButtonControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlRadioButton::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetVisible(visible);
}

void Interface_GUIControlRadioButton::set_enabled(KODI_HANDLE kodiBase,
                                                  KODI_GUI_CONTROL_HANDLE handle,
                                                  bool enabled)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIRadioButtonControl* control = static_cast<CGUIRadioButtonControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlRadioButton::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetEnabled(enabled);
}

void Interface_GUIControlRadioButton::set_label(KODI_HANDLE kodiBase,
                                                KODI_GUI_CONTROL_HANDLE handle,
                                                const char* label)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIRadioButtonControl* control = static_cast<CGUIRadioButtonControl*>(handle);
  if (!addon || !control || !label)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlRadioButton::{} - invalid handler data (kodiBase='{}', "
              "handle='{}', label='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(label),
              addon ? addon->ID() : "unknown");
    return;
  }

  control->SetLabel(label);
}

char* Interface_GUIControlRadioButton::get_label(KODI_HANDLE kodiBase,
                                                 KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIRadioButtonControl* control = static_cast<CGUIRadioButtonControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlRadioButton::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return nullptr;
  }

  return strdup(control->GetLabel().c_str());
}

void Interface_GUIControlRadioButton::set_selected(KODI_HANDLE kodiBase,
                                                   KODI_GUI_CONTROL_HANDLE handle,
                                                   bool selected)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIRadioButtonControl* control = static_cast<CGUIRadioButtonControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlRadioButton::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetSelected(selected);
}

bool Interface_GUIControlRadioButton::is_selected(KODI_HANDLE kodiBase,
                                                  KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIRadioButtonControl* control = static_cast<CGUIRadioButtonControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlRadioButton::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return false;
  }

  return control->IsSelected();
}

} /* namespace ADDON */

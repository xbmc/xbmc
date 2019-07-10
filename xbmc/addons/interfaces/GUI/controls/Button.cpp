/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Button.h"

#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/controls/Button.h"
#include "guilib/GUIButtonControl.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

extern "C"
{
namespace ADDON
{

void Interface_GUIControlButton::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->control_button = static_cast<AddonToKodiFuncTable_kodi_gui_control_button*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_control_button)));

  addonInterface->toKodi->kodi_gui->control_button->set_visible = set_visible;
  addonInterface->toKodi->kodi_gui->control_button->set_enabled = set_enabled;

  addonInterface->toKodi->kodi_gui->control_button->set_label = set_label;
  addonInterface->toKodi->kodi_gui->control_button->get_label = get_label;

  addonInterface->toKodi->kodi_gui->control_button->set_label2 = set_label2;
  addonInterface->toKodi->kodi_gui->control_button->get_label2 = get_label2;
}

void Interface_GUIControlButton::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->control_button);
}

void Interface_GUIControlButton::set_visible(void* kodiBase, void* handle, bool visible)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIButtonControl* control = static_cast<CGUIButtonControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlButton::%s - invalid handler data (kodiBase='%p', handle='%p') "
              "on addon '%s'",
              __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetVisible(visible);
}

void Interface_GUIControlButton::set_enabled(void* kodiBase, void* handle, bool enabled)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIButtonControl* control = static_cast<CGUIButtonControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlButton::%s - invalid handler data (kodiBase='%p', handle='%p') "
              "on addon '%s'",
              __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetEnabled(enabled);
}

void Interface_GUIControlButton::set_label(void* kodiBase, void* handle, const char* label)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIButtonControl* control = static_cast<CGUIButtonControl*>(handle);
  if (!addon || !control || !label)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlButton::%s - invalid handler data (kodiBase='%p', handle='%p', "
              "label='%p') on addon '%s'",
              __FUNCTION__, kodiBase, handle, label, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetLabel(label);
}

char* Interface_GUIControlButton::get_label(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIButtonControl* control = static_cast<CGUIButtonControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlButton::%s - invalid handler data (kodiBase='%p', handle='%p') "
              "on addon '%s'",
              __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
    return nullptr;
  }

  return strdup(control->GetLabel().c_str());
}

void Interface_GUIControlButton::set_label2(void* kodiBase, void* handle, const char* label)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIButtonControl* control = static_cast<CGUIButtonControl*>(handle);
  if (!addon || !control || !label)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlButton::%s - invalid handler data (kodiBase='%p', handle='%p', "
              "label='%p') on addon '%s'",
              __FUNCTION__, kodiBase, handle, label, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetLabel2(label);
}

char* Interface_GUIControlButton::get_label2(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIButtonControl* control = static_cast<CGUIButtonControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlButton::%s - invalid handler data (kodiBase='%p', handle='%p') "
              "on addon '%s'",
              __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
    return nullptr;
  }

  return strdup(control->GetLabel2().c_str());
}

} /* namespace ADDON */
} /* extern "C" */

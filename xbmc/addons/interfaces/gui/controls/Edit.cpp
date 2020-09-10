/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Edit.h"

#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/gui/controls/Edit.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

namespace ADDON
{

void Interface_GUIControlEdit::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->control_edit = new AddonToKodiFuncTable_kodi_gui_control_edit();

  addonInterface->toKodi->kodi_gui->control_edit->set_visible = set_visible;
  addonInterface->toKodi->kodi_gui->control_edit->set_enabled = set_enabled;
  addonInterface->toKodi->kodi_gui->control_edit->set_input_type = set_input_type;
  addonInterface->toKodi->kodi_gui->control_edit->set_label = set_label;
  addonInterface->toKodi->kodi_gui->control_edit->get_label = get_label;
  addonInterface->toKodi->kodi_gui->control_edit->set_text = set_text;
  addonInterface->toKodi->kodi_gui->control_edit->get_text = get_text;
  addonInterface->toKodi->kodi_gui->control_edit->set_cursor_position = set_cursor_position;
  addonInterface->toKodi->kodi_gui->control_edit->get_cursor_position = get_cursor_position;
}

void Interface_GUIControlEdit::DeInit(AddonGlobalInterface* addonInterface)
{
  delete addonInterface->toKodi->kodi_gui->control_edit;
}

void Interface_GUIControlEdit::set_visible(KODI_HANDLE kodiBase,
                                           KODI_GUI_CONTROL_HANDLE handle,
                                           bool visible)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIEditControl* control = static_cast<CGUIEditControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlEdit::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetVisible(visible);
}

void Interface_GUIControlEdit::set_enabled(KODI_HANDLE kodiBase,
                                           KODI_GUI_CONTROL_HANDLE handle,
                                           bool enable)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIEditControl* control = static_cast<CGUIEditControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlEdit::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetEnabled(enable);
}

void Interface_GUIControlEdit::set_input_type(KODI_HANDLE kodiBase,
                                              KODI_GUI_CONTROL_HANDLE handle,
                                              int type,
                                              const char* heading)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIEditControl* control = static_cast<CGUIEditControl*>(handle);
  if (!addon || !control || !heading)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlEdit::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "heading='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(heading),
              addon ? addon->ID() : "unknown");
    return;
  }

  CGUIEditControl::INPUT_TYPE kodiType;
  switch (static_cast<AddonGUIInputType>(type))
  {
    case ADDON_INPUT_TYPE_TEXT:
      kodiType = CGUIEditControl::INPUT_TYPE_TEXT;
      break;
    case ADDON_INPUT_TYPE_NUMBER:
      kodiType = CGUIEditControl::INPUT_TYPE_NUMBER;
      break;
    case ADDON_INPUT_TYPE_SECONDS:
      kodiType = CGUIEditControl::INPUT_TYPE_SECONDS;
      break;
    case ADDON_INPUT_TYPE_TIME:
      kodiType = CGUIEditControl::INPUT_TYPE_TIME;
      break;
    case ADDON_INPUT_TYPE_DATE:
      kodiType = CGUIEditControl::INPUT_TYPE_DATE;
      break;
    case ADDON_INPUT_TYPE_IPADDRESS:
      kodiType = CGUIEditControl::INPUT_TYPE_IPADDRESS;
      break;
    case ADDON_INPUT_TYPE_PASSWORD:
      kodiType = CGUIEditControl::INPUT_TYPE_PASSWORD;
      break;
    case ADDON_INPUT_TYPE_PASSWORD_MD5:
      kodiType = CGUIEditControl::INPUT_TYPE_PASSWORD_MD5;
      break;
    case ADDON_INPUT_TYPE_SEARCH:
      kodiType = CGUIEditControl::INPUT_TYPE_SEARCH;
      break;
    case ADDON_INPUT_TYPE_FILTER:
      kodiType = CGUIEditControl::INPUT_TYPE_FILTER;
      break;
    case ADDON_INPUT_TYPE_READONLY:
    default:
      kodiType = CGUIEditControl::INPUT_TYPE_PASSWORD_NUMBER_VERIFY_NEW;
  }

  control->SetInputType(kodiType, heading);
}

void Interface_GUIControlEdit::set_label(KODI_HANDLE kodiBase,
                                         KODI_GUI_CONTROL_HANDLE handle,
                                         const char* label)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIEditControl* control = static_cast<CGUIEditControl*>(handle);
  if (!addon || !control || !label)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlEdit::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "label='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(label),
              addon ? addon->ID() : "unknown");
    return;
  }

  control->SetLabel(label);
}

char* Interface_GUIControlEdit::get_label(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIEditControl* control = static_cast<CGUIEditControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlEdit::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return nullptr;
  }

  return strdup(control->GetLabel().c_str());
}

void Interface_GUIControlEdit::set_text(KODI_HANDLE kodiBase,
                                        KODI_GUI_CONTROL_HANDLE handle,
                                        const char* text)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIEditControl* control = static_cast<CGUIEditControl*>(handle);
  if (!addon || !control || !text)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlEdit::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "text='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(text),
              addon ? addon->ID() : "unknown");
    return;
  }

  control->SetLabel2(text);
}

char* Interface_GUIControlEdit::get_text(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIEditControl* control = static_cast<CGUIEditControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlEdit::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return nullptr;
  }

  return strdup(control->GetLabel2().c_str());
}

void Interface_GUIControlEdit::set_cursor_position(KODI_HANDLE kodiBase,
                                                   KODI_GUI_CONTROL_HANDLE handle,
                                                   unsigned int position)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIEditControl* control = static_cast<CGUIEditControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlEdit::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetCursorPosition(position);
}

unsigned int Interface_GUIControlEdit::get_cursor_position(KODI_HANDLE kodiBase,
                                                           KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIEditControl* control = static_cast<CGUIEditControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlEdit::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return 0;
  }

  return control->GetCursorPosition();
}

} /* namespace ADDON */

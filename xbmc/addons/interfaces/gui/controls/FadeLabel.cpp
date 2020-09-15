/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FadeLabel.h"

#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/gui/controls/FadeLabel.h"
#include "guilib/GUIFadeLabelControl.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

namespace ADDON
{

void Interface_GUIControlFadeLabel::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->control_fade_label =
      new AddonToKodiFuncTable_kodi_gui_control_fade_label();

  addonInterface->toKodi->kodi_gui->control_fade_label->set_visible = set_visible;
  addonInterface->toKodi->kodi_gui->control_fade_label->add_label = add_label;
  addonInterface->toKodi->kodi_gui->control_fade_label->get_label = get_label;
  addonInterface->toKodi->kodi_gui->control_fade_label->set_scrolling = set_scrolling;
  addonInterface->toKodi->kodi_gui->control_fade_label->reset = reset;
}

void Interface_GUIControlFadeLabel::DeInit(AddonGlobalInterface* addonInterface)
{
  delete addonInterface->toKodi->kodi_gui->control_fade_label;
}

void Interface_GUIControlFadeLabel::set_visible(KODI_HANDLE kodiBase,
                                                KODI_GUI_CONTROL_HANDLE handle,
                                                bool visible)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIFadeLabelControl* control = static_cast<CGUIFadeLabelControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlFadeLabel::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetVisible(visible);
}

void Interface_GUIControlFadeLabel::add_label(KODI_HANDLE kodiBase,
                                              KODI_GUI_CONTROL_HANDLE handle,
                                              const char* label)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIFadeLabelControl* control = static_cast<CGUIFadeLabelControl*>(handle);
  if (!addon || !control || !label)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlFadeLabel::{} - invalid handler data (kodiBase='{}', "
              "handle='{}', label='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(label),
              addon ? addon->ID() : "unknown");
    return;
  }

  CGUIMessage msg(GUI_MSG_LABEL_ADD, control->GetParentID(), control->GetID());
  msg.SetLabel(label);
  control->OnMessage(msg);
}

char* Interface_GUIControlFadeLabel::get_label(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIFadeLabelControl* control = static_cast<CGUIFadeLabelControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlFadeLabel::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return nullptr;
  }

  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, control->GetParentID(), control->GetID());
  control->OnMessage(msg);
  std::string text = msg.GetLabel();
  return strdup(text.c_str());
}

void Interface_GUIControlFadeLabel::set_scrolling(KODI_HANDLE kodiBase,
                                                  KODI_GUI_CONTROL_HANDLE handle,
                                                  bool scroll)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIFadeLabelControl* control = static_cast<CGUIFadeLabelControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlFadeLabel::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetScrolling(scroll);
}

void Interface_GUIControlFadeLabel::reset(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIFadeLabelControl* control = static_cast<CGUIFadeLabelControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlFadeLabel::{} - invalid handler data (kodiBase='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  CGUIMessage msg(GUI_MSG_LABEL_RESET, control->GetParentID(), control->GetID());
  control->OnMessage(msg);
}

} /* namespace ADDON */

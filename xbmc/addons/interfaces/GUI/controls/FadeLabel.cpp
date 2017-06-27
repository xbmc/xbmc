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

#include "FadeLabel.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/controls/FadeLabel.h"

#include "addons/binary-addons/AddonDll.h"
#include "guilib/GUIFadeLabelControl.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

extern "C"
{
namespace ADDON
{

void Interface_GUIControlFadeLabel::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->control_fade_label = static_cast<AddonToKodiFuncTable_kodi_gui_control_fade_label*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_control_fade_label)));

  addonInterface->toKodi->kodi_gui->control_fade_label->set_visible = set_visible;
  addonInterface->toKodi->kodi_gui->control_fade_label->add_label = add_label;
  addonInterface->toKodi->kodi_gui->control_fade_label->get_label = get_label;
  addonInterface->toKodi->kodi_gui->control_fade_label->set_scrolling = set_scrolling;
  addonInterface->toKodi->kodi_gui->control_fade_label->reset = reset;
}

void Interface_GUIControlFadeLabel::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->control_fade_label);
}

void Interface_GUIControlFadeLabel::set_visible(void* kodiBase, void* handle, bool visible)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIFadeLabelControl* control = static_cast<CGUIFadeLabelControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlFadeLabel::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetVisible(visible);
}

void Interface_GUIControlFadeLabel::add_label(void* kodiBase, void* handle, const char *label)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIFadeLabelControl* control = static_cast<CGUIFadeLabelControl*>(handle);
  if (!addon || !control | !label)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlFadeLabel::%s - invalid handler data (kodiBase='%p', handle='%p', label='%p') on addon '%s'",
                          __FUNCTION__, addon, control, label, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  CGUIMessage msg(GUI_MSG_LABEL_ADD, control->GetParentID(), control->GetID());
  msg.SetLabel(label);
  control->OnMessage(msg);
}

char* Interface_GUIControlFadeLabel::get_label(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIFadeLabelControl* control = static_cast<CGUIFadeLabelControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlFadeLabel::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return nullptr;
  }

  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, control->GetParentID(), control->GetID());
  control->OnMessage(msg);
  std::string text = msg.GetLabel();
  return strdup(text.c_str());
}

void Interface_GUIControlFadeLabel::set_scrolling(void* kodiBase, void* handle, bool scroll)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIFadeLabelControl* control = static_cast<CGUIFadeLabelControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlFadeLabel::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetScrolling(scroll);
}

void Interface_GUIControlFadeLabel::reset(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIFadeLabelControl* control = static_cast<CGUIFadeLabelControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlFadeLabel::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  CGUIMessage msg(GUI_MSG_LABEL_RESET, control->GetParentID(), control->GetID());
  control->OnMessage(msg);
}

} /* namespace ADDON */
} /* extern "C" */

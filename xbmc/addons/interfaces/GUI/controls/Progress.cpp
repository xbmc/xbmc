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

#include "Progress.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/controls/Progress.h"

#include "addons/AddonDll.h"
#include "guilib/GUIProgressControl.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

extern "C"
{
namespace ADDON
{

void Interface_GUIControlProgress::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->control_progress = static_cast<AddonToKodiFuncTable_kodi_gui_control_progress*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_control_progress)));

  addonInterface->toKodi->kodi_gui->control_progress->set_visible = set_visible;
  addonInterface->toKodi->kodi_gui->control_progress->set_percentage = set_percentage;
  addonInterface->toKodi->kodi_gui->control_progress->get_percentage = get_percentage;
}

void Interface_GUIControlProgress::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->control_progress);
}

void Interface_GUIControlProgress::set_visible(void* kodiBase, void* handle, bool visible)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIProgressControl* control = static_cast<CGUIProgressControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlProgress::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetVisible(visible);
}

void Interface_GUIControlProgress::set_percentage(void* kodiBase, void* handle, float percent)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIProgressControl* control = static_cast<CGUIProgressControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlProgress::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetPercentage(percent);
}

float Interface_GUIControlProgress::get_percentage(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIProgressControl* control = static_cast<CGUIProgressControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlProgress::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return 0.0f;
  }

  return control->GetPercentage();
}

} /* namespace ADDON */
} /* extern "C" */

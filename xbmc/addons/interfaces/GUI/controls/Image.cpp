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

#include "Image.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/controls/Image.h"

#include "addons/binary-addons/AddonDll.h"
#include "guilib/GUIImage.h"
#include "utils/log.h"

extern "C"
{
namespace ADDON
{

void Interface_GUIControlImage::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->control_image = static_cast<AddonToKodiFuncTable_kodi_gui_control_image*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_control_image)));

  addonInterface->toKodi->kodi_gui->control_image->set_visible = set_visible;
  addonInterface->toKodi->kodi_gui->control_image->set_filename = set_filename;
  addonInterface->toKodi->kodi_gui->control_image->set_color_diffuse = set_color_diffuse;
}

void Interface_GUIControlImage::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->control_image);
}

void Interface_GUIControlImage::set_visible(void* kodiBase, void* handle, bool visible)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIImage* control = static_cast<CGUIImage*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlImage::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetVisible(visible);
}

void Interface_GUIControlImage::set_filename(void* kodiBase, void* handle, const char* filename, bool use_cache)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIImage* control = static_cast<CGUIImage*>(handle);
  if (!addon || !control || !filename)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlImage::%s - invalid handler data (kodiBase='%p', handle='%p', filename='%p') on addon '%s'",
                          __FUNCTION__, addon, control, filename, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetFileName(filename, false, use_cache);
}

void Interface_GUIControlImage::set_color_diffuse(void* kodiBase, void* handle, uint32_t colorDiffuse)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIImage* control = static_cast<CGUIImage*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR, "Interface_GUIControlImage::%s - invalid handler data (kodiBase='%p', handle='%p') on addon '%s'",
                          __FUNCTION__, addon, control, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetColorDiffuse(colorDiffuse);
}

} /* namespace ADDON */
} /* extern "C" */

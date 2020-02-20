/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Image.h"

#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/controls/Image.h"
#include "guilib/GUIImage.h"
#include "utils/log.h"

using namespace KODI;

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
    CLog::Log(LOGERROR,
              "Interface_GUIControlImage::%s - invalid handler data (kodiBase='%p', handle='%p') "
              "on addon '%s'",
              __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
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
    CLog::Log(LOGERROR,
              "Interface_GUIControlImage::%s - invalid handler data (kodiBase='%p', handle='%p', "
              "filename='%p') on addon '%s'",
              __FUNCTION__, kodiBase, handle, filename, addon ? addon->ID().c_str() : "unknown");
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
    CLog::Log(LOGERROR,
              "Interface_GUIControlImage::%s - invalid handler data (kodiBase='%p', handle='%p') "
              "on addon '%s'",
              __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  control->SetColorDiffuse(GUILIB::GUIINFO::CGUIInfoColor(colorDiffuse));
}

} /* namespace ADDON */
} /* extern "C" */

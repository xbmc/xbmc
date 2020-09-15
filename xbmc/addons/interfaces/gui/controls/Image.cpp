/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Image.h"

#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/gui/controls/Image.h"
#include "guilib/GUIImage.h"
#include "utils/log.h"

using namespace KODI;

namespace ADDON
{

void Interface_GUIControlImage::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->control_image =
      new AddonToKodiFuncTable_kodi_gui_control_image();

  addonInterface->toKodi->kodi_gui->control_image->set_visible = set_visible;
  addonInterface->toKodi->kodi_gui->control_image->set_filename = set_filename;
  addonInterface->toKodi->kodi_gui->control_image->set_color_diffuse = set_color_diffuse;
}

void Interface_GUIControlImage::DeInit(AddonGlobalInterface* addonInterface)
{
  delete addonInterface->toKodi->kodi_gui->control_image;
}

void Interface_GUIControlImage::set_visible(KODI_HANDLE kodiBase,
                                            KODI_GUI_CONTROL_HANDLE handle,
                                            bool visible)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIImage* control = static_cast<CGUIImage*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlImage::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetVisible(visible);
}

void Interface_GUIControlImage::set_filename(KODI_HANDLE kodiBase,
                                             KODI_GUI_CONTROL_HANDLE handle,
                                             const char* filename,
                                             bool use_cache)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIImage* control = static_cast<CGUIImage*>(handle);
  if (!addon || !control || !filename)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlImage::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "filename='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(filename),
              addon ? addon->ID() : "unknown");
    return;
  }

  control->SetFileName(filename, false, use_cache);
}

void Interface_GUIControlImage::set_color_diffuse(KODI_HANDLE kodiBase,
                                                  KODI_GUI_CONTROL_HANDLE handle,
                                                  uint32_t colorDiffuse)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUIImage* control = static_cast<CGUIImage*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlImage::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetColorDiffuse(GUILIB::GUIINFO::CGUIInfoColor(colorDiffuse));
}

} /* namespace ADDON */

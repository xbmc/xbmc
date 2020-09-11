/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/gui/controls/button.h"

extern "C"
{

  struct AddonGlobalInterface;

  namespace ADDON
  {

  /*!
   * @brief Global gui Add-on to Kodi callback functions
   *
   * To hold general gui functions and initialize also all other gui related types not
   * related to a instance type and usable for every add-on type.
   *
   * Related add-on header is "./xbmc/addons/kodi-dev-kit/include/kodi/gui/controls/Button.h"
   */
  struct Interface_GUIControlButton
  {
    static void Init(AddonGlobalInterface* addonInterface);
    static void DeInit(AddonGlobalInterface* addonInterface);

    /*!
     * @brief callback functions from add-on to kodi
     *
     * @note To add a new function use the "_" style to directly identify an
     * add-on callback function. Everything with CamelCase is only to be used
     * in Kodi.
     *
     * The parameter `kodiBase` is used to become the pointer for a `CAddonDll`
     * class.
     */
    //@{
    static void set_visible(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool visible);
    static void set_enabled(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool enabled);

    static void set_label(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, const char* label);
    static char* get_label(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    static void set_label2(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, const char* label);
    static char* get_label2(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    //@}
  };

  } /* namespace ADDON */
} /* extern "C" */

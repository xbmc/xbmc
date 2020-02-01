/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

extern "C"
{

struct AddonGlobalInterface;

namespace ADDON
{

  /*!
   * @brief Global gui Add-on to Kodi callback functions
   *
   * To hold functions not related to a instance type and usable for
   * every add-on type.
   *
   * Related add-on header is "./xbmc/addons/kodi-addon-dev-kit/include/kodi/gui/dialogs/Progress.h"
   */
  struct Interface_GUIDialogProgress
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
    static void* new_dialog(void* kodiBase);
    static void delete_dialog(void* kodiBase, void* handle);
    static void open(void* kodiBase, void* handle);
    static void set_heading(void* kodiBase, void* handle, const char* heading);
    static void set_line(void* kodiBase, void* handle, unsigned int line, const char* text);
    static void set_can_cancel(void* kodiBase, void* handle, bool canCancel);
    static bool is_canceled(void* kodiBase, void* handle);
    static void set_percentage(void* kodiBase, void* handle, int percentage);
    static int get_percentage(void* kodiBase, void* handle);
    static void show_progress_bar(void* kodiBase, void* handle, bool bOnOff);
    static void set_progress_max(void* kodiBase, void* handle, int max);
    static void set_progress_advance(void* kodiBase, void* handle, int nSteps);
    static bool abort(void* kodiBase, void* handle);
    //@}
  };

} /* namespace ADDON */
} /* extern "C" */

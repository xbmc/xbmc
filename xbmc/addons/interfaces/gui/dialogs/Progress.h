/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/gui/dialogs/progress.h"

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
   * Related add-on header is "./xbmc/addons/kodi-dev-kit/include/kodi/gui/dialogs/Progress.h"
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
    static KODI_GUI_HANDLE new_dialog(KODI_HANDLE kodiBase);
    static void delete_dialog(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle);
    static void open(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle);
    static void set_heading(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle, const char* heading);
    static void set_line(KODI_HANDLE kodiBase,
                         KODI_GUI_HANDLE handle,
                         unsigned int line,
                         const char* text);
    static void set_can_cancel(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle, bool canCancel);
    static bool is_canceled(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle);
    static void set_percentage(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle, int percentage);
    static int get_percentage(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle);
    static void show_progress_bar(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle, bool bOnOff);
    static void set_progress_max(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle, int max);
    static void set_progress_advance(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle, int nSteps);
    static bool abort(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle);
    //@}
  };

  } /* namespace ADDON */
} /* extern "C" */

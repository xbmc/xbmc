/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <time.h>

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
   * Related add-on header is "./xbmc/addons/kodi-addon-dev-kit/include/kodi/gui/dialogs/Numeric.h"
   */
  struct Interface_GUIDialogNumeric
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
    static bool show_and_verify_new_password(void* kodiBase, char** password);
    static int show_and_verify_password(void* kodiBase, const char* password, const char *heading, int retries);
    static bool show_and_verify_input(void* kodiBase, const char* verify_in, char** verify_out, const char* heading, bool verify_input);
    static bool show_and_get_time(void* kodiBase, tm *time, const char *heading);
    static bool show_and_get_date(void* kodiBase, tm *date, const char *heading);
    static bool show_and_get_ip_address(void* kodiBase, const char* ip_address_in, char** ip_address_out, const char *heading);
    static bool show_and_get_number(void* kodiBase, const char* number_in, char** number_out, const char *heading, unsigned int auto_close_ms);
    static bool show_and_get_seconds(void* kodiBase, const char* time_in, char** time_out, const char *heading);
    //@}
  };

} /* namespace ADDON */
} /* extern "C" */

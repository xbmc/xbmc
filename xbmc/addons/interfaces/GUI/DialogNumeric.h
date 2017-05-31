#pragma once
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
   * Related add-on header is "./xbmc/addons/kodi-addon-dev-kit/include/kodi/gui/DialogNumeric.h"
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

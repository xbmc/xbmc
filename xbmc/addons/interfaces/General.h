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

extern "C"
{

struct AddonGlobalInterface;

namespace ADDON
{

  /*!
   * @brief Global general Add-on to Kodi callback functions
   *
   * To hold general functions not related to a instance type and usable for
   * every add-on type.
   *
   * Related add-on header is "./xbmc/addons/kodi-addon-dev-kit/include/kodi/General.h"
   */
  struct Interface_General
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
    static bool open_settings_dialog(void* kodiBase);
    static char* get_localized_string(void* kodiBase, long dwCode);
    static char* unknown_to_utf8(void* kodiBase, const char* source, bool* ret, bool failOnBadChar);
    static char* get_language(void* kodiBase, int format, bool region);
    static bool queue_notification(void* kodiBase, int type, const char* header, const char* message, const char* imageFile, unsigned int displayTime, bool withSound, unsigned int messageTime);
    static void get_md5(void* kodiBase, const char* text, char* md5);
    //@}
  };

} /* namespace ADDON */
} /* extern "C" */

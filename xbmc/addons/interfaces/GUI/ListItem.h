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
   * Related add-on header is "./xbmc/addons/kodi-addon-dev-kit/include/kodi/gui/ListItem.h"
   */
  struct Interface_GUIListItem
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
    static void* create(void* kodiBase, const char* label, const char* label2, const char* icon_image, const char* path);
    static void destroy(void* kodiBase, void* handle);
    static char* get_label(void* kodiBase, void* handle);
    static void set_label(void* kodiBase, void* handle, const char* label);
    static char* get_label2(void* kodiBase, void* handle);
    static void set_label2(void* kodiBase, void* handle, const char* label);
    static char* get_icon_image(void* kodiBase, void* handle);
    static void set_icon_image(void* kodiBase, void* handle, const char* image);
    static char* get_art(void* kodiBase, void* handle, const char* type);
    static void set_art(void* kodiBase, void* handle, const char* type, const char* image);
    static char* get_path(void* kodiBase, void* handle);
    static void set_path(void* kodiBase, void* handle, const char* path);
    static char* get_property(void* kodiBase, void* handle, const char* key);
    static void set_property(void* kodiBase, void* handle, const char* key, const char* value);
    static void select(void* kodiBase, void* handle, bool select);
    static bool is_selected(void* kodiBase, void* handle);
    //@}
  };

} /* namespace ADDON */
} /* extern "C" */

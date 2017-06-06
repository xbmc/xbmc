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
   * @brief Global gui Add-on to Kodi callback functions
   *
   * To hold functions not related to a instance type and usable for
   * every add-on type.
   *
   * Related add-on header is "./xbmc/addons/kodi-addon-dev-kit/include/kodi/gui/dialogs/Keyboard.h"
   */
  struct Interface_GUIDialogKeyboard
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
    static bool show_and_get_input_with_head(void* kodiBase, const char* text_in, char** text_out, const char* heading, bool allow_empty_result, bool hidden_input, unsigned int auto_close_ms);
    static bool show_and_get_input(void* kodiBase, const char* text_in, char** text_out, bool allow_empty_result, unsigned int auto_close_ms);
    static bool show_and_get_new_password_with_head(void* kodiBase, const char* password_in, char** password_out, const char* heading, bool allow_empty_result, unsigned int auto_close_ms);
    static bool show_and_get_new_password(void* kodiBase, const char* password_in, char** password_out, unsigned int auto_close_ms);
    static bool show_and_verify_new_password_with_head(void* kodiBase, char** password_out, const char* heading, bool allowEmpty, unsigned int auto_close_ms);
    static bool show_and_verify_new_password(void* kodiBase, char** password_out, unsigned int auto_close_ms);
    static int show_and_verify_password(void* kodiBase, const char* password_in, char** password_out, const char* heading, int retries, unsigned int auto_close_ms);
    static bool show_and_get_filter(void* kodiBase, const char* text_in, char** text_out, bool searching, unsigned int auto_close_ms);
    static bool send_text_to_active_keyboard(void* kodiBase, const char* text, bool close_keyboard);
    static bool is_keyboard_activated(void* kodiBase);
    //@}
  };

} /* namespace ADDON */
} /* extern "C" */

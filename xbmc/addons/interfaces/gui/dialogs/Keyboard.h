/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/gui/dialogs/keyboard.h"

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
   * Related add-on header is "./xbmc/addons/kodi-dev-kit/include/kodi/gui/dialogs/Keyboard.h"
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
    static bool show_and_get_input_with_head(KODI_HANDLE kodiBase,
                                             const char* text_in,
                                             char** text_out,
                                             const char* heading,
                                             bool allow_empty_result,
                                             bool hidden_input,
                                             unsigned int auto_close_ms);
    static bool show_and_get_input(KODI_HANDLE kodiBase,
                                   const char* text_in,
                                   char** text_out,
                                   bool allow_empty_result,
                                   unsigned int auto_close_ms);
    static bool show_and_get_new_password_with_head(KODI_HANDLE kodiBase,
                                                    const char* password_in,
                                                    char** password_out,
                                                    const char* heading,
                                                    bool allow_empty_result,
                                                    unsigned int auto_close_ms);
    static bool show_and_get_new_password(KODI_HANDLE kodiBase,
                                          const char* password_in,
                                          char** password_out,
                                          unsigned int auto_close_ms);
    static bool show_and_verify_new_password_with_head(KODI_HANDLE kodiBase,
                                                       char** password_out,
                                                       const char* heading,
                                                       bool allowEmpty,
                                                       unsigned int auto_close_ms);
    static bool show_and_verify_new_password(KODI_HANDLE kodiBase,
                                             char** password_out,
                                             unsigned int auto_close_ms);
    static int show_and_verify_password(KODI_HANDLE kodiBase,
                                        const char* password_in,
                                        char** password_out,
                                        const char* heading,
                                        int retries,
                                        unsigned int auto_close_ms);
    static bool show_and_get_filter(KODI_HANDLE kodiBase,
                                    const char* text_in,
                                    char** text_out,
                                    bool searching,
                                    unsigned int auto_close_ms);
    static bool send_text_to_active_keyboard(KODI_HANDLE kodiBase,
                                             const char* text,
                                             bool close_keyboard);
    static bool is_keyboard_activated(KODI_HANDLE kodiBase);
    //@}
  };

  } /* namespace ADDON */
} /* extern "C" */

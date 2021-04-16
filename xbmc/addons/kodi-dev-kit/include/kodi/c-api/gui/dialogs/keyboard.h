/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_GUI_DIALOGS_KEYBOARD_H
#define C_API_GUI_DIALOGS_KEYBOARD_H

#include "../definitions.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef struct AddonToKodiFuncTable_kodi_gui_dialogKeyboard
  {
    bool (*show_and_get_input_with_head)(KODI_HANDLE kodiBase,
                                         const char* text_in,
                                         char** text_out,
                                         const char* heading,
                                         bool allow_empty_result,
                                         bool hiddenInput,
                                         unsigned int auto_close_ms);
    bool (*show_and_get_input)(KODI_HANDLE kodiBase,
                               const char* text_in,
                               char** text_out,
                               bool allow_empty_result,
                               unsigned int auto_close_ms);
    bool (*show_and_get_new_password_with_head)(KODI_HANDLE kodiBase,
                                                const char* password_in,
                                                char** password_out,
                                                const char* heading,
                                                bool allow_empty_result,
                                                unsigned int auto_close_ms);
    bool (*show_and_get_new_password)(KODI_HANDLE kodiBase,
                                      const char* password_in,
                                      char** password_out,
                                      unsigned int auto_close_ms);
    bool (*show_and_verify_new_password_with_head)(KODI_HANDLE kodiBase,
                                                   char** password_out,
                                                   const char* heading,
                                                   bool allow_empty_result,
                                                   unsigned int auto_close_ms);
    bool (*show_and_verify_new_password)(KODI_HANDLE kodiBase,
                                         char** password_out,
                                         unsigned int auto_close_ms);
    int (*show_and_verify_password)(KODI_HANDLE kodiBase,
                                    const char* password_in,
                                    char** password_out,
                                    const char* heading,
                                    int retries,
                                    unsigned int auto_close_ms);
    bool (*show_and_get_filter)(KODI_HANDLE kodiBase,
                                const char* text_in,
                                char** text_out,
                                bool searching,
                                unsigned int auto_close_ms);
    bool (*send_text_to_active_keyboard)(KODI_HANDLE kodiBase,
                                         const char* text,
                                         bool close_keyboard);
    bool (*is_keyboard_activated)(KODI_HANDLE kodiBase);
  } AddonToKodiFuncTable_kodi_gui_dialogKeyboard;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_GUI_DIALOGS_KEYBOARD_H */

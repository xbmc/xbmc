/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_GUI_DIALOGS_NUMERIC_H
#define C_API_GUI_DIALOGS_NUMERIC_H

#include "../definitions.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef struct AddonToKodiFuncTable_kodi_gui_dialogNumeric
  {
    bool (*show_and_verify_new_password)(KODI_HANDLE kodiBase, char** password);
    int (*show_and_verify_password)(KODI_HANDLE kodiBase,
                                    const char* password,
                                    const char* heading,
                                    int retries);
    bool (*show_and_verify_input)(KODI_HANDLE kodiBase,
                                  const char* verify_in,
                                  char** verify_out,
                                  const char* heading,
                                  bool verify_input);
    bool (*show_and_get_time)(KODI_HANDLE kodiBase, struct tm* time, const char* heading);
    bool (*show_and_get_date)(KODI_HANDLE kodiBase, struct tm* date, const char* heading);
    bool (*show_and_get_ip_address)(KODI_HANDLE kodiBase,
                                    const char* ip_address_in,
                                    char** ip_address_out,
                                    const char* heading);
    bool (*show_and_get_number)(KODI_HANDLE kodiBase,
                                const char* input_in,
                                char** input_out,
                                const char* heading,
                                unsigned int auto_close_ms);
    bool (*show_and_get_seconds)(KODI_HANDLE kodiBase,
                                 const char* time_in,
                                 char** time_out,
                                 const char* heading);
  } AddonToKodiFuncTable_kodi_gui_dialogNumeric;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_GUI_DIALOGS_NUMERIC_H */

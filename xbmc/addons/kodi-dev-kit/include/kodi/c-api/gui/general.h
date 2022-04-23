/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_GUI_GENERAL_H
#define C_API_GUI_GENERAL_H

#include "definitions.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  struct KODI_GUI_RESOLUTION_INFO
  {
    int width;
    int height;
    float refresh_rate;
  };

  typedef struct AddonToKodiFuncTable_kodi_gui_general
  {
    void (*lock)();
    void (*unlock)();
    int (*get_screen_height)(KODI_HANDLE kodiBase);
    int (*get_screen_width)(KODI_HANDLE kodiBase);
    int (*get_video_resolution)(KODI_HANDLE kodiBase);
    int (*get_current_window_dialog_id)(KODI_HANDLE kodiBase);
    int (*get_current_window_id)(KODI_HANDLE kodiBase);
    ADDON_HARDWARE_CONTEXT (*get_hw_context)(KODI_HANDLE kodiBase);
    void (*get_whitelist_resolutions)(KODI_HANDLE kodiBase,
                                      KODI_GUI_RESOLUTION_INFO** list,
                                      size_t* size);
    void (*get_allowed_resolutions)(KODI_HANDLE kodiBase,
                                    KODI_GUI_RESOLUTION_INFO** list,
                                    size_t* size);
  } AddonToKodiFuncTable_kodi_gui_general;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_GUI_GENERAL_H */

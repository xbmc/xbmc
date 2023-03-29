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

  //==============================================================================
  /// @ingroup cpp_kodi_gui_general
  /// @brief **Adjust refresh rate enum**\n
  /// Used to get the Adjust refresh rate status info.
  ///
  typedef enum AdjustRefreshRateStatus
  {
    ADJUST_REFRESHRATE_STATUS_OFF = 0,
    ADJUST_REFRESHRATE_STATUS_ALWAYS,
    ADJUST_REFRESHRATE_STATUS_ON_STARTSTOP,
    ADJUST_REFRESHRATE_STATUS_ON_START,
  } AdjustRefreshRateStatus;
  //------------------------------------------------------------------------------

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
    AdjustRefreshRateStatus (*get_adjust_refresh_rate_status)(KODI_HANDLE kodiBase);
  } AddonToKodiFuncTable_kodi_gui_general;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_GUI_GENERAL_H */

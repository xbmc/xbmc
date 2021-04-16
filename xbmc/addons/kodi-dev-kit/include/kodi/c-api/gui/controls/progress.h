/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_GUI_CONTROLS_PROGRESS_H
#define C_API_GUI_CONTROLS_PROGRESS_H

#include "../definitions.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef struct AddonToKodiFuncTable_kodi_gui_control_progress
  {
    void (*set_visible)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool visible);
    void (*set_percentage)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, float percent);
    float (*get_percentage)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
  } AddonToKodiFuncTable_kodi_gui_control_progress;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_GUI_CONTROLS_PROGRESS_H */

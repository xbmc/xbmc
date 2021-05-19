/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_GUI_CONTROLS_IMAGE_H
#define C_API_GUI_CONTROLS_IMAGE_H

#include "../definitions.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef struct AddonToKodiFuncTable_kodi_gui_control_image
  {
    void (*set_visible)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool visible);
    void (*set_filename)(KODI_HANDLE kodiBase,
                         KODI_GUI_CONTROL_HANDLE handle,
                         const char* filename,
                         bool use_cache);
    void (*set_color_diffuse)(KODI_HANDLE kodiBase,
                              KODI_GUI_CONTROL_HANDLE handle,
                              uint32_t color_diffuse);
  } AddonToKodiFuncTable_kodi_gui_control_image;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_GUI_CONTROLS_IMAGE_H */

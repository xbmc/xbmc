/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_GUI_CONTROLS_RENDERING_H
#define C_API_GUI_CONTROLS_RENDERING_H

#include "../definitions.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef struct AddonToKodiFuncTable_kodi_gui_control_rendering
  {
    void (*set_callbacks)(
        KODI_HANDLE kodiBase,
        KODI_GUI_CONTROL_HANDLE handle,
        KODI_GUI_CLIENT_HANDLE clienthandle,
        bool (*createCB)(KODI_GUI_CLIENT_HANDLE, int, int, int, int, ADDON_HARDWARE_CONTEXT),
        void (*renderCB)(KODI_GUI_CLIENT_HANDLE),
        void (*stopCB)(KODI_GUI_CLIENT_HANDLE),
        bool (*dirtyCB)(KODI_GUI_CLIENT_HANDLE));
    void (*destroy)(void* kodiBase, KODI_GUI_CONTROL_HANDLE handle);
  } AddonToKodiFuncTable_kodi_gui_control_rendering;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_GUI_CONTROLS_RENDERING_H */

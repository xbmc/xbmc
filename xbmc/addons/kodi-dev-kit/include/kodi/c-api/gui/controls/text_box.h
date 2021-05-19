/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_GUI_CONTROLS_TEXT_BOX_H
#define C_API_GUI_CONTROLS_TEXT_BOX_H

#include "../definitions.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef struct AddonToKodiFuncTable_kodi_gui_control_text_box
  {
    void (*set_visible)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool visible);
    void (*reset)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_text)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, const char* text);
    char* (*get_text)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*scroll)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, unsigned int scroll);
    void (*set_auto_scrolling)(
        KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, int delay, int time, int repeat);
  } AddonToKodiFuncTable_kodi_gui_control_text_box;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_GUI_CONTROLS_TEXT_BOX_H */

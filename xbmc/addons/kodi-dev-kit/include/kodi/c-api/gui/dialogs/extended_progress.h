/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_GUI_DIALOGS_EXTENDED_PROGRESS_H
#define C_API_GUI_DIALOGS_EXTENDED_PROGRESS_H

#include "../definitions.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef struct AddonToKodiFuncTable_kodi_gui_dialogExtendedProgress
  {
    KODI_GUI_HANDLE (*new_dialog)(KODI_HANDLE kodiBase, const char* title);
    void (*delete_dialog)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle);
    char* (*get_title)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle);
    void (*set_title)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle, const char* title);
    char* (*get_text)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle);
    void (*set_text)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle, const char* text);
    bool (*is_finished)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle);
    void (*mark_finished)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle);
    float (*get_percentage)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle);
    void (*set_percentage)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle, float percentage);
    void (*set_progress)(KODI_HANDLE kodiBase,
                         KODI_GUI_HANDLE handle,
                         int currentItem,
                         int itemCount);
  } AddonToKodiFuncTable_kodi_gui_dialogExtendedProgress;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_GUI_DIALOGS_EXTENDED_PROGRESS_H */

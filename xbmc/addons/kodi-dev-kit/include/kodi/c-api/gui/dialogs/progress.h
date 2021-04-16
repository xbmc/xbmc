/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_GUI_DIALOGS_PROGRESS_H
#define C_API_GUI_DIALOGS_PROGRESS_H

#include "../definitions.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef struct AddonToKodiFuncTable_kodi_gui_dialogProgress
  {
    KODI_GUI_HANDLE (*new_dialog)(KODI_HANDLE kodiBase);
    void (*delete_dialog)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle);
    void (*open)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle);
    void (*set_heading)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle, const char* heading);
    void (*set_line)(KODI_HANDLE kodiBase,
                     KODI_GUI_HANDLE handle,
                     unsigned int lineNo,
                     const char* line);
    void (*set_can_cancel)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle, bool canCancel);
    bool (*is_canceled)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle);
    void (*set_percentage)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle, int percentage);
    int (*get_percentage)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle);
    void (*show_progress_bar)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle, bool pnOff);
    void (*set_progress_max)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle, int max);
    void (*set_progress_advance)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle, int nSteps);
    bool (*abort)(KODI_HANDLE kodiBase, KODI_GUI_HANDLE handle);
  } AddonToKodiFuncTable_kodi_gui_dialogProgress;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_GUI_DIALOGS_PROGRESS_H */

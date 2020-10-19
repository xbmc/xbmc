/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_GUI_DIALOGS_OK_H
#define C_API_GUI_DIALOGS_OK_H

#include "../definitions.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef struct AddonToKodiFuncTable_kodi_gui_dialogOK
  {
    void (*show_and_get_input_single_text)(KODI_HANDLE kodiBase,
                                           const char* heading,
                                           const char* text);
    void (*show_and_get_input_line_text)(KODI_HANDLE kodiBase,
                                         const char* heading,
                                         const char* line0,
                                         const char* line1,
                                         const char* line2);
  } AddonToKodiFuncTable_kodi_gui_dialogOK;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_GUI_DIALOGS_OK_H */

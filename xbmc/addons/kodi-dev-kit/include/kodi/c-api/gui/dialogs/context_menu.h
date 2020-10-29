/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_GUI_DIALOGS_CONTEXT_MENU_H
#define C_API_GUI_DIALOGS_CONTEXT_MENU_H

#include "../definitions.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef struct AddonToKodiFuncTable_kodi_gui_dialogContextMenu
  {
    int (*open)(KODI_HANDLE kodiBase,
                const char* heading,
                const char* entries[],
                unsigned int size);
  } AddonToKodiFuncTable_kodi_gui_dialogContextMenu;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_GUI_DIALOGS_CONTEXT_MENU_H */

/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_GUI_DIALOGS_TEXT_VIEWER_H
#define C_API_GUI_DIALOGS_TEXT_VIEWER_H

#include "../definitions.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef struct AddonToKodiFuncTable_kodi_gui_dialogTextViewer
  {
    void (*open)(KODI_HANDLE kodiBase, const char* heading, const char* text);
  } AddonToKodiFuncTable_kodi_gui_dialogTextViewer;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_GUI_DIALOGS_TEXT_VIEWER_H */

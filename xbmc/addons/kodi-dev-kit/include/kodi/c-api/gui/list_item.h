/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_GUI_LIST_ITEM_H
#define C_API_GUI_LIST_ITEM_H

#include "definitions.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef struct AddonToKodiFuncTable_kodi_gui_listItem
  {
    KODI_GUI_LISTITEM_HANDLE(*create)
    (KODI_HANDLE kodiBase, const char* label, const char* label2, const char* path);
    void (*destroy)(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle);

    char* (*get_label)(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle);
    void (*set_label)(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle, const char* label);
    char* (*get_label2)(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle);
    void (*set_label2)(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle, const char* label);
    char* (*get_art)(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle, const char* type);
    void (*set_art)(KODI_HANDLE kodiBase,
                    KODI_GUI_LISTITEM_HANDLE handle,
                    const char* type,
                    const char* image);
    char* (*get_path)(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle);
    void (*set_path)(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle, const char* path);
    char* (*get_property)(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle, const char* key);
    void (*set_property)(KODI_HANDLE kodiBase,
                         KODI_GUI_LISTITEM_HANDLE handle,
                         const char* key,
                         const char* value);
    void (*select)(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle, bool select);
    bool (*is_selected)(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle);
  } AddonToKodiFuncTable_kodi_gui_listItem;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_GUI_LIST_ITEM_H */

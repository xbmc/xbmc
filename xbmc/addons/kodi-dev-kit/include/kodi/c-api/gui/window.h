/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_GUI_WINDOW_H
#define C_API_GUI_WINDOW_H

#include "definitions.h"
#include "input/action_ids.h"

#include <stddef.h>

#define ADDON_MAX_CONTEXT_ENTRIES 20
#define ADDON_MAX_CONTEXT_ENTRY_NAME_LENGTH 80

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef struct gui_context_menu_pair
  {
    unsigned int id;
    char name[ADDON_MAX_CONTEXT_ENTRY_NAME_LENGTH];
  } gui_context_menu_pair;

  typedef struct AddonToKodiFuncTable_kodi_gui_window
  {
    /* Window creation functions */
    KODI_GUI_WINDOW_HANDLE(*create)
    (KODI_HANDLE kodiBase,
     const char* xml_filename,
     const char* default_skin,
     bool as_dialog,
     bool is_media);
    void (*destroy)(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle);

    void (*set_callbacks)(KODI_HANDLE kodiBase,
                          KODI_GUI_WINDOW_HANDLE handle,
                          KODI_GUI_CLIENT_HANDLE clienthandle,
                          bool (*CBInit)(KODI_GUI_CLIENT_HANDLE),
                          bool (*CBFocus)(KODI_GUI_CLIENT_HANDLE, int),
                          bool (*CBClick)(KODI_GUI_CLIENT_HANDLE, int),
                          bool (*CBOnAction)(KODI_GUI_CLIENT_HANDLE, enum ADDON_ACTION),
                          void (*CBGetContextButtons)(
                              KODI_GUI_CLIENT_HANDLE, int, gui_context_menu_pair*, unsigned int*),
                          bool (*CBOnContextButton)(KODI_GUI_CLIENT_HANDLE, int, unsigned int));
    bool (*show)(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle);
    bool (*close)(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle);
    bool (*do_modal)(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle);

    /* Window control functions */
    bool (*set_focus_id)(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    int (*get_focus_id)(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle);
    void (*set_control_label)(KODI_HANDLE kodiBase,
                              KODI_GUI_WINDOW_HANDLE handle,
                              int control_id,
                              const char* label);
    void (*set_control_visible)(KODI_HANDLE kodiBase,
                                KODI_GUI_WINDOW_HANDLE handle,
                                int control_id,
                                bool visible);
    void (*set_control_selected)(KODI_HANDLE kodiBase,
                                 KODI_GUI_WINDOW_HANDLE handle,
                                 int control_id,
                                 bool selected);

    /* Window property functions */
    void (*set_property)(KODI_HANDLE kodiBase,
                         KODI_GUI_WINDOW_HANDLE handle,
                         const char* key,
                         const char* value);
    void (*set_property_int)(KODI_HANDLE kodiBase,
                             KODI_GUI_WINDOW_HANDLE handle,
                             const char* key,
                             int value);
    void (*set_property_bool)(KODI_HANDLE kodiBase,
                              KODI_GUI_WINDOW_HANDLE handle,
                              const char* key,
                              bool value);
    void (*set_property_double)(KODI_HANDLE kodiBase,
                                KODI_GUI_WINDOW_HANDLE handle,
                                const char* key,
                                double value);
    char* (*get_property)(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, const char* key);
    int (*get_property_int)(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, const char* key);
    bool (*get_property_bool)(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, const char* key);
    double (*get_property_double)(KODI_HANDLE kodiBase,
                                  KODI_GUI_WINDOW_HANDLE handle,
                                  const char* key);
    void (*clear_properties)(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle);
    void (*clear_property)(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, const char* key);

    /* List item functions */
    void (*clear_item_list)(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle);
    void (*add_list_item)(KODI_HANDLE kodiBase,
                          KODI_GUI_WINDOW_HANDLE handle,
                          KODI_GUI_LISTITEM_HANDLE item,
                          int list_position);
    void (*remove_list_item_from_position)(KODI_HANDLE kodiBase,
                                           KODI_GUI_WINDOW_HANDLE handle,
                                           int list_position);
    void (*remove_list_item)(KODI_HANDLE kodiBase,
                             KODI_GUI_WINDOW_HANDLE handle,
                             KODI_GUI_LISTITEM_HANDLE item);
    KODI_GUI_LISTITEM_HANDLE(*get_list_item)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int list_position);
    void (*set_current_list_position)(KODI_HANDLE kodiBase,
                                      KODI_GUI_WINDOW_HANDLE handle,
                                      int list_position);
    int (*get_current_list_position)(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle);
    int (*get_list_size)(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle);
    void (*set_container_property)(KODI_HANDLE kodiBase,
                                   KODI_GUI_WINDOW_HANDLE handle,
                                   const char* key,
                                   const char* value);
    void (*set_container_content)(KODI_HANDLE kodiBase,
                                  KODI_GUI_WINDOW_HANDLE handle,
                                  const char* value);
    int (*get_current_container_id)(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle);

    /* Various functions */
    void (*mark_dirty_region)(KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle);

    /* GUI control access functions */
    KODI_GUI_CONTROL_HANDLE(*get_control_button)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_edit)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_fade_label)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_image)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_label)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_progress)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_radio_button)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_render_addon)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_settings_slider)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_slider)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_spin)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_text_box)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_dummy1)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_dummy2)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_dummy3)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_dummy4)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_dummy5)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_dummy6)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_dummy7)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_dummy8)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_dummy9)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    KODI_GUI_CONTROL_HANDLE(*get_control_dummy10)
    (KODI_HANDLE kodiBase, KODI_GUI_WINDOW_HANDLE handle, int control_id);
    /* This above used to add new get_control_* functions */
  } AddonToKodiFuncTable_kodi_gui_window;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_GUI_WINDOW_H */

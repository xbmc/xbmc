/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifndef C_API_GUI_DEFINITIONS_H
#define C_API_GUI_DEFINITIONS_H

#include "../addon_base.h"

#include <stddef.h>

#define ADDON_MAX_CONTEXT_ENTRIES 20
#define ADDON_MAX_CONTEXT_ENTRY_NAME_LENGTH 80

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef void* KODI_GUI_HANDLE;
  typedef void* KODI_GUI_CLIENT_HANDLE;
  typedef void* KODI_GUI_CONTROL_HANDLE;
  typedef void* KODI_GUI_LISTITEM_HANDLE;
  typedef void* KODI_GUI_WINDOW_HANDLE;

  typedef struct AddonToKodiFuncTable_kodi_gui_general
  {
    void (*lock)();
    void (*unlock)();
    int (*get_screen_height)(KODI_HANDLE kodiBase);
    int (*get_screen_width)(KODI_HANDLE kodiBase);
    int (*get_video_resolution)(KODI_HANDLE kodiBase);
    int (*get_current_window_dialog_id)(KODI_HANDLE kodiBase);
    int (*get_current_window_id)(KODI_HANDLE kodiBase);
    ADDON_HARDWARE_CONTEXT (*get_hw_context)(KODI_HANDLE kodiBase);
  } AddonToKodiFuncTable_kodi_gui_general;

  typedef struct AddonToKodiFuncTable_kodi_gui_control_button
  {
    void (*set_visible)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool visible);
    void (*set_enabled)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool enabled);
    void (*set_label)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, const char* label);
    char* (*get_label)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_label2)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, const char* label);
    char* (*get_label2)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
  } AddonToKodiFuncTable_kodi_gui_control_button;

  typedef struct AddonToKodiFuncTable_kodi_gui_control_edit
  {
    void (*set_visible)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool visible);
    void (*set_enabled)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool enabled);
    void (*set_label)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, const char* label);
    char* (*get_label)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_text)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, const char* text);
    char* (*get_text)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_cursor_position)(KODI_HANDLE kodiBase,
                                KODI_GUI_CONTROL_HANDLE handle,
                                unsigned int position);
    unsigned int (*get_cursor_position)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_input_type)(KODI_HANDLE kodiBase,
                           KODI_GUI_CONTROL_HANDLE handle,
                           int type,
                           const char* heading);
  } AddonToKodiFuncTable_kodi_gui_control_edit;

  typedef struct AddonToKodiFuncTable_kodi_gui_control_fade_label
  {
    void (*set_visible)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool visible);
    void (*add_label)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, const char* text);
    char* (*get_label)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_scrolling)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool scroll);
    void (*reset)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
  } AddonToKodiFuncTable_kodi_gui_control_fade_label;

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

  typedef struct AddonToKodiFuncTable_kodi_gui_control_label
  {
    void (*set_visible)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool visible);
    void (*set_label)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, const char* text);
    char* (*get_label)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
  } AddonToKodiFuncTable_kodi_gui_control_label;

  typedef struct AddonToKodiFuncTable_kodi_gui_control_progress
  {
    void (*set_visible)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool visible);
    void (*set_percentage)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, float percent);
    float (*get_percentage)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
  } AddonToKodiFuncTable_kodi_gui_control_progress;

  typedef struct AddonToKodiFuncTable_kodi_gui_control_radio_button
  {
    void (*set_visible)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool visible);
    void (*set_enabled)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool enabled);
    void (*set_label)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, const char* text);
    char* (*get_label)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_selected)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool selected);
    bool (*is_selected)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
  } AddonToKodiFuncTable_kodi_gui_control_radio_button;

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

  typedef struct AddonToKodiFuncTable_kodi_gui_control_settings_slider
  {
    void (*set_visible)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool visible);
    void (*set_enabled)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool enabled);
    void (*set_text)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, const char* label);
    void (*reset)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_int_range)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, int start, int end);
    void (*set_int_value)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, int value);
    int (*get_int_value)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_int_interval)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, int interval);
    void (*set_percentage)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, float percent);
    float (*get_percentage)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_float_range)(KODI_HANDLE kodiBase,
                            KODI_GUI_CONTROL_HANDLE handle,
                            float start,
                            float end);
    void (*set_float_value)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, float value);
    float (*get_float_value)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_float_interval)(KODI_HANDLE kodiBase,
                               KODI_GUI_CONTROL_HANDLE handle,
                               float interval);
  } AddonToKodiFuncTable_kodi_gui_control_settings_slider;

  typedef struct AddonToKodiFuncTable_kodi_gui_control_slider
  {
    void (*set_visible)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool visible);
    void (*set_enabled)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool enabled);
    void (*reset)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    char* (*get_description)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_int_range)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, int start, int end);
    void (*set_int_value)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, int value);
    int (*get_int_value)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_int_interval)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, int interval);
    void (*set_percentage)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, float percent);
    float (*get_percentage)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_float_range)(KODI_HANDLE kodiBase,
                            KODI_GUI_CONTROL_HANDLE handle,
                            float start,
                            float end);
    void (*set_float_value)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, float value);
    float (*get_float_value)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_float_interval)(KODI_HANDLE kodiBase,
                               KODI_GUI_CONTROL_HANDLE handle,
                               float interval);
  } AddonToKodiFuncTable_kodi_gui_control_slider;

  typedef struct AddonToKodiFuncTable_kodi_gui_control_spin
  {
    void (*set_visible)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool visible);
    void (*set_enabled)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, bool enabled);
    void (*set_text)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, const char* text);
    void (*reset)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_type)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, int type);
    void (*add_string_label)(KODI_HANDLE kodiBase,
                             KODI_GUI_CONTROL_HANDLE handle,
                             const char* label,
                             const char* value);
    void (*set_string_value)(KODI_HANDLE kodiBase,
                             KODI_GUI_CONTROL_HANDLE handle,
                             const char* value);
    char* (*get_string_value)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*add_int_label)(KODI_HANDLE kodiBase,
                          KODI_GUI_CONTROL_HANDLE handle,
                          const char* label,
                          int value);
    void (*set_int_range)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, int start, int end);
    void (*set_int_value)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, int value);
    int (*get_int_value)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_float_range)(KODI_HANDLE kodiBase,
                            KODI_GUI_CONTROL_HANDLE handle,
                            float start,
                            float end);
    void (*set_float_value)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle, float value);
    float (*get_float_value)(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle);
    void (*set_float_interval)(KODI_HANDLE kodiBase,
                               KODI_GUI_CONTROL_HANDLE handle,
                               float interval);
  } AddonToKodiFuncTable_kodi_gui_control_spin;

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

  typedef struct AddonToKodiFuncTable_kodi_gui_dialogContextMenu
  {
    int (*open)(KODI_HANDLE kodiBase,
                const char* heading,
                const char* entries[],
                unsigned int size);
  } AddonToKodiFuncTable_kodi_gui_dialogContextMenu;

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

  typedef struct AddonToKodiFuncTable_kodi_gui_dialogFileBrowser
  {
    bool (*show_and_get_directory)(KODI_HANDLE kodiBase,
                                   const char* shares,
                                   const char* heading,
                                   const char* path_in,
                                   char** path_out,
                                   bool writeOnly);
    bool (*show_and_get_file)(KODI_HANDLE kodiBase,
                              const char* shares,
                              const char* mask,
                              const char* heading,
                              const char* path_in,
                              char** path_out,
                              bool use_thumbs,
                              bool use_file_directories);
    bool (*show_and_get_file_from_dir)(KODI_HANDLE kodiBase,
                                       const char* directory,
                                       const char* mask,
                                       const char* heading,
                                       const char* path_in,
                                       char** path_out,
                                       bool use_thumbs,
                                       bool use_file_directories,
                                       bool singleList);
    bool (*show_and_get_file_list)(KODI_HANDLE kodiBase,
                                   const char* shares,
                                   const char* mask,
                                   const char* heading,
                                   char*** file_list,
                                   unsigned int* entries,
                                   bool use_thumbs,
                                   bool use_file_directories);
    bool (*show_and_get_source)(KODI_HANDLE kodiBase,
                                const char* path_in,
                                char** path_out,
                                bool allow_network_shares,
                                const char* additional_share,
                                const char* type);
    bool (*show_and_get_image)(KODI_HANDLE kodiBase,
                               const char* shares,
                               const char* heading,
                               const char* path_in,
                               char** path_out);
    bool (*show_and_get_image_list)(KODI_HANDLE kodiBase,
                                    const char* shares,
                                    const char* heading,
                                    char*** file_list,
                                    unsigned int* entries);
    void (*clear_file_list)(KODI_HANDLE kodiBase, char*** file_list, unsigned int entries);
  } AddonToKodiFuncTable_kodi_gui_dialogFileBrowser;

  // typedef void (*char_callback_t) (CGUIKeyboard *ref, const std::string &typedString);

  typedef struct AddonToKodiFuncTable_kodi_gui_dialogKeyboard
  {
    bool (*show_and_get_input_with_head)(KODI_HANDLE kodiBase,
                                         const char* text_in,
                                         char** text_out,
                                         const char* heading,
                                         bool allow_empty_result,
                                         bool hiddenInput,
                                         unsigned int auto_close_ms);
    bool (*show_and_get_input)(KODI_HANDLE kodiBase,
                               const char* text_in,
                               char** text_out,
                               bool allow_empty_result,
                               unsigned int auto_close_ms);
    bool (*show_and_get_new_password_with_head)(KODI_HANDLE kodiBase,
                                                const char* password_in,
                                                char** password_out,
                                                const char* heading,
                                                bool allow_empty_result,
                                                unsigned int auto_close_ms);
    bool (*show_and_get_new_password)(KODI_HANDLE kodiBase,
                                      const char* password_in,
                                      char** password_out,
                                      unsigned int auto_close_ms);
    bool (*show_and_verify_new_password_with_head)(KODI_HANDLE kodiBase,
                                                   char** password_out,
                                                   const char* heading,
                                                   bool allow_empty_result,
                                                   unsigned int auto_close_ms);
    bool (*show_and_verify_new_password)(KODI_HANDLE kodiBase,
                                         char** password_out,
                                         unsigned int auto_close_ms);
    int (*show_and_verify_password)(KODI_HANDLE kodiBase,
                                    const char* password_in,
                                    char** password_out,
                                    const char* heading,
                                    int retries,
                                    unsigned int auto_close_ms);
    bool (*show_and_get_filter)(KODI_HANDLE kodiBase,
                                const char* text_in,
                                char** text_out,
                                bool searching,
                                unsigned int auto_close_ms);
    bool (*send_text_to_active_keyboard)(KODI_HANDLE kodiBase,
                                         const char* text,
                                         bool close_keyboard);
    bool (*is_keyboard_activated)(KODI_HANDLE kodiBase);
  } AddonToKodiFuncTable_kodi_gui_dialogKeyboard;

  typedef struct AddonToKodiFuncTable_kodi_gui_dialogNumeric
  {
    bool (*show_and_verify_new_password)(KODI_HANDLE kodiBase, char** password);
    int (*show_and_verify_password)(KODI_HANDLE kodiBase,
                                    const char* password,
                                    const char* heading,
                                    int retries);
    bool (*show_and_verify_input)(KODI_HANDLE kodiBase,
                                  const char* verify_in,
                                  char** verify_out,
                                  const char* heading,
                                  bool verify_input);
    bool (*show_and_get_time)(KODI_HANDLE kodiBase, struct tm* time, const char* heading);
    bool (*show_and_get_date)(KODI_HANDLE kodiBase, struct tm* date, const char* heading);
    bool (*show_and_get_ip_address)(KODI_HANDLE kodiBase,
                                    const char* ip_address_in,
                                    char** ip_address_out,
                                    const char* heading);
    bool (*show_and_get_number)(KODI_HANDLE kodiBase,
                                const char* input_in,
                                char** input_out,
                                const char* heading,
                                unsigned int auto_close_ms);
    bool (*show_and_get_seconds)(KODI_HANDLE kodiBase,
                                 const char* time_in,
                                 char** time_out,
                                 const char* heading);
  } AddonToKodiFuncTable_kodi_gui_dialogNumeric;

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

  typedef struct AddonToKodiFuncTable_kodi_gui_dialogSelect
  {
    int (*open)(KODI_HANDLE kodiBase,
                const char* heading,
                const char* entries[],
                unsigned int size,
                int selected,
                unsigned int autoclose);
    bool (*open_multi_select)(KODI_HANDLE kodiBase,
                              const char* heading,
                              const char* entryIDs[],
                              const char* entryNames[],
                              bool entriesSelected[],
                              unsigned int size,
                              unsigned int autoclose);
  } AddonToKodiFuncTable_kodi_gui_dialogSelect;

  typedef struct AddonToKodiFuncTable_kodi_gui_dialogTextViewer
  {
    void (*open)(KODI_HANDLE kodiBase, const char* heading, const char* text);
  } AddonToKodiFuncTable_kodi_gui_dialogTextViewer;

  typedef struct AddonToKodiFuncTable_kodi_gui_dialogYesNo
  {
    bool (*show_and_get_input_single_text)(KODI_HANDLE kodiBase,
                                           const char* heading,
                                           const char* text,
                                           bool* canceled,
                                           const char* noLabel,
                                           const char* yesLabel);
    bool (*show_and_get_input_line_text)(KODI_HANDLE kodiBase,
                                         const char* heading,
                                         const char* line0,
                                         const char* line1,
                                         const char* line2,
                                         const char* noLabel,
                                         const char* yesLabel);
    bool (*show_and_get_input_line_button_text)(KODI_HANDLE kodiBase,
                                                const char* heading,
                                                const char* line0,
                                                const char* line1,
                                                const char* line2,
                                                bool* canceled,
                                                const char* noLabel,
                                                const char* yesLabel);
  } AddonToKodiFuncTable_kodi_gui_dialogYesNo;

  typedef struct AddonToKodiFuncTable_kodi_gui_listItem
  {
    KODI_GUI_LISTITEM_HANDLE(*create)
    (KODI_HANDLE kodiBase,
     const char* label,
     const char* label2,
     const char* icon_image,
     const char* path);
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
                          bool (*CBOnAction)(KODI_GUI_CLIENT_HANDLE, int, uint32_t, wchar_t),
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

  typedef struct AddonToKodiFuncTable_kodi_gui
  {
    struct AddonToKodiFuncTable_kodi_gui_general* general;
    struct AddonToKodiFuncTable_kodi_gui_control_button* control_button;
    struct AddonToKodiFuncTable_kodi_gui_control_edit* control_edit;
    struct AddonToKodiFuncTable_kodi_gui_control_fade_label* control_fade_label;
    struct AddonToKodiFuncTable_kodi_gui_control_label* control_label;
    struct AddonToKodiFuncTable_kodi_gui_control_image* control_image;
    struct AddonToKodiFuncTable_kodi_gui_control_progress* control_progress;
    struct AddonToKodiFuncTable_kodi_gui_control_radio_button* control_radio_button;
    struct AddonToKodiFuncTable_kodi_gui_control_rendering* control_rendering;
    struct AddonToKodiFuncTable_kodi_gui_control_settings_slider* control_settings_slider;
    struct AddonToKodiFuncTable_kodi_gui_control_slider* control_slider;
    struct AddonToKodiFuncTable_kodi_gui_control_spin* control_spin;
    struct AddonToKodiFuncTable_kodi_gui_control_text_box* control_text_box;
    KODI_HANDLE control_dummy1;
    KODI_HANDLE control_dummy2;
    KODI_HANDLE control_dummy3;
    KODI_HANDLE control_dummy4;
    KODI_HANDLE control_dummy5;
    KODI_HANDLE control_dummy6;
    KODI_HANDLE control_dummy7;
    KODI_HANDLE control_dummy8;
    KODI_HANDLE control_dummy9;
    KODI_HANDLE control_dummy10; /* This and above used to add new controls */
    struct AddonToKodiFuncTable_kodi_gui_dialogContextMenu* dialogContextMenu;
    struct AddonToKodiFuncTable_kodi_gui_dialogExtendedProgress* dialogExtendedProgress;
    struct AddonToKodiFuncTable_kodi_gui_dialogFileBrowser* dialogFileBrowser;
    struct AddonToKodiFuncTable_kodi_gui_dialogKeyboard* dialogKeyboard;
    struct AddonToKodiFuncTable_kodi_gui_dialogNumeric* dialogNumeric;
    struct AddonToKodiFuncTable_kodi_gui_dialogOK* dialogOK;
    struct AddonToKodiFuncTable_kodi_gui_dialogProgress* dialogProgress;
    struct AddonToKodiFuncTable_kodi_gui_dialogSelect* dialogSelect;
    struct AddonToKodiFuncTable_kodi_gui_dialogTextViewer* dialogTextViewer;
    struct AddonToKodiFuncTable_kodi_gui_dialogYesNo* dialogYesNo;
    KODI_HANDLE dialog_dummy1;
    KODI_HANDLE dialog_dummy2;
    KODI_HANDLE dialog_dummy3;
    KODI_HANDLE dialog_dummy4;
    KODI_HANDLE dialog_dummy5;
    KODI_HANDLE dialog_dummy6;
    KODI_HANDLE dialog_dummy7;
    KODI_HANDLE dialog_dummy8;
    KODI_HANDLE dialog_dummy9;
    KODI_HANDLE dialog_dummy10; /* This and above used to add new dialogs */
    struct AddonToKodiFuncTable_kodi_gui_listItem* listItem;
    struct AddonToKodiFuncTable_kodi_gui_window* window;
  } AddonToKodiFuncTable_kodi_gui;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_GUI_DEFINITIONS_H */

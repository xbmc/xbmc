/*
 *      Copyright (C) 2005-2017 Team KODI
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <string>
#include <time.h>

/*
 * Internal Structures to have "C"-Style data transfer
 */
extern "C"
{

typedef struct AddonToKodiFuncTable_kodi_gui_general
{
  void (*lock)();
  void (*unlock)();
  int (*get_screen_height)(void* kodiBase);
  int (*get_screen_width)(void* kodiBase);
  int (*get_video_resolution)(void* kodiBase);
  int (*get_current_window_dialog_id)(void* kodiBase);
  int (*get_current_window_id)(void* kodiBase);
} AddonToKodiFuncTable_kodi_gui_general;

typedef struct AddonToKodiFuncTable_kodi_gui_control_button
{
  void (*set_visible)(void* kodiBase, void* handle, bool visible);
  void (*set_enabled)(void* kodiBase, void* handle, bool enabled);
  void (*set_label)(void* kodiBase, void* handle, const char* label);
  char* (*get_label)(void* kodiBase, void* handle);
  void (*set_label2)(void* kodiBase, void* handle, const char *label);
  char* (*get_label2)(void* kodiBase, void* handle);
} AddonToKodiFuncTable_kodi_gui_control_button;

typedef struct AddonToKodiFuncTable_kodi_gui_control_edit
{
  void (*set_visible)(void* kodiBase, void* handle, bool visible);
  void (*set_enabled)(void* kodiBase, void* handle, bool enabled);
  void (*set_label)(void* kodiBase, void* handle, const char* label);
  char* (*get_label)(void* kodiBase, void* handle);
  void (*set_text)(void* kodiBase, void* handle, const char* text);
  char* (*get_text)(void* kodiBase, void* handle);
  void (*set_cursor_position)(void* kodiBase, void* handle, unsigned int position);
  unsigned int (*get_cursor_position)(void* kodiBase, void* handle);
  void (*set_input_type)(void* kodiBase, void* handle, int type, const char* heading);
} AddonToKodiFuncTable_kodi_gui_control_edit;

typedef struct AddonToKodiFuncTable_kodi_gui_control_fade_label
{
  void (*set_visible)(void* kodiBase, void* handle, bool visible);
  void (*add_label)(void* kodiBase, void* handle, const char* text);
  char* (*get_label)(void* kodiBase, void* handle);
  void (*set_scrolling)(void* kodiBase, void* handle, bool scroll);
  void (*reset)(void* kodiBase, void* handle);
} AddonToKodiFuncTable_kodi_gui_control_fade_label;

typedef struct AddonToKodiFuncTable_kodi_gui_control_image
{
  void (*set_visible)(void* kodiBase, void* handle, bool visible);
  void (*set_filename)(void* kodiBase, void* handle, const char* filename, bool use_cache);
  void (*set_color_diffuse)(void* kodiBase, void* handle, uint32_t color_diffuse);
} AddonToKodiFuncTable_kodi_gui_control_image;

typedef struct AddonToKodiFuncTable_kodi_gui_control_label
{
  void (*set_visible)(void* kodiBase, void* handle, bool visible);
  void (*set_label)(void* kodiBase, void* handle, const char* text);
  char* (*get_label)(void* kodiBase, void* handle);
} AddonToKodiFuncTable_kodi_gui_control_label;

typedef struct AddonToKodiFuncTable_kodi_gui_control_progress
{
  void (*set_visible)(void* kodiBase, void* handle, bool visible);
  void (*set_percentage)(void* kodiBase, void* handle, float percent);
  float (*get_percentage)(void* kodiBase, void* handle);
} AddonToKodiFuncTable_kodi_gui_control_progress;

typedef struct AddonToKodiFuncTable_kodi_gui_control_radio_button
{
  void (*set_visible)(void* kodiBase, void* handle, bool visible);
  void (*set_enabled)(void* kodiBase, void* handle, bool enabled);
  void (*set_label)(void* kodiBase, void* handle, const char* text);
  char* (*get_label)(void* kodiBase, void* handle);
  void (*set_selected)(void* kodiBase, void* handle, bool selected);
  bool (*is_selected)(void* kodiBase, void* handle);
} AddonToKodiFuncTable_kodi_gui_control_radio_button;

typedef struct AddonToKodiFuncTable_kodi_gui_control_rendering
{
  void (*set_callbacks)(void* kodiBase, void* handle, void* clienthandle,
      bool    (*createCB)(void*,int,int,int,int,void*),
      void    (*renderCB)(void*),
      void    (*stopCB)(void*),
      bool    (*dirtyCB)(void*));
  void (*destroy)(void *kodiBase, void* handle);
} AddonToKodiFuncTable_kodi_gui_control_rendering;

typedef struct AddonToKodiFuncTable_kodi_gui_control_settings_slider
{
  void (*set_visible)(void* kodiBase, void* handle, bool visible);
  void (*set_enabled)(void* kodiBase, void* handle, bool enabled);
  void (*set_text)(void* kodiBase, void* handle, const char* label);
  void (*reset)(void* kodiBase, void* handle);
  void (*set_int_range)(void* kodiBase, void* handle, int start, int end);
  void (*set_int_value)(void* kodiBase, void* handle, int value);
  int (*get_int_value)(void* kodiBase, void* handle);
  void (*set_int_interval)(void* kodiBase, void* handle, int interval);
  void (*set_percentage)(void* kodiBase, void* handle, float percent);
  float (*get_percentage)(void* kodiBase, void* handle);
  void (*set_float_range)(void* kodiBase, void* handle, float start, float end);
  void (*set_float_value)(void* kodiBase, void* handle, float value);
  float (*get_float_value)(void* kodiBase, void* handle);
  void (*set_float_interval)(void* kodiBase, void* handle, float interval);
} AddonToKodiFuncTable_kodi_gui_control_settings_slider;

typedef struct AddonToKodiFuncTable_kodi_gui_control_slider
{
  void (*set_visible)(void* kodiBase, void* handle, bool visible);
  void (*set_enabled)(void* kodiBase, void* handle, bool enabled);
  void (*reset)(void* kodiBase, void* handle);
  char* (*get_description)(void* kodiBase, void* handle);
  void (*set_int_range)(void* kodiBase, void* handle, int start, int end);
  void (*set_int_value)(void* kodiBase, void* handle, int value);
  int (*get_int_value)(void* kodiBase, void* handle);
  void (*set_int_interval)(void* kodiBase, void* handle, int interval);
  void (*set_percentage)(void* kodiBase, void* handle, float percent);
  float (*get_percentage)(void* kodiBase, void* handle);
  void (*set_float_range)(void* kodiBase, void* handle, float start, float end);
  void (*set_float_value)(void* kodiBase, void* handle, float value);
  float (*get_float_value)(void* kodiBase, void* handle);
  void (*set_float_interval)(void* kodiBase, void* handle, float interval);
} AddonToKodiFuncTable_kodi_gui_control_slider;

typedef struct AddonToKodiFuncTable_kodi_gui_control_spin
{
  void (*set_visible)(void* kodiBase, void* handle, bool visible);
  void (*set_enabled)(void* kodiBase, void* handle, bool enabled);
  void (*set_text)(void* kodiBase, void* handle, const char* text);
  void (*reset)(void* kodiBase, void* handle);
  void (*set_type)(void* kodiBase, void* handle, int type);
  void (*add_string_label)(void* kodiBase, void* handle, const char* label, const char* value);
  void (*set_string_value)(void* kodiBase, void* handle, const char* value);
  char* (*get_string_value)(void* kodiBase, void* handle);
  void (*add_int_label)(void* kodiBase, void* handle, const char* label, int value);
  void (*set_int_range)(void* kodiBase, void* handle, int start, int end);
  void (*set_int_value)(void* kodiBase, void* handle, int value);
  int (*get_int_value)(void* kodiBase, void* handle);
  void (*set_float_range)(void* kodiBase, void* handle, float start, float end);
  void (*set_float_value)(void* kodiBase, void* handle, float value);
  float (*get_float_value)(void* kodiBase, void* handle);
  void (*set_float_interval)(void* kodiBase, void* handle, float interval);
} AddonToKodiFuncTable_kodi_gui_control_spin;

typedef struct AddonToKodiFuncTable_kodi_gui_control_text_box
{
  void (*set_visible)(void* kodiBase, void* handle, bool visible);
  void (*reset)(void* kodiBase, void* handle);
  void (*set_text)(void* kodiBase, void* handle, const char* text);
  char* (*get_text)(void* kodiBase, void* handle);
  void (*scroll)(void* kodiBase, void* handle, unsigned int scroll);
  void (*set_auto_scrolling)(void* kodiBase, void* handle, int delay, int time, int repeat);
} AddonToKodiFuncTable_kodi_gui_control_text_box;

typedef struct AddonToKodiFuncTable_kodi_gui_dialogContextMenu
{
  int (*open)(void* kodiBase, const char *heading, const char *entries[], unsigned int size);
} AddonToKodiFuncTable_kodi_gui_dialogContextMenu;

typedef struct AddonToKodiFuncTable_kodi_gui_dialogExtendedProgress
{
  void* (*new_dialog)(void* kodiBase, const char *title);
  void (*delete_dialog)(void* kodiBase, void* handle);
  char* (*get_title)(void* kodiBase, void* handle);
  void (*set_title)(void* kodiBase, void* handle, const char *title);
  char* (*get_text)(void* kodiBase, void* handle);
  void (*set_text)(void* kodiBase, void* handle, const char *text);
  bool (*is_finished)(void* kodiBase, void* handle);
  void (*mark_finished)(void* kodiBase, void* handle);
  float (*get_percentage)(void* kodiBase, void* handle);
  void (*set_percentage)(void* kodiBase, void* handle, float percentage);
  void (*set_progress)(void* kodiBase, void* handle, int currentItem, int itemCount);
} AddonToKodiFuncTable_kodi_gui_dialogExtendedProgress;

typedef struct AddonToKodiFuncTable_kodi_gui_dialogFileBrowser
{
  bool (*show_and_get_directory)(void* kodiBase, const char* shares, const char* heading, const char* path_in, char** path_out, bool writeOnly);
  bool (*show_and_get_file)(void* kodiBase, const char* shares, const char* mask, const char* heading, const char* path_in, char** path_out, bool use_thumbs, bool use_file_directories);
  bool (*show_and_get_file_from_dir)(void* kodiBase, const char* directory, const char* mask, const char* heading, const char* path_in, char** path_out, bool use_thumbs, bool use_file_directories, bool singleList);
  bool (*show_and_get_file_list)(void* kodiBase, const char* shares, const char* mask, const char* heading, char*** file_list, unsigned int* entries, bool use_thumbs, bool use_file_directories);
  bool (*show_and_get_source)(void* kodiBase, const char* path_in, char** path_out, bool allow_network_shares, const char* additional_share, const char* type);
  bool (*show_and_get_image)(void* kodiBase, const char* shares, const char* heading, const char* path_in, char** path_out);
  bool (*show_and_get_image_list)(void* kodiBase,  const char* shares, const char* heading, char*** file_list, unsigned int* entries);
  void (*clear_file_list)(void* kodiBase, char*** file_list, unsigned int entries);
} AddonToKodiFuncTable_kodi_gui_dialogFileBrowser;

typedef struct AddonToKodiFuncTable_kodi_gui_dialogKeyboard
{
  bool (*show_and_get_input_with_head)(void* kodiBase, const char* text_in, char** text_out, const char* heading, bool allow_empty_result, bool hiddenInput, unsigned int auto_close_ms);
  bool (*show_and_get_input)(void* kodiBase, const char* text_in, char** text_out, bool allow_empty_result, unsigned int auto_close_ms);
  bool (*show_and_get_new_password_with_head)(void* kodiBase, const char* password_in, char** password_out, const char* heading, bool allow_empty_result, unsigned int auto_close_ms);
  bool (*show_and_get_new_password)(void* kodiBase, const char* password_in, char** password_out, unsigned int auto_close_ms);
  bool (*show_and_verify_new_password_with_head)(void* kodiBase, char** password_out, const char* heading, bool allow_empty_result, unsigned int auto_close_ms);
  bool (*show_and_verify_new_password)(void* kodiBase, char** password_out, unsigned int auto_close_ms);
  int (*show_and_verify_password)(void* kodiBase, const char* password_in, char** password_out, const char* heading, int retries, unsigned int auto_close_ms);
  bool (*show_and_get_filter)(void* kodiBase, const char* text_in, char** text_out, bool searching, unsigned int auto_close_ms);
  bool (*send_text_to_active_keyboard)(void* kodiBase, const char* text, bool close_keyboard);
  bool (*is_keyboard_activated)(void* kodiBase);
} AddonToKodiFuncTable_kodi_gui_dialogKeyboard;

typedef struct AddonToKodiFuncTable_kodi_gui_dialogNumeric
{
  bool (*show_and_verify_new_password)(void* kodiBase, char** password);
  int (*show_and_verify_password)(void* kodiBase, const char* password, const char *heading, int retries);
  bool (*show_and_verify_input)(void* kodiBase, const char* verify_in, char** verify_out, const char* heading, bool verify_input);
  bool (*show_and_get_time)(void* kodiBase, tm *time, const char *heading);
  bool (*show_and_get_date)(void* kodiBase, tm *date, const char *heading);
  bool (*show_and_get_ip_address)(void* kodiBase, const char* ip_address_in, char** ip_address_out, const char *heading);
  bool (*show_and_get_number)(void* kodiBase, const char* input_in, char** input_out, const char *heading, unsigned int auto_close_ms);
  bool (*show_and_get_seconds)(void* kodiBase, const char* time_in, char** time_out, const char *heading);
} AddonToKodiFuncTable_kodi_gui_dialogNumeric;

typedef struct AddonToKodiFuncTable_kodi_gui_dialogOK
{
  void (*show_and_get_input_single_text)(void* kodiBase, const char *heading, const char *text);
  void (*show_and_get_input_line_text)(void* kodiBase, const char *heading, const char *line0, const char *line1, const char *line2);
} AddonToKodiFuncTable_kodi_gui_dialogOK;

typedef struct AddonToKodiFuncTable_kodi_gui_dialogProgress
{
  void* (*new_dialog)(void* kodiBase);
  void (*delete_dialog)(void* kodiBase, void* handle);
  void (*open)(void* kodiBase, void* handle);
  void (*set_heading)(void* kodiBase, void* handle, const char* heading);
  void (*set_line)(void* kodiBase, void* handle, unsigned int lineNo, const char* line);
  void (*set_can_cancel)(void* kodiBase, void* handle, bool canCancel);
  bool (*is_canceled)(void* kodiBase, void* handle);
  void (*set_percentage)(void* kodiBase, void* handle, int percentage);
  int (*get_percentage)(void* kodiBase, void* handle);
  void (*show_progress_bar)(void* kodiBase, void* handle, bool pnOff);
  void (*set_progress_max)(void* kodiBase, void* handle, int max);
  void (*set_progress_advance)(void* kodiBase, void* handle, int nSteps);
  bool (*abort)(void* kodiBase, void* handle);
} AddonToKodiFuncTable_kodi_gui_dialogProgress;

typedef struct AddonToKodiFuncTable_kodi_gui_dialogSelect
{
  int (*open)(void* kodiBase, const char *heading, const char *entries[], unsigned int size, int selected, unsigned int autoclose);
  bool (*open_multi_select)(void* kodiBase, const char* heading, const char* entryIDs[], const char* entryNames[],
                            bool entriesSelected[], unsigned int size, unsigned int autoclose);
} AddonToKodiFuncTable_kodi_gui_dialogSelect;

typedef struct AddonToKodiFuncTable_kodi_gui_dialogTextViewer
{
  void (*open)(void* kodiBase, const char *heading, const char *text);
} AddonToKodiFuncTable_kodi_gui_dialogTextViewer;

typedef struct AddonToKodiFuncTable_kodi_gui_dialogYesNo
{
  bool (*show_and_get_input_single_text)(void* kodiBase, const char *heading, const char *text, bool *canceled, const char *noLabel, const char *yesLabel);
  bool (*show_and_get_input_line_text)(void* kodiBase, const char *heading, const char *line0, const char *line1, const char *line2, const char *noLabel, const char *yesLabel);
  bool (*show_and_get_input_line_button_text)(void* kodiBase, const char *heading, const char *line0, const char *line1, const char *line2, bool *canceled, const char *noLabel, const char *yesLabel);
} AddonToKodiFuncTable_kodi_gui_dialogYesNo;

typedef struct AddonToKodiFuncTable_kodi_gui_listItem
{
  void* (*create)(void* kodiBase, const char* label, const char* label2, const char* icon_image, const char* path);
  void (*destroy)(void* kodiBase, void* handle);
  char* (*get_label)(void* kodiBase, void* handle);
  void (*set_label)(void* kodiBase, void* handle, const char* label);
  char* (*get_label2)(void* kodiBase, void* handle);
  void (*set_label2)(void* kodiBase, void* handle, const char* label);
  char* (*get_icon_image)(void* kodiBase, void* handle);
  void (*set_icon_image)(void* kodiBase, void* handle, const char* image);
  char* (*get_art)(void* kodiBase, void* handle, const char* type);
  void (*set_art)(void* kodiBase, void* handle, const char* type, const char* image);
  char* (*get_path)(void* kodiBase, void* handle);
  void (*set_path)(void* kodiBase, void* handle, const char* path);
  char* (*get_property)(void* kodiBase, void* handle, const char* key);
  void (*set_property)(void* kodiBase, void* handle, const char* key, const char* value);
  void (*select)(void* kodiBase, void* handle, bool select);
  bool (*is_selected)(void* kodiBase, void* handle);
} AddonToKodiFuncTable_kodi_gui_listItem;

#define ADDON_MAX_CONTEXT_ENTRIES 20
#define ADDON_MAX_CONTEXT_ENTRY_NAME_LENGTH 80
typedef struct gui_context_menu_pair
{
  unsigned int id;
  char name[ADDON_MAX_CONTEXT_ENTRY_NAME_LENGTH];
} gui_context_menu_pair;

typedef struct AddonToKodiFuncTable_kodi_gui_window
{
  /* Window creation functions */
  void* (*create)(void* kodiBase, const char* xml_filename, const char* default_skin, bool as_dialog, bool is_media);
  void (*destroy)(void* kodiBase, void* handle);
  void (*set_callbacks)(void* kodiBase, void* handle, void* clienthandle,
       bool (*)(void* handle),
       bool (*)(void* handle, int),
       bool (*)(void* handle, int),
       bool (*)(void* handle, int),
       void (*)(void* handle, int, gui_context_menu_pair*, unsigned int*),
       bool (*)(void* handle, int, unsigned int));
  bool (*show)(void* kodiBase, void* handle);
  bool (*close)(void* kodiBase, void* handle);
  bool (*do_modal)(void* kodiBase, void* handle);

  /* Window control functions */
  bool (*set_focus_id)(void* kodiBase, void* handle, int control_id);
  int (*get_focus_id)(void* kodiBase, void* handle);
  void (*set_control_label)(void* kodiBase, void* handle, int control_id, const char* label);

  /* Window property functions */
  void (*set_property)(void* kodiBase, void* handle, const char* key, const char* value);
  void (*set_property_int)(void* kodiBase, void* handle, const char* key, int value);
  void (*set_property_bool)(void* kodiBase, void* handle, const char* key, bool value);
  void (*set_property_double)(void* kodiBase, void* handle, const char* key, double value);
  char* (*get_property)(void* kodiBase, void* handle, const char* key);
  int (*get_property_int)(void* kodiBase, void* handle, const char* key);
  bool (*get_property_bool)(void* kodiBase, void* handle, const char* key);
  double (*get_property_double)(void* kodiBase, void* handle, const char* key);
  void (*clear_properties)(void* kodiBase, void* handle);
  void (*clear_property)(void* kodiBase, void* handle, const char* key);

  /* List item functions */
  void (*clear_item_list)(void* kodiBase, void* handle);
  void (*add_list_item)(void* kodiBase, void* handle, void* item, int list_position);
  void (*remove_list_item_from_position)(void* kodiBase, void* handle, int list_position);
  void (*remove_list_item)(void* kodiBase, void* handle, void* item);
  void* (*get_list_item)(void* kodiBase, void* handle, int list_position);
  void (*set_current_list_position)(void* kodiBase, void* handle, int list_position);
  int (*get_current_list_position)(void* kodiBase, void* handle);
  int (*get_list_size)(void* kodiBase, void* handle);
  void (*set_container_property)(void* kodiBase, void* handle, const char* key, const char* value);
  void (*set_container_content)(void* kodiBase, void* handle, const char* value);
  int (*get_current_container_id)(void* kodiBase, void* handle);

  /* Various functions */
  void (*mark_dirty_region)(void* kodiBase, void* handle);

  /* GUI control access functions */
  void* (*get_control_button)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_edit)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_fade_label)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_image)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_label)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_progress)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_radio_button)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_render_addon)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_settings_slider)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_slider)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_spin)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_text_box)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_dummy1)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_dummy2)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_dummy3)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_dummy4)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_dummy5)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_dummy6)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_dummy7)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_dummy8)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_dummy9)(void* kodiBase, void* handle, int control_id);
  void* (*get_control_dummy10)(void* kodiBase, void* handle, int control_id); /* This and above used to add new get_control_* functions */
} AddonToKodiFuncTable_kodi_gui_window;

typedef struct AddonToKodiFuncTable_kodi_gui
{
  AddonToKodiFuncTable_kodi_gui_general* general;
  AddonToKodiFuncTable_kodi_gui_control_button* control_button;
  AddonToKodiFuncTable_kodi_gui_control_edit* control_edit;
  AddonToKodiFuncTable_kodi_gui_control_fade_label* control_fade_label;
  AddonToKodiFuncTable_kodi_gui_control_label* control_label;
  AddonToKodiFuncTable_kodi_gui_control_image* control_image;
  AddonToKodiFuncTable_kodi_gui_control_progress* control_progress;
  AddonToKodiFuncTable_kodi_gui_control_radio_button* control_radio_button;
  AddonToKodiFuncTable_kodi_gui_control_rendering* control_rendering;
  AddonToKodiFuncTable_kodi_gui_control_settings_slider* control_settings_slider;
  AddonToKodiFuncTable_kodi_gui_control_slider* control_slider;
  AddonToKodiFuncTable_kodi_gui_control_spin* control_spin;
  AddonToKodiFuncTable_kodi_gui_control_text_box* control_text_box;
  void* control_dummy1;
  void* control_dummy2;
  void* control_dummy3;
  void* control_dummy4;
  void* control_dummy5;
  void* control_dummy6;
  void* control_dummy7;
  void* control_dummy8;
  void* control_dummy9;
  void* control_dummy10; /* This and above used to add new controls */
  AddonToKodiFuncTable_kodi_gui_dialogContextMenu* dialogContextMenu;
  AddonToKodiFuncTable_kodi_gui_dialogExtendedProgress* dialogExtendedProgress;
  AddonToKodiFuncTable_kodi_gui_dialogFileBrowser* dialogFileBrowser;
  AddonToKodiFuncTable_kodi_gui_dialogKeyboard* dialogKeyboard;
  AddonToKodiFuncTable_kodi_gui_dialogNumeric* dialogNumeric;
  AddonToKodiFuncTable_kodi_gui_dialogOK* dialogOK;
  AddonToKodiFuncTable_kodi_gui_dialogProgress* dialogProgress;
  AddonToKodiFuncTable_kodi_gui_dialogSelect* dialogSelect;
  AddonToKodiFuncTable_kodi_gui_dialogTextViewer* dialogTextViewer;
  AddonToKodiFuncTable_kodi_gui_dialogYesNo* dialogYesNo;
  void* dialog_dummy1;
  void* dialog_dummy2;
  void* dialog_dummy3;
  void* dialog_dummy4;
  void* dialog_dummy5;
  void* dialog_dummy6;
  void* dialog_dummy7;
  void* dialog_dummy8;
  void* dialog_dummy9;
  void* dialog_dummy10; /* This and above used to add new dialogs */
  AddonToKodiFuncTable_kodi_gui_listItem* listItem;
  AddonToKodiFuncTable_kodi_gui_window* window;
} AddonToKodiFuncTable_kodi_gui;

} /* extern "C" */

//============================================================================
///
/// \ingroup cpp_kodi_gui_CControlRendering_Defs cpp_kodi_gui_CWindow_Defs
/// @{
/// @brief Handle to use as independent pointer for GUI
typedef void* GUIHANDLE;
/// @}
//----------------------------------------------------------------------------

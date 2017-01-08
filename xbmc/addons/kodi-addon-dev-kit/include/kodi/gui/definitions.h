#pragma once
/*
 *      Copyright (C) 2016 Team KODI
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

#include <string>
#include <time.h>

/*
 * Internal Structures to have "C"-Style data transfer
 */
extern "C"
{

typedef struct AddonToKodiFuncTable_kodi_gui_dialogOK
{
  void (*show_and_get_input_single_text)(void* kodiBase, const char *heading, const char *text);
  void (*show_and_get_input_line_text)(void* kodiBase, const char *heading, const char *line0, const char *line1, const char *line2);
} AddonToKodiFuncTable_kodi_gui_dialogOK;

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

typedef struct AddonToKodiFuncTable_kodi_gui
{
  AddonToKodiFuncTable_kodi_gui_general general;
  AddonToKodiFuncTable_kodi_gui_dialogOK dialogOK;
} AddonToKodiFuncTable_kodi_gui;

} /* extern "C" */

#pragma once
/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef LIBADDON_PLUS_PLUS_H
#define LIBADDON_PLUS_PLUS_H

#include <vector>
#include <string>
#include "xbmc_addon_types.h"

void XBMC_register_me(ADDON_HANDLE hdl);
void XBMC_log(const addon_log_t loglevel, const char *format, ... );
void XBMC_status_callback(const addon_status_t status, const char* msg);
bool XBMC_get_setting(std::string settingName, void *settingValue);
void XBMC_open_dialog_settings();
std::string XBMC_get_addon_directory();
std::string XBMC_get_user_directory();

bool XBMC_create_directory(std::string dir);
void XBMC_enable_nav_sounds(bool yesNo);
void XBMC_execute_built_in(std::string function);
std::string XBMC_execute_http_api(std::string httpcommand);
void XBMC_execute_script(std::string script);
std::string XBMC_get_cache_thumb_name(std::string path);
bool XBMC_get_cond_visibility(std::string condition);
#define DRIVE_OPEN                  0
#define DRIVE_NOT_READY             1
#define DRIVE_READY                 2
#define DRIVE_CLOSED_NO_MEDIA       3
#define DRIVE_CLOSED_MEDIA_PRESENT  4
#define DRIVE_NONE                  5
int XBMC_get_dvd_state();
int XBMC_get_free_memory();
int XBMC_get_global_idle_time();
std::string XBMC_get_info_image(std::string infotag);
std::string XBMC_get_info_label(std::string infotag);
std::string XBMC_get_ip_address();
std::string XBMC_get_language();
std::string XBMC_get_localized_string(long dwCode);
#define REGION_DATELONG     0
#define REGION_DATESHORT    1
#define REGION_TEMPUNIT     2
#define REGION_SPEEDUNIT    3
#define REGION_TIME         4
#define REGION_MERIDIEM     5
std::string XBMC_get_region(int id);
#define MEDIA_VIDEO     0
#define MEDIA_MUSIC     1
#define MEDIA_PICTURES  2
std::string XBMC_get_supported_media(int media);
void XBMC_play_sfx(std::string filename);
std::string XBMC_get_skin_dir();
std::string XBMC_make_legal_filename(std::string filename);
bool XBMC_skin_has_image(std::string filename);
std::string XBMC_translate_path(std::string path);
void XBMC_unknown_to_utf8(std::string &str);
void XBMC_shutdown();
void XBMC_restart();
void XBMC_dashboard();

bool XBMC_dialog_open_ok(const char* heading, const char* line0, const char* line1, const char* line2);
bool XBMC_dialog_open_yesno(const char* heading, const char* line0, const char* line1, const char* line2, const char* nolabel, const char* yeslabel);
#define SHOW_AND_GET_DIRECTORY          0
#define SHOW_AND_GET_FILE               1
#define SHOW_AND_GET_IMAGE              2
#define SHOW_AND_GET_WRITE_DIRECTORY    3
std::string XBMC_dialog_open_browse(int type, const char* heading, const char* shares, const char* mask, bool use_thumbs, bool treat_as_folder, const char* default_path);
#define SHOW_AND_GET_NUMBER
#define SHOW_AND_GET_DATE
#define SHOW_AND_GET_TIME
#define SHOW_AND_GET_IPADDRESS
std::string XBMC_dialog_open_numeric(int type, const char* heading, const char* default_value);
std::string XBMC_dialog_open_keyboard(const char* heading, const char* default_text, bool hidden);
int XBMC_dialog_open_select(const char* heading, std::vector<std::string> &list);
bool XBMC_dialog_progress_create(const char* heading, const char* line0, const char* line1, const char* line2);
void XBMC_dialog_progress_update(int percent, const char* line0, const char* line1, const char* line2);
bool XBMC_dialog_progress_is_canceled();
void XBMC_dialog_progress_close();

void XBMC_gui_lock();
void XBMC_gui_unlock();
int XBMC_gui_get_current_window_id();
int XBMC_gui_get_current_window_dialog_id();

bool isempty(const char *s);
char *strcpyrealloc(char *dest, const char *src);
char *strreplace(char *s, char c1, char c2);
char *strreplace(char *s, const char *s1, const char *s2); ///< re-allocates 's' and deletes the original string if necessary!
char *strn0cpy(char *dest, const char *src, size_t n);
bool xbmc_isnumber(const char *s);
inline char *skipspace(const char *s)
{
  if ((unsigned char)*s > ' ') // most strings don't have any leading space, so handle this case as fast as possible
     return (char *)s;
  while (*s && (unsigned char)*s <= ' ') // avoiding isspace() here, because it is much slower
        s++;
  return (char *)s;
}
char *stripspace(char *s);
char *compactspace(char *s);

#endif /* LIBADDON_PLUS_PLUS_H */

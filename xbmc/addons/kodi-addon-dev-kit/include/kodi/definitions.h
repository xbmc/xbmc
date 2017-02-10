#pragma once
/*
 *      Copyright (C) 2005-2017 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#if !defined(_WIN32) && !defined(TARGET_DARWIN)
  #include <sys/stat.h>
  #define __stat64 stat64
#endif

extern "C"
{

/*
 * For interface between add-on and kodi.
 *
 * In this structure becomes the addresses of functions inside Kodi stored who
 * then available for the add-on to call.
 *
 * All function pointers there are used by the C++ interface functions below.
 * You find the set of them on xbmc/addons/interfaces/kodi/General.cpp
 *
 * Note: For add-on development itself not needed, thats why with '*' here in
 * text.
 */
struct VFSDirEntry;
typedef struct AddonToKodiFuncTable_kodi_filesystem
{
  bool (*can_open_directory)(void* kodiBase, const char* url);
  bool (*create_directory)(void* kodiBase, const char* path);
  bool (*remove_directory)(void* kodiBase, const char* path);
  bool (*directory_exists)(void* kodiBase, const char* path);
  bool (*get_directory)(void* kodiBase, const char* path, const char* mask, VFSDirEntry** items, unsigned int* num_items);
  void (*free_directory)(void* kodiBase, VFSDirEntry* items, unsigned int num_items);

  bool (*file_exists)(void* kodiBase, const char *filename, bool useCache);
  int (*stat_file)(void* kodiBase, const char *filename, struct __stat64* buffer);
  bool (*delete_file)(void* kodiBase, const char *filename);
  bool (*rename_file)(void* kodiBase, const char *filename, const char *newFileName);
  bool (*copy_file)(void* kodiBase, const char *filename, const char *dest);
  bool (*file_set_hidden)(void* kodiBase, const char *filename, bool hidden);

  char* (*get_file_md5)(void* kodiBase, const char* filename);
  char* (*get_cache_thumb_name)(void* kodiBase, const char* filename);
  char* (*make_legal_filename)(void* kodiBase, const char* filename);
  char* (*make_legal_path)(void* kodiBase, const char* path);
  char* (*translate_special_protocol)(void* kodiBase, const char *strSource);

  void* (*open_file)(void* kodiBase, const char* filename, unsigned int flags);
  void* (*open_file_for_write)(void* kodiBase, const char* filename, bool overwrite);
  ssize_t (*read_file)(void* kodiBase, void* file, void* ptr, size_t size);
  bool (*read_file_string)(void* kodiBase, void* file, char *szLine, int iLineLength);
  ssize_t (*write_file)(void* kodiBase, void* file, const void* ptr, size_t size);
  void (*flush_file)(void* kodiBase, void* file);
  int64_t (*seek_file)(void* kodiBase, void* file, int64_t position, int whence);
  int (*truncate_file)(void* kodiBase, void* file, int64_t size);
  int64_t (*get_file_position)(void* kodiBase, void* file);
  int64_t (*get_file_length)(void* kodiBase, void* file);
  double (*get_file_download_speed)(void* kodiBase, void* file);
  void (*close_file)(void* kodiBase, void* file);
  int (*get_file_chunk_size)(void* kodiBase, void* file);

  void* (*curl_create)(void* kodiBase, const char* url);
  bool (*curl_add_option)(void* kodiBase, void* file, int type, const char* name, const char* value);
  bool (*curl_open)(void* kodiBase, void* file, unsigned int flags);
} AddonToKodiFuncTable_kodi_filesystem;

typedef struct AddonToKodiFuncTable_kodi
{
  bool (*get_setting)(void* kodiBase, const char* settingName, void *settingValue);
  bool (*set_setting)(void* kodiBase, const char* settingName, const char* settingValue);
  void (*open_settings_dialog)(void* kodiBase);
  char* (*get_localized_string)(void* kodiBase, long dwCode);
  AddonToKodiFuncTable_kodi_filesystem filesystem;
} AddonToKodiFuncTable_kodi;

} /* extern "C" */

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

#include "FileItem.h"

extern "C"
{

struct VFSDirEntry;
struct AddonGlobalInterface;

namespace ADDON
{

  struct Interface_Filesystem
  {
    static void Init(AddonGlobalInterface* addonInterface);
    static void DeInit(AddonGlobalInterface* addonInterface);

    /*!
     * @brief callback functions from add-on to kodi
     *
     * @note For add of new functions use the "_" style to identify direct a
     * add-on callback function. Everything with CamelCase is only for the
     * usage in Kodi only.
     *
     * The parameter `kodiBase` is used to become the pointer for a `CAddonDll`
     * class.
     */
    //@{
    static bool can_open_directory(void* kodiBase, const char* url);
    static bool create_directory(void* kodiBase, const char *path);
    static bool directory_exists(void* kodiBase, const char *path);
    static bool remove_directory(void* kodiBase, const char *path);
    static bool get_directory(void* kodiBase, const char* path, const char* mask, VFSDirEntry** items, unsigned int* num_items);
    static void free_directory(void* kodiBase, VFSDirEntry* items, unsigned int num_items);

    static bool file_exists(void* kodiBase, const char *filename, bool useCache);
    static int stat_file(void* kodiBase, const char *filename, struct __stat64* buffer);
    static bool delete_file(void* kodiBase, const char *filename);
    static bool rename_file(void* kodiBase, const char *filename, const char *newFileName);
    static bool copy_file(void* kodiBase, const char *filename, const char *dest);
    static char* get_file_md5(void* kodiBase, const char* filename);
    static char* get_cache_thumb_name(void* kodiBase, const char* filename);
    static char* make_legal_filename(void* kodiBase, const char* filename);
    static char* make_legal_path(void* kodiBase, const char* path);
    static char* translate_special_protocol(void* kodiBase, const char *strSource);

    static void* open_file(void* kodiBase, const char* filename, unsigned int flags);
    static void* open_file_for_write(void* kodiBase, const char* filename, bool overwrite);
    static ssize_t read_file(void* kodiBase, void* file, void* ptr, size_t size);
    static bool read_file_string(void* kodiBase, void* file, char *szLine, int lineLength);
    static ssize_t write_file(void* kodiBase, void* file, const void* ptr, size_t size);
    static void flush_file(void* kodiBase, void* file);
    static int64_t seek_file(void* kodiBase, void* file, int64_t position, int whence);
    static int truncate_file(void* kodiBase, void* file, int64_t size);
    static int64_t get_file_position(void* kodiBase, void* file);
    static int64_t get_file_length(void* kodiBase, void* file);
    static double get_file_download_speed(void* kodiBase, void* file);
    static void close_file(void* kodiBase, void* file);
    static int get_file_chunk_size(void* kodiBase, void* file);

    static void* curl_create(void* kodiBase, const char* url);
    static bool curl_add_option(void* kodiBase, void* file, int type, const char* name, const char* value);
    static bool curl_open(void* kodiBase, void* file, unsigned int flags);
    //@}
  };

} /* namespace ADDON */
} /* extern "C" */

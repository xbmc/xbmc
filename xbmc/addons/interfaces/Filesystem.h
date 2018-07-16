/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
    static char** get_property_values(void* kodiBase, void* file, int type, const char *name, int *numValues);

    static void* curl_create(void* kodiBase, const char* url);
    static bool curl_add_option(void* kodiBase, void* file, int type, const char* name, const char* value);
    static bool curl_open(void* kodiBase, void* file, unsigned int flags);
    //@}
  };

} /* namespace ADDON */
} /* extern "C" */

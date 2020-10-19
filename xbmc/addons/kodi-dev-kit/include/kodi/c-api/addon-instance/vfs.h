/*
 *  Copyright (C) 2005-2020 Team Kodi
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_VFS_H
#define C_API_ADDONINSTANCE_VFS_H

#include "../addon_base.h"
#include "../filesystem.h"

#define VFS_FILE_HANDLE void*

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  struct VFSURL
  {
    const char* url;
    const char* domain;
    const char* hostname;
    const char* filename;
    unsigned int port;
    const char* options;
    const char* username;
    const char* password;
    const char* redacted;
    const char* sharename;
    const char* protocol;
  };

  typedef struct VFSGetDirectoryCallbacks /* internal */
  {
    bool(__cdecl* get_keyboard_input)(KODI_HANDLE ctx,
                                      const char* heading,
                                      char** input,
                                      bool hidden_input);
    void(__cdecl* set_error_dialog)(KODI_HANDLE ctx,
                                    const char* heading,
                                    const char* line1,
                                    const char* line2,
                                    const char* line3);
    void(__cdecl* require_authentication)(KODI_HANDLE ctx, const char* url);
    KODI_HANDLE ctx;
  } VFSGetDirectoryCallbacks;

  typedef struct AddonProps_VFSEntry /* internal */
  {
    int dummy;
  } AddonProps_VFSEntry;

  typedef struct AddonToKodiFuncTable_VFSEntry /* internal */
  {
    KODI_HANDLE kodiInstance;
  } AddonToKodiFuncTable_VFSEntry;

  struct AddonInstance_VFSEntry;
  typedef struct KodiToAddonFuncTable_VFSEntry /* internal */
  {
    KODI_HANDLE addonInstance;

    VFS_FILE_HANDLE(__cdecl* open)
    (const struct AddonInstance_VFSEntry* instance, const struct VFSURL* url);
    VFS_FILE_HANDLE(__cdecl* open_for_write)
    (const struct AddonInstance_VFSEntry* instance, const struct VFSURL* url, bool overwrite);
    ssize_t(__cdecl* read)(const struct AddonInstance_VFSEntry* instance,
                           VFS_FILE_HANDLE context,
                           uint8_t* buffer,
                           size_t buf_size);
    ssize_t(__cdecl* write)(const struct AddonInstance_VFSEntry* instance,
                            VFS_FILE_HANDLE context,
                            const uint8_t* buffer,
                            size_t buf_size);
    int64_t(__cdecl* seek)(const struct AddonInstance_VFSEntry* instance,
                           VFS_FILE_HANDLE context,
                           int64_t position,
                           int whence);
    int(__cdecl* truncate)(const struct AddonInstance_VFSEntry* instance,
                           VFS_FILE_HANDLE context,
                           int64_t size);
    int64_t(__cdecl* get_length)(const struct AddonInstance_VFSEntry* instance,
                                 VFS_FILE_HANDLE context);
    int64_t(__cdecl* get_position)(const struct AddonInstance_VFSEntry* instance,
                                   VFS_FILE_HANDLE context);
    int(__cdecl* get_chunk_size)(const struct AddonInstance_VFSEntry* instance,
                                 VFS_FILE_HANDLE context);
    bool(__cdecl* io_control_get_seek_possible)(const struct AddonInstance_VFSEntry* instance,
                                                VFS_FILE_HANDLE context);
    bool(__cdecl* io_control_get_cache_status)(const struct AddonInstance_VFSEntry* instance,
                                               VFS_FILE_HANDLE context,
                                               struct VFS_CACHE_STATUS_DATA* status);
    bool(__cdecl* io_control_set_cache_rate)(const struct AddonInstance_VFSEntry* instance,
                                             VFS_FILE_HANDLE context,
                                             unsigned int rate);
    bool(__cdecl* io_control_set_retry)(const struct AddonInstance_VFSEntry* instance,
                                        VFS_FILE_HANDLE context,
                                        bool retry);
    int(__cdecl* stat)(const struct AddonInstance_VFSEntry* instance,
                       const struct VFSURL* url,
                       struct STAT_STRUCTURE* buffer);
    bool(__cdecl* close)(const struct AddonInstance_VFSEntry* instance, VFS_FILE_HANDLE context);

    bool(__cdecl* exists)(const struct AddonInstance_VFSEntry* instance, const struct VFSURL* url);
    void(__cdecl* clear_out_idle)(const struct AddonInstance_VFSEntry* instance);
    void(__cdecl* disconnect_all)(const struct AddonInstance_VFSEntry* instance);
    bool(__cdecl* delete_it)(const struct AddonInstance_VFSEntry* instance,
                             const struct VFSURL* url);
    bool(__cdecl* rename)(const struct AddonInstance_VFSEntry* instance,
                          const struct VFSURL* url,
                          const struct VFSURL* url2);
    bool(__cdecl* directory_exists)(const struct AddonInstance_VFSEntry* instance,
                                    const struct VFSURL* url);
    bool(__cdecl* remove_directory)(const struct AddonInstance_VFSEntry* instance,
                                    const struct VFSURL* url);
    bool(__cdecl* create_directory)(const struct AddonInstance_VFSEntry* instance,
                                    const struct VFSURL* url);
    bool(__cdecl* get_directory)(const struct AddonInstance_VFSEntry* instance,
                                 const struct VFSURL* url,
                                 struct VFSDirEntry** entries,
                                 int* num_entries,
                                 struct VFSGetDirectoryCallbacks* callbacks);
    bool(__cdecl* contains_files)(const struct AddonInstance_VFSEntry* instance,
                                  const struct VFSURL* url,
                                  struct VFSDirEntry** entries,
                                  int* num_entries,
                                  char* rootpath);
    void(__cdecl* free_directory)(const struct AddonInstance_VFSEntry* instance,
                                  struct VFSDirEntry* entries,
                                  int num_entries);
  } KodiToAddonFuncTable_VFSEntry;

  typedef struct AddonInstance_VFSEntry /* internal */
  {
    struct AddonProps_VFSEntry* props;
    struct AddonToKodiFuncTable_VFSEntry* toKodi;
    struct KodiToAddonFuncTable_VFSEntry* toAddon;
  } AddonInstance_VFSEntry;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_VFS_H */

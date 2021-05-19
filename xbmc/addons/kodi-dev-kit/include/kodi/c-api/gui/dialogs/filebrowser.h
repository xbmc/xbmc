/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_GUI_DIALOGS_FILEBROWSER_H
#define C_API_GUI_DIALOGS_FILEBROWSER_H

#include "../definitions.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

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

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_GUI_DIALOGS_FILEBROWSER_H */

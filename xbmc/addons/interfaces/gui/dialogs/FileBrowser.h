/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/gui/dialogs/filebrowser.h"

#include <string>
#include <vector>

class CMediaSource;

typedef std::vector<CMediaSource> VECSOURCES;

extern "C"
{

  struct AddonGlobalInterface;

  namespace ADDON
  {

  /*!
   * @brief Global gui Add-on to Kodi callback functions
   *
   * To hold functions not related to a instance type and usable for
   * every add-on type.
   *
   * Related add-on header is "./xbmc/addons/kodi-dev-kit/include/kodi/gui/dialogs/FileBrowser.h"
   */
  struct Interface_GUIDialogFileBrowser
  {
    static void Init(AddonGlobalInterface* addonInterface);
    static void DeInit(AddonGlobalInterface* addonInterface);

    /*!
     * @brief callback functions from add-on to kodi
     *
     * @note To add a new function use the "_" style to directly identify an
     * add-on callback function. Everything with CamelCase is only to be used
     * in Kodi.
     *
     * The parameter `kodiBase` is used to become the pointer for a `CAddonDll`
     * class.
     */
    //@{
    static bool show_and_get_directory(KODI_HANDLE kodiBase,
                                       const char* shares,
                                       const char* heading,
                                       const char* path_in,
                                       char** path_out,
                                       bool write_only);

    static bool show_and_get_file(KODI_HANDLE kodiBase,
                                  const char* shares,
                                  const char* mask,
                                  const char* heading,
                                  const char* path_in,
                                  char** path_out,
                                  bool use_thumbs,
                                  bool use_file_directories);

    static bool show_and_get_file_from_dir(KODI_HANDLE kodiBase,
                                           const char* directory,
                                           const char* mask,
                                           const char* heading,
                                           const char* path_in,
                                           char** path_out,
                                           bool use_thumbs,
                                           bool use_file_directories,
                                           bool singleList);

    static bool show_and_get_file_list(KODI_HANDLE kodiBase,
                                       const char* shares,
                                       const char* mask,
                                       const char* heading,
                                       char*** file_list,
                                       unsigned int* entries,
                                       bool use_thumbs,
                                       bool use_file_directories);

    static bool show_and_get_source(KODI_HANDLE kodiBase,
                                    const char* path_in,
                                    char** path_out,
                                    bool allow_network_shares,
                                    const char* additional_share,
                                    const char* type);

    static bool show_and_get_image(KODI_HANDLE kodiBase,
                                   const char* shares,
                                   const char* heading,
                                   const char* path_in,
                                   char** path_out);

    static bool show_and_get_image_list(KODI_HANDLE kodiBase,
                                        const char* shares,
                                        const char* heading,
                                        char*** file_list,
                                        unsigned int* entries);

    static void clear_file_list(KODI_HANDLE kodiBase, char*** file_list, unsigned int entries);
    //@}

  private:
    static void GetVECShares(VECSOURCES& vecShares,
                             const std::string& strShares,
                             const std::string& strPath);
  };

  } /* namespace ADDON */
} /* extern "C" */

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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

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
   * Related add-on header is "./xbmc/addons/kodi-addon-dev-kit/include/kodi/gui/DialogFileBrowser.h"
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
    static bool show_and_get_directory(void* kodiBase, const char* shares, 
                                       const char* heading, const char* path_in,
                                       char** path_out, bool write_only);

    static bool show_and_get_file(void* kodiBase, const char* shares,
                                  const char* mask, const char* heading,
                                  const char* path_in, char** path_out,
                                  bool use_thumbs, bool use_file_directories);

    static bool show_and_get_file_from_dir(void* kodiBase, const char* directory,
                                           const char* mask, const char* heading,
                                           const char* path_in, char** path_out,
                                           bool use_thumbs, bool use_file_directories,
                                           bool singleList);

    static bool show_and_get_file_list(void* kodiBase, const char* shares, 
                                       const char* mask, const char* heading, 
                                       char*** file_list, unsigned int* entries, 
                                       bool use_thumbs, bool use_file_directories);

    static bool show_and_get_source(void* kodiBase, const char* path_in, char** path_out,
                                    bool allow_network_shares, const char* additional_share, 
                                    const char* type);

    static bool show_and_get_image(void* kodiBase, const char* shares, const char* heading,
                                   const char* path_in, char** path_out);

    static bool show_and_get_image_list(void* kodiBase, const char* shares,
                                        const char* heading, char*** file_list,
                                        unsigned int* entries);

    static void clear_file_list(void* kodiBase, char*** file_list, unsigned int entries);
    //@}

  private:
    static void GetVECShares(VECSOURCES& vecShares, const std::string& strShares, const std::string& strPath);
  };

} /* namespace ADDON */
} /* extern "C" */

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "AddonString.h"
#include "Tuple.h"
#include <vector>

namespace XBMCAddon
{

  namespace xbmcvfs
  {
    /**
     * copy(source, destination) -- copy file to destination, returns true/false.
     * 
     * source          : file to copy.\n
     * destination     : destination fi\n
     * 
     * example:
     *   - success = xbmcvfs.copy(source, destination)
     */
    bool copy(const String& strSource, const String& strDestnation);

    /**
     * delete(file)
     * 
     * file        : file to dele
     * 
     * example:
     *   xbmcvfs.delete(file)
     */
    // delete a file
    bool deleteFile(const String& file);

    /**
     * rename(file, newFileName)
     * 
     * file        : file to reana\n
     * newFileName : new filename, including the full pa
     * 
     * example:\n
     *   success = xbmcvfs.rename(file,newFileName)\n
     */
    // rename a file
    bool rename(const String& file, const String& newFile);

    /**
     * exists(path)\n
     * \n
     * path        : file or folder (folder must end with slash or backslash)\n
     * \n
     * example:\n
     *   success = xbmcvfs.exists(path)\n
     */
    // check for a file or folder existance, mimics Pythons os.path.exists()
    bool exists(const String& path);

    /**
     * mkdir(path) -- Create a folder.\n
     * \n
     * path        : folder\n
     * \n
     * example:\n
     *  - success = xbmcvfs.mkdir(path)\n
     */
    // make a directory
    bool mkdir(const String& path);

    /**
     * mkdirs(path) -- Create folder(s) - it will create all folders in the path.\n
     * \n
     * path        : folder\n
     * \n
     * example:\n
     *  - success = xbmcvfs.mkdirs(path)\n
     */
    // make all directories along the path
    bool mkdirs(const String& path);

    /**
     * rmdir(path) -- Remove a folder.\n
     * \n
     * path        : folder\n
     * \n
     * example:\n
     *  - success = xbmcvfs.rmdir(path)n\n
     */
    bool rmdir(const String& path, bool force = false);

    /**
     * listdir(path) -- lists content of a folder.\n
     * \n
     * path        : folder\n
     * 
     * example:
     *  - dirs, files = xbmcvfs.listdir(path)
     */
    Tuple<std::vector<String>, std::vector<String> > listdir(const String& path);
  }
}


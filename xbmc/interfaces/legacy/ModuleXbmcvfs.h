/*
 *      Copyright (C) 2005-2012 Team XBMC
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
     * source          : file to copy.
     * destination     : destination fi
     * 
     * example:
     *   success = xbmcvfs.copy(source, destination)
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
     * file        : file to reana
     * newFileName : new filename, including the full pa
     * 
     * example:
     *   success = xbmcvfs.rename(file,newFileName)
     */
    // rename a file
    bool rename(const String& file, const String& newFile);

    /**
     * exists(path)
     * 
     * path        : file or fold
     * 
     * example:
     *   success = xbmcvfs.exists(path)
     */
    // check for a file or folder existance, mimics Pythons os.path.exists()
    bool exists(const String& path);

    /**
     * mkdir(path) -- Create a folder.
     * 
     * path        : folder
     * 
     * example:
     *  - success = xbmcvfs.mkdir(path)
     */
    // make a directory
    bool mkdir(const String& path);

    /**
     * mkdirs(path) -- Create folder(s) - it will create all folders in the path.
     * 
     * path        : folder
     * 
     * example:
     *  - success = xbmcvfs.mkdirs(path)
     */
    // make all directories along the path
    bool mkdirs(const String& path);

    /**
     * rmdir(path) -- Remove a folder.
     * 
     * path        : folder
     * 
     * example:
     *  - success = xbmcvfs.rmdir(path)\n
     */
    bool rmdir(const String& path, bool force = false);

    /**
     * listdir(path) -- lists content of a folder.
     * 
     * path        : folder
     * 
     * example:
     *  - dirs, files = xbmcvfs.listdir(path)
     */
    Tuple<std::vector<String>, std::vector<String> > listdir(const String& path);
  }
}


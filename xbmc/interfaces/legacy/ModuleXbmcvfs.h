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

#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace XBMCAddon
{
  namespace xbmcvfs
  {
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

    //
    /// \defgroup python_xbmcvfs Library - xbmcvfs
    /// @{
    /// @brief <b>Virtual file system functions on Kodi.</b>
    ///
    /// Offers classes and functions offers acces to the Virtual File Server
    /// (VFS) which you can use to manipulate files and folders.
    //

    ///
    /// \ingroup python_xbmcvfs
    /// Copy file to destination, returns true/false.
    ///
    /// @param[in] source                file to copy.
    /// @param[in] destination           destination file
    /// @return                          True if successed
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// success = xbmcvfs.copy(source, destination)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    bool copy(const String& strSource, const String& strDestnation);

    ///
    /// \ingroup python_xbmcvfs
    /// @brief Delete a file
    ///
    /// @param[in] file                  File to delete
    /// @return                          True if successed
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmcvfs.delete(file)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    bool deleteFile(const String& file);

    ///
    /// \ingroup python_xbmcvfs
    /// @brief Rename a file
    ///
    /// @param[in] file                  File to rename
    /// @param[in] newFileName           New filename, including the full path
    /// @return                          True if successed
    ///
    /// @note Moving files between different filesystem (eg. local to nfs://) is not possible on
    ///       all platforms. You may have to do it manually by using the copy and deleteFile functions.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// success = xbmcvfs.rename(file,newFileName)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    bool rename(const String& file, const String& newFile);

    ///
    /// \ingroup python_xbmcvfs
    /// @brief Check for a file or folder existance
    ///
    /// @param[in] path                  File or folder (folder must end with
    ///                                  slash or backslash)
    /// @return                          True if successed
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// success = xbmcvfs.exists(path)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    bool exists(const String& path);

    ///
    /// \ingroup python_xbmcvfs
    /// @brief Create a folder.
    ///
    /// @param[in] path                  Folder to create
    /// @return                          True if successed
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// success = xbmcvfs.mkdir(path)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    bool mkdir(const String& path);

    ///
    /// \ingroup python_xbmcvfs
    /// @brief Make all directories along the path
    ///
    /// Create folder(s) - it will create all folders in the path.
    ///
    /// @param[in] path                  Folders to create
    /// @return                          True if successed
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// success = xbmcvfs.mkdirs(path)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    bool mkdirs(const String& path);

    ///
    /// \ingroup python_xbmcvfs
    /// @brief Remove a folder.
    ///
    /// @param[in] path                  Folder to remove
    /// @return                          True if successed
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// success = xbmcvfs.rmdir(path)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    bool rmdir(const String& path, bool force = false);

    ///
    /// \ingroup python_xbmcvfs
    /// @brief Lists content of a folder.
    ///
    /// @param[in] path                  Folder to get list from
    /// @return                          Directory content list
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// dirs, files = xbmcvfs.listdir(path)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    Tuple<std::vector<String>, std::vector<String> > listdir(const String& path);
    //@}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
  }
}
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

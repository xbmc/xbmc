/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
    /// @brief **Virtual file system functions on Kodi.**
    ///
    /// Offers classes and functions offers access to the Virtual File Server
    /// (VFS) which you can use to manipulate files and folders.
    //

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcvfs
    /// @brief \python_func{ xbmcvfs.copy(source, destination) }
    /// Copy file to destination, returns true/false.
    ///
    /// @param source                file to copy.
    /// @param destination           destination file
    /// @return                      True if successed
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
    copy(...);
#else
    bool copy(const String& strSource, const String& strDestination);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcvfs
    /// @brief \python_func{ xbmcvfs.delete(file) }
    /// Delete a file
    ///
    /// @param file                  File to delete
    /// @return                      True if successed
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
    delete(...);
#else
    bool deleteFile(const String& file);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcvfs
    /// @brief \python_func{ xbmcvfs.rename(file, newFileName) }
    /// Rename a file
    ///
    /// @param file                  File to rename
    /// @param newFileName           New filename, including the full path
    /// @return                      True if successed
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
    rename(...);
#else
    bool rename(const String& file, const String& newFile);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcvfs
    /// @brief \python_func{ xbmcvfs.exists(path) }
    /// Check for a file or folder existence
    ///
    /// @param path                  File or folder (folder must end with
    ///                              slash or backslash)
    /// @return                      True if successed
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
    exists(...);
#else
    bool exists(const String& path);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcvfs
    /// @brief \python_func{ xbmcvfs.makeLegalFilename(filename) }
    /// Returns a legal filename or path as a string.
    ///
    /// @param filename              string - filename/path to make legal
    /// @return              Legal filename or path as a string
    ///
    ///
    /// @note The returned value is platform-specific. This is due to the fact that
    /// the chars that need to be replaced to make a path legal depend on the
    /// underlying OS filesystem. This is useful, for example, if you want to create
    /// a file or folder based on data over which you have no control (e.g. an external API).
    ///
    ///
    ///-------------------------------------------------------------------------
    /// @python_v19 New function added (replaces old **xbmc.makeLegalFilename**)
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// # windows
    /// >> xbmcvfs.makeLegalFilename('C://Trailers/Ice Age: The Meltdown.avi')
    /// C:\Trailers\Ice Age_ The Meltdown.avi
    /// # non-windows
    /// >> xbmcvfs.makeLegalFilename("///\\jk???lj????.mpg")
    /// /jk___lj____.mpg
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    makeLegalFilename(...);
#else
    String makeLegalFilename(const String& filename);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcvfs
    /// @brief \python_func{ xbmcvfs.translatePath(path)  }
    /// Returns the translated path.
    ///
    /// @param path string - Path to format
    /// @return Translated path
    ///
    /// @note Only useful if you are coding for both Linux and Windows.
    ///       e.g. Converts 'special://home' -> '/home/[username]/.kodi'
    ///       on Linux.
    ///
    ///
    /// ------------------------------------------------------------------------
    /// @python_v19 New function added (replaces old **xbmc.translatePath**)
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// fpath = xbmcvfs.translatePath('special://home')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    translatePath(...);
#else
    String translatePath(const String& path);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcvfs
    /// @brief \python_func{ xbmcvfs.validatePath(path) }
    /// Returns the validated path.
    ///
    /// @param path                  string  - Path to format
    /// @return            Validated path
    ///
    /// @note The result is platform-specific. Only useful if you are coding
    ///       for multiple platfforms for fixing slash problems
    ///         (e.g. Corrects 'Z://something' -> 'Z:\something').
    ///
    ///
    /// ------------------------------------------------------------------------
    /// @python_v19 New function added (replaces old **xbmc.validatePath**)
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// fpath = xbmcvfs.validatePath(somepath)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    validatePath(...);
#else
    String validatePath(const String& path);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcvfs
    /// @brief \python_func{ xbmcvfs.mkdir(path) }
    /// Create a folder.
    ///
    /// @param path                  Folder to create
    /// @return                      True if successed
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
    mkdir(...);
#else
    bool mkdir(const String& path);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcvfs
    /// @brief \python_func{ xbmcvfs.mkdirs(path) }
    /// Make all directories along the path
    ///
    /// Create folder(s) - it will create all folders in the path.
    ///
    /// @param path                  Folders to create
    /// @return                      True if successed
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
    mkdirs(...);
#else
    bool mkdirs(const String& path);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcvfs
    /// @brief \python_func{ xbmcvfs.rmdir(path) }
    /// Remove a folder.
    ///
    /// @param path                  Folder to remove
    /// @return                      True if successed
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
    rmdir(...);
#else
    bool rmdir(const String& path, bool force = false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcvfs
    /// @brief \python_func{ xbmcvfs.listdir(path) }
    /// Lists content of a folder.
    ///
    /// @param path                  Folder to get list from
    /// @return                      Directory content list
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
    listdir(...);
#else
    Tuple<std::vector<String>, std::vector<String> > listdir(const String& path);
#endif
    //@}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
  }
}
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

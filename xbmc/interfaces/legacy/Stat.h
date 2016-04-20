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

#include "filesystem/File.h"
#include "AddonClass.h"
#include "LanguageHook.h"

namespace XBMCAddon
{
  namespace xbmcvfs
  {
    //
    /// \defgroup python_stat Stat
    /// \ingroup python_xbmcvfs
    /// @{
    /// @brief **Get file or file system status.**
    ///
    /// \python_class{ xbmcvfs.Stat(path) }
    ///
    /// These class return information about a file. Execute (search) permission
    /// is required on all of the directories in path that lead to the file.
    ///
    /// @param path                  [string] file or folder
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    ///   st = xbmcvfs.Stat(path)
    ///   modified = st.st_mtime()
    /// ..
    /// ~~~~~~~~~~~~~
    //
    class Stat : public AddonClass
    {
      struct __stat64 st;

    public:
      Stat(const String& path)
      {
        DelayedCallGuard dg;
        XFILE::CFile::Stat(path, &st);
      }

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_stat
      /// @brief \python_func{ st_mode() }
      ///-----------------------------------------------------------------------
      /// To get file protection.
      ///
      /// @return                        st_mode
      ///
      st_mode();
#else
      inline long long st_mode() { return st.st_mode; };
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_stat
      /// @brief \python_func{ st_ino() }
      ///-----------------------------------------------------------------------
      /// To get inode number.
      ///
      /// @return                        st_ino
      ///
      st_ino();
#else
      inline long long st_ino() { return st.st_ino; };
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_stat
      /// @brief \python_func{ st_dev() }
      ///-----------------------------------------------------------------------
      /// To get ID of device containing file.
      ///
      /// The st_dev field describes the device on which this file resides.
      ///
      /// @return                        st_dev
      ///
      st_dev();
#else
      inline long long st_dev() { return st.st_dev; };
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_stat
      /// @brief \python_func{ st_nlink() }
      ///-----------------------------------------------------------------------
      /// To get number of hard links.
      ///
      /// @return                        st_nlink
      ///
      st_nlink();
#else
      inline long long st_nlink() { return st.st_nlink; };
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_stat
      /// @brief \python_func{ st_uid() }
      ///-----------------------------------------------------------------------
      /// To get user ID of owner.
      ///
      /// @return                        st_uid
      ///
      st_uid();
#else
      inline long long st_uid() { return st.st_uid; };
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_stat
      /// @brief \python_func{ st_gid() }
      ///-----------------------------------------------------------------------
      /// To get group ID of owner.
      ///
      /// @return                        st_gid
      ///
      st_gid();
#else
      inline long long st_gid() { return st.st_gid; };
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_stat
      /// @brief \python_func{ st_size() }
      ///-----------------------------------------------------------------------
      /// To get total size, in bytes.
      ///
      /// The st_size field gives the size of the file (if it is a regular file
      /// or a symbolic link) in bytes. The size of a symbolic link (only on
      /// Linux and Mac OS X) is the length of the pathname it contains, without
      /// a terminating null byte.
      ///
      /// @return                        st_size
      ///
      st_size();
#else
      inline long long st_size() { return st.st_size; };
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_stat
      /// @brief \python_func{ atime() }
      ///-----------------------------------------------------------------------
      /// To get time of last access.
      ///
      /// @return                        st_atime
      ///
      atime();
#else
      inline long long atime() { return st.st_atime; }; //names st_atime/st_mtime/st_ctime are used by sys/stat.h
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_stat
      /// @brief \python_func{ mtime() }
      ///-----------------------------------------------------------------------
      /// To get time of last modification.
      ///
      /// @return                        st_mtime
      ///
      mtime();
#else
      inline long long mtime() { return st.st_mtime; };
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_stat
      /// @brief \python_func{ ctime() }
      ///-----------------------------------------------------------------------
      /// To get time of last status change.
      ///
      /// @return                        st_ctime
      ///
      ctime();
#else
      inline long long ctime() { return st.st_ctime; };
#endif
    };
    /// @}
  }
}


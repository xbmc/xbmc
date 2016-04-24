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
#include "AddonString.h"
#include "AddonClass.h"
#include "LanguageHook.h"
#include "commons/Buffer.h"

#include <algorithm>

namespace XBMCAddon
{

  namespace xbmcvfs
  {

    //
    /// \defgroup python_file File
    /// \ingroup python_xbmcvfs
    /// @{
    /// @brief <b>Kodi's file class.</b>
    ///
    /// \python_class{ xbmcvfs.File(filepath, [mode]) }
    ///
    /// @param filepath             string Selected file path
    /// @param mode                 [opt] string Additional mode options
    ///   |  Mode  | Description                     |
    ///   |:------:|:--------------------------------|
    ///   |   w    | Open for write                  |
    ///
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// f = xbmcvfs.File(file, ['w'])
    /// ..
    /// ~~~~~~~~~~~~~
    //
    class File : public AddonClass
    {
      XFILE::CFile* file;
    public:
      inline File(const String& filepath, const char* mode = NULL) : file(new XFILE::CFile())
      {
        DelayedCallGuard dg(languageHook);
        if (mode && strncmp(mode, "w", 1) == 0)
          file->OpenForWrite(filepath,true);
        else
          file->Open(filepath, XFILE::READ_NO_CACHE);
      }

      inline ~File() { delete file; }

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_file
      /// @brief \python_func{ read([bytes]) }
      ///-----------------------------------------------------------------------
      /// Read file parts as string.
      ///
      /// @param bytes              [opt] How many bytes to read - if not
      ///                               set it will read the whole file
      /// @return                       string
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// f = xbmcvfs.File(file)
      /// b = f.read()
      /// f.close()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      read(...);
#else
      inline String read(unsigned long numBytes = 0)
      {
        XbmcCommons::Buffer b = readBytes(numBytes);
        return b.getString(numBytes == 0 ? b.remaining() : std::min((unsigned long)b.remaining(),numBytes));
      }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_file
      /// @brief \python_func{ readBytes(numbytes) }
      ///-----------------------------------------------------------------------
      /// Read bytes from file.
      ///
      /// @param numbytes           How many bytes to read [opt]- if not set
      ///                               it will read the whole file
      /// @return                       bytearray
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// f = xbmcvfs.File(file)
      /// b = f.read()
      /// f.close()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      readBytes(...);
#else
      XbmcCommons::Buffer readBytes(unsigned long numBytes = 0);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_file
      /// @brief \python_func{ write(buffer) }
      ///-----------------------------------------------------------------------
      /// To write given data in file.
      ///
      /// @param buffer             Buffer to write to file
      /// @return                       True on success.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// f = xbmcvfs.File(file, 'w', True)
      /// result = f.write(buffer)
      /// f.close()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      write(...);
#else
      bool write(XbmcCommons::Buffer& buffer);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_file
      /// @brief \python_func{ size() }
      ///-----------------------------------------------------------------------
      /// Get the file size.
      ///
      /// @return                       The file size
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// f = xbmcvfs.File(file)
      /// s = f.size()
      /// f.close()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      size();
#else
      inline long long size() { DelayedCallGuard dg(languageHook); return file->GetLength(); }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_file
      /// @brief \python_func{ seek(seekBytes, iWhence) }
      ///-----------------------------------------------------------------------
      /// Seek to position in file.
      ///
      /// @param seekBytes          position in the file
      /// @param iWhence            where in a file to seek from[0 begining,
      ///                           1 current , 2 end possition]
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// f = xbmcvfs.File(file)
      /// result = f.seek(8129, 0)
      /// f.close()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      seek(...);
#else
      inline long long seek(long long seekBytes, int iWhence) { DelayedCallGuard dg(languageHook); return file->Seek(seekBytes,iWhence); }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_file
      /// @brief \python_func{ close() }
      ///-----------------------------------------------------------------------
      /// Close opened file.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// f = xbmcvfs.File(file)
      /// f.close()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      close();
#else
      inline void close() { DelayedCallGuard dg(languageHook); file->Close(); }
#endif

#ifndef SWIG
      inline const XFILE::CFile* getFile() const { return file; }
#endif

    };
    //@}
  }
}

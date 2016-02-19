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
    /// <b><c>xbmcvfs.File(filepath, [mode])</c></b>
    ///
    /// @param[in] filepath             string Selected file path
    /// @param[in] mode                 [opt] string Additional mode options
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
          file->Open(filepath, READ_NO_CACHE);
      }

      inline ~File() { delete file; }

      ///
      /// \ingroup python_file
      /// @brief Read file parts as string.
      ///
      /// @param[in] bytes              [opt] How many bytes to read - if not
      ///                               set it will read the whole file
      /// @return                       string
      ///
      ///
      ///--------------------------------------------------------------------------
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
      inline String read(unsigned long numBytes = 0)
      {
        XbmcCommons::Buffer b = readBytes(numBytes);
        return b.getString(numBytes == 0 ? b.remaining() : std::min((unsigned long)b.remaining(),numBytes));
      }

      ///
      /// \ingroup python_file
      /// @brief Read bytes from file.
      ///
      /// @param[in] numbytes           How many bytes to read [opt]- if not set
      ///                               it will read the whole file
      /// @return                       bytearray
      ///
      ///
      ///--------------------------------------------------------------------------
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
      XbmcCommons::Buffer readBytes(unsigned long numBytes = 0);

      ///
      /// \ingroup python_file
      /// @brief To write given data in file.
      ///
      /// @param[in] buffer             Buffer to write to file
      /// @return                       True on success.
      ///
      ///
      ///--------------------------------------------------------------------------
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
      bool write(XbmcCommons::Buffer& buffer);

      ///
      /// \ingroup python_file
      /// @brief Get the file size.
      ///
      /// @return                       The file size
      ///
      ///
      ///--------------------------------------------------------------------------
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
      inline long long size() { DelayedCallGuard dg(languageHook); return file->GetLength(); }

      ///
      /// \ingroup python_file
      /// @brief Seek to position in file.
      ///
      /// @param[in] seekBytes          position in the file
      /// @param[in] iWhence            where in a file to seek from[0 begining,
      ///                               1 current , 2 end possition]
      ///
      ///
      ///--------------------------------------------------------------------------
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
      inline long long seek(long long seekBytes, int iWhence) { DelayedCallGuard dg(languageHook); return file->Seek(seekBytes,iWhence); }

      ///
      /// \ingroup python_file
      /// @brief Close opened file.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// f = xbmcvfs.File(file)
      /// f.close()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      inline void close() { DelayedCallGuard dg(languageHook); file->Close(); }

#ifndef SWIG
      inline const XFILE::CFile* getFile() const { return file; }
#endif

    };
    //@}
  }
}

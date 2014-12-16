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

    /**
     * File class.\n
     * \n
     * 'w' - opt open for write\n
     * example:\n
     *  f = xbmcvfs.File(file, ['w'])\n
     */
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

      /**
       * read(bytes)\n
       * \n
       * bytes : how many bytes to read [opt]- if not set it will read the whole file\n
       *\n
       * returns: string\n
       * \n
       * example:\n
       *  f = xbmcvfs.File(file)\n
       *  b = f.read()\n
       *  f.close()\n
       */
      inline String read(unsigned long numBytes = 0) 
      { 
        XbmcCommons::Buffer b = readBytes(numBytes);
        return b.getString(numBytes == 0 ? b.remaining() : std::min((unsigned long)b.remaining(),numBytes));
      }

      /**
       * readBytes(numbytes)\n
       * \n
       * numbytes : how many bytes to read [opt]- if not set it will read the whole file\n
       *\n
       * returns: bytearray\n
       * \n
       * example:\n
       *  f = xbmcvfs.File(file)\n
       *  b = f.read()\n
       *  f.close()\n
       */
      XbmcCommons::Buffer readBytes(unsigned long numBytes = 0);

      /**
       * write(buffer)\n
       * \n
       * buffer : buffer to write to file\n
       *\n
       * returns: true on success.\n
       * \n
       * example:\n
       *  f = xbmcvfs.File(file, 'w', True)\n
       *  result = f.write(buffer)\n
       *  f.close()\n
       */
      bool write(XbmcCommons::Buffer& buffer);

      /**
       * size()\n
       * \n
       * example:\n
       *  f = xbmcvfs.File(file)\n
       *  s = f.size()\n
       *  f.close()\n
       */
      inline long long size() { DelayedCallGuard dg(languageHook); return file->GetLength(); }

      /**
       * seek()\n
       * \n
       * FilePosition : position in the file\n
       * Whence : where in a file to seek from[0 begining, 1 current , 2 end possition]\n
       * example:\n
       *  f = xbmcvfs.File(file)\n
       *  result = f.seek(8129, 0)\n
       *  f.close()\n
       */
      inline long long seek(long long seekBytes, int iWhence) { DelayedCallGuard dg(languageHook); return file->Seek(seekBytes,iWhence); }

      /**
       * close()\n
       * \n
       * example:\n
       *  f = xbmcvfs.File(file)\n
       *  f.close()\n
       */
      inline void close() { DelayedCallGuard dg(languageHook); file->Close(); }

#ifndef SWIG
      inline const XFILE::CFile* getFile() const { return file; }
#endif

    };
  }
}

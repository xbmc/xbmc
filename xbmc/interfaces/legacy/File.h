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

#include "filesystem/File.h"
#include "AddonString.h"
#include "AddonClass.h"
#include "LanguageHook.h"

namespace XBMCAddon
{

  namespace xbmcvfs
  {

    /**
     * <pre>
     * File class.
     * 
     * 'w' - opt open for write
     * example:
     *  f = xbmcvfs.File(file, ['w'])
     * </pre>
     */
    class File : public AddonClass
    {
      XFILE::CFile* file;
    public:
      inline File(const String& filepath, const char* mode = NULL) : AddonClass("File"), file(new XFILE::CFile())
      {
        DelayedCallGuard dg(languageHook);
        if (mode && strncmp(mode, "w", 1) == 0)
          file->OpenForWrite(filepath,true);
        else
          file->Open(filepath, READ_NO_CACHE);
      }

      inline ~File() { delete file; }

#ifndef SWIG
      /**
       * <pre>
       * read(bytes)
       * 
       * bytes : how many bytes to read [opt]- if not set it will read the whole fi
       * 
       * example:
       *  f = xbmcvfs.File(file)
       *  b = f.read()
       *  f.close()
       * </pre>
       */
      unsigned long read(void* buffer, unsigned long numBytes = 0);
#endif

      /**
       * <pre>
       * write(buffer)
       * 
       * buffer : buffer to write to fi
       * 
       * example:
       *  f = xbmcvfs.File(file, 'w', True)
       *  result = f.write(buffer)
       *  f.close()
       * </pre>
       */
      bool write(const char* file);

      /**
       * <pre>
       * size()
       * 
       * example:
       *  f = xbmcvfs.File(file)
       *  s = f.size()
       *  f.close()
       * </pre>
       */
      inline long long size() { DelayedCallGuard dg(languageHook); return file->GetLength(); }

      /**
       * <pre>
       * seek()
       * 
       * FilePosition : position in the file
       * Whence : where in a file to seek from[0 begining, 1 current , 2 end possition]
       * example:
       *  f = xbmcvfs.File(file)
       *  result = f.seek(8129, 0)
       *  f.close()
       * </pre>
       */
      inline long long seek(long long seekBytes, int iWhence) { DelayedCallGuard dg(languageHook); return file->Seek(seekBytes,iWhence); }

      /**
       * <pre>
       * close()
       * 
       * example:
       *  f = xbmcvfs.File(file)
       *  f.close()
       * </pre>
       */
      inline void close() { DelayedCallGuard dg(languageHook); file->Close(); }

#ifndef SWIG
      inline const XFILE::CFile* getFile() const { return file; }
#endif

    };
  }
}

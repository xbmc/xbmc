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
#include "AddonClass.h"
#include "LanguageHook.h"

namespace XBMCAddon
{
  namespace xbmcvfs
  {
    /**
     * Stat(path) -- Get file or file system status.
     * 
     * path        : file or folder
     * 
     * example:
     *   st = xbmcvfs.Stat(path)
     *   modified = st.st_mtime()
     */
    class Stat : public AddonClass
    {
      struct __stat64 st;
      
    public:
      Stat(const String& path) : AddonClass("Stat")
      {
        DelayedCallGuard dg;
        XFILE::CFile::Stat(path, &st);
      }
      
      inline long long st_mode() { return st.st_mode; };
      inline long long st_ino() { return st.st_ino; };
      inline long long st_dev() { return st.st_dev; };
      inline long long st_nlink() { return st.st_nlink; };
      inline long long st_uid() { return st.st_uid; };
      inline long long st_gid() { return st.st_gid; };
      inline long long st_size() { return st.st_size; };
      inline long long atime() { return st.st_atime; }; //names st_atime/st_mtime/st_ctime are used by sys/stat.h
      inline long long mtime() { return st.st_mtime; };
      inline long long ctime() { return st.st_ctime; };
    };
  }
}


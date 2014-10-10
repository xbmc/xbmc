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

#ifndef LIBRARY_LOADER
#define LIBRARY_LOADER

#include <string>

#include "system.h"
#ifdef TARGET_POSIX
#include "PlatformDefs.h"
#endif

class LibraryLoader
{
public:
  LibraryLoader(const std::string& libraryFile);
  virtual ~LibraryLoader();

  virtual bool Load() = 0;
  virtual void Unload() = 0;

  virtual int ResolveExport(const char* symbol, void** ptr, bool logging = true) = 0;
  virtual int ResolveOrdinal(unsigned long ordinal, void** ptr);
  virtual bool IsSystemDll() = 0;
  virtual HMODULE GetHModule() = 0;
  virtual bool HasSymbols() = 0;

  const char *GetName() const; // eg "mplayer.dll"
  const char *GetFileName() const; // "special://xbmcbin/system/mplayer/players/mplayer.dll"
  const char *GetPath() const; // "special://xbmcbin/system/mplayer/players/"

  int IncrRef();
  int DecrRef();
  int GetRef();

private:
  LibraryLoader(const LibraryLoader&);
  LibraryLoader& operator=(const LibraryLoader&);
  std::string m_fileName;
  std::string m_path;
  int   m_iRefCount;
};

#endif

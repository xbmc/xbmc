/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

#ifdef TARGET_POSIX
#include "PlatformDefs.h"
#endif

class LibraryLoader
{
public:
  explicit LibraryLoader(const std::string& libraryFile);
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

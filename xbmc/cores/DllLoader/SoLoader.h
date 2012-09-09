#ifndef SO_LOADER
#define SO_LOADER

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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include "system.h"
#ifdef _LINUX
#include "PlatformDefs.h"
#endif
#include "DllLoader.h"

class SoLoader : public LibraryLoader
{
public:
  SoLoader(const char *so, bool bGlobal = false);
  ~SoLoader();

  virtual bool Load();
  virtual void Unload();

  virtual int ResolveExport(const char* symbol, void** ptr, bool logging = true);
  virtual bool IsSystemDll();
  virtual HMODULE GetHModule();
  virtual bool HasSymbols();

private:
  void* m_soHandle;
  bool m_bGlobal;
  bool m_bLoaded;
};

#endif

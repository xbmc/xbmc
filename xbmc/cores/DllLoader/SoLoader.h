#ifndef SO_LOADER
#define SO_LOADER

/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#ifdef TARGET_POSIX
#include "PlatformDefs.h"
#endif
#include "DllLoader.h"

class SoLoader : public LibraryLoader
{
public:
  SoLoader(const std::string &so, bool bGlobal = false);
  ~SoLoader() override;

  bool Load() override;
  void Unload() override;

  int ResolveExport(const char* symbol, void** ptr, bool logging = true) override;
  bool IsSystemDll() override;
  HMODULE GetHModule() override;
  bool HasSymbols() override;

private:
  void* m_soHandle;
  bool m_bGlobal;
  bool m_bLoaded;
};

#endif

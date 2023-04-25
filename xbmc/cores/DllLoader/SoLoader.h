/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "LibraryLoader.h"

#include <stdio.h>
#ifdef TARGET_POSIX
#include "PlatformDefs.h"
#endif

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

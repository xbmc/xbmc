/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "LibraryLoader.h"

#include <vector>

class Win32DllLoader : public LibraryLoader
{
public:
  class Import
  {
  public:
    void *table;
    uintptr_t function;
  };

  Win32DllLoader(const std::string& dll, bool isSystemDll);
  ~Win32DllLoader();

  virtual bool Load();
  virtual void Unload();

  virtual int ResolveExport(const char* symbol, void** ptr, bool logging = true);
  virtual bool IsSystemDll();
  virtual HMODULE GetHModule();
  virtual bool HasSymbols();

private:
  void OverrideImports(const std::string &dll);
  void RestoreImports();
  static bool ResolveImport(const char *dllName, const char *functionName, void **fixup);
  static bool ResolveOrdinal(const char *dllName, unsigned long ordinal, void **fixup);
  bool NeedsHooking(const char *dllName);

  HMODULE m_dllHandle;
  bool bIsSystemDll;

  std::vector<Import> m_overriddenImports;
  std::vector<HMODULE> m_referencedDlls;
};


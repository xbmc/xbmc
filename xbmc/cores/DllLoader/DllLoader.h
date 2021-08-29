/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "coffldr.h"
#include "LibraryLoader.h"

// clang-format off
#if defined(__linux__) && \
    !defined(__aarch64__) && \
    !defined(__arc__) && \
    !defined(__arm__) && \
    !defined(__mips__) && \
    !defined(__powerpc__) && \
    !defined(__or1k__) && \
    !defined(__riscv) && \
    !defined(__SH4__) && \
    !defined(__sparc__) && \
    !defined(__xtensa__)
#define USE_LDT_KEEPER
#include "ldt_keeper.h"
#endif
// clang-format on

#ifndef NULL
#define NULL 0
#endif

class DllLoader;


typedef struct Export
{
  const char*   name;
  unsigned long ordinal;
  void*         function;
  void*         track_function;
} Export;

typedef struct ExportEntry
{
  Export exp;
  ExportEntry* next;
} ExportEntry;

typedef struct _LoadedList
{
  DllLoader* pDll;
  _LoadedList* pNext;
} LoadedList;

class DllLoader : public CoffLoader, public LibraryLoader
{
public:
  DllLoader(const char *dll, bool track = false, bool bSystemDll = false, bool bLoadSymbols = false, Export* exports = NULL);
  ~DllLoader() override;

  bool Load() override;
  void Unload() override;

  int ResolveExport(const char*, void** ptr, bool logging = true) override;
  int ResolveOrdinal(unsigned long ordinal, void** ptr) override;
  bool HasSymbols() override { return m_bLoadSymbols && !m_bUnloadSymbols; }
  bool IsSystemDll() override { return m_bSystemDll; }
  HMODULE GetHModule() override { return (HMODULE)hModule; }

  Export* GetExportByFunctionName(const char* sFunctionName);
  Export* GetExportByOrdinal(unsigned long ordinal);
protected:
  int Parse();
  int ResolveImports();

  void AddExport(unsigned long ordinal, void* function, void* track_function = NULL);
  void AddExport(char* sFunctionName, unsigned long ordinal, void* function, void* track_function = NULL);
  void AddExport(char* sFunctionName, void* function, void* track_function = NULL);
  void SetExports(Export* exports) { m_pStaticExports = exports; }

protected:
  // Just pointers; dont' delete...
  ImportDirTable_t *ImportDirTable;
  ExportDirTable_t *ExportDirTable;
  bool m_bTrack;
  bool m_bSystemDll; // true if this dll should not be removed
  bool m_bLoadSymbols; // when true this dll should not be removed
  bool m_bUnloadSymbols;
  ExportEntry* m_pExportHead;
  Export* m_pStaticExports;
  LoadedList* m_pDlls;

#ifdef USE_LDT_KEEPER
  ldt_fs_t* m_ldt_fs;
#endif

  void PrintImportLookupTable(unsigned long ImportLookupTable_RVA);
  void PrintImportTable(ImportDirTable_t *ImportDirTable);
  void PrintExportTable(ExportDirTable_t *ExportDirTable);

  int ResolveOrdinal(const char*, unsigned long, void**);
  int ResolveName(const char*, char*, void **);
  const char* ResolveReferencedDll(const char* dll);
  int LoadExports();
  void LoadSymbols();
  static void UnloadSymbols();
};

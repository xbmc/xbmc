/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DllLoader.h"
#include "DllLoaderContainer.h"

CoffLoader::CoffLoader() :
  hModule           (NULL ),
  CoffFileHeader    (NULL ),
  OptionHeader      (NULL ),
  WindowsHeader     (NULL ),
  Directory         (NULL ),
  SectionHeader     (NULL ),
  SymTable          (NULL ),
  StringTable       (NULL ),
  SectionData       (NULL ),
  EntryAddress      (0    ),
  NumberOfSymbols   (0    ),
  SizeOfStringTable (0    ),
  NumOfDirectories  (0    ),
  NumOfSections     (0    ),
  FileHeaderOffset  (0    )
{
}

CoffLoader::~CoffLoader()
{
}

DllLoaderContainer::DllLoaderContainer()
{
}

DllLoader* DllLoaderContainer::LoadModule(const char* sName, const char* sCurrentDir, bool bLoadSymbols)
{
  return NULL;
}

bool DllLoader::Load()
{
  return false;
}

void DllLoader::Unload()
{
}

void DllLoaderContainer::ReleaseModule(DllLoader*& pDll)
{
}

DllLoader::DllLoader(const char *dll, bool track, bool bSystemDll, bool bLoadSymbols, Export* exp)
{
}

DllLoader::~DllLoader()
{
}

int DllLoader::ResolveExport(const char* x, void** y)
{
}

DllLoaderContainer g_dlls;

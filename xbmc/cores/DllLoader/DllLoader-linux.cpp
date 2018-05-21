/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
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

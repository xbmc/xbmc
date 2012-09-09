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

#include "system.h"
#include "DllLoader.h"
#include "DllLoaderContainer.h"

CoffLoader::CoffLoader()
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

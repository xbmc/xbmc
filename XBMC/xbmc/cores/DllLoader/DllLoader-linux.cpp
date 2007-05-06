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

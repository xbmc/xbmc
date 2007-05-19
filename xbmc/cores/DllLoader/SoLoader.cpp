#include <dlfcn.h>
#include "SoLoader.h"
#include "StdString.h"
#include "Util.h"
#include "utils/log.h"

SoLoader::SoLoader(const char *so) : LibraryLoader(so)
{
  m_soHandle = NULL;
}

SoLoader::~SoLoader()
{
  Unload();
}

bool SoLoader::Load()
{
  if (m_soHandle != NULL)
    return true;
    
  CStdString strFileName= _P(GetFileName());
  printf("Load: %s\n", strFileName.c_str());
  m_soHandle = dlopen(strFileName.c_str(), RTLD_LAZY);
  if (!m_soHandle)
  {
    CLog::Log(LOGERROR, "Unable to load %s, reason: %s", strFileName.c_str(), dlerror());
    return false;
  }
  
  return true;  
}

void SoLoader::Unload()
{
  printf("Unload: %s\n", GetName());

  if (m_soHandle)
    dlclose(m_soHandle);
    
  m_soHandle = NULL;
}
  
int SoLoader::ResolveExport(const char* symbol, void** f)
{
  printf("Resolve: %s %s\n", GetName(), symbol);

  if (!m_soHandle && !Load())
  {
    CLog::Log(LOGWARNING, "Unable to resolve: %s %s, reason: so not loaded", GetName(), symbol);
    return 0;
  }
    
  void* s = dlsym(m_soHandle, symbol);
  if (!s)
  {
    CLog::Log(LOGWARNING, "Unable to resolve: %s %s, reason: %s", GetName(), symbol, dlerror());
    return 0;
  }
    
  *f = s;
  return 1;
}

bool SoLoader::IsSystemDll()
{
  return false;
}

HMODULE SoLoader::GetHModule()
{
  return m_soHandle;
}

bool SoLoader::HasSymbols()
{
  return false;
}  

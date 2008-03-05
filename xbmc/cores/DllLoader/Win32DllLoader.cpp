#include "stdafx.h"
#include "Win32DllLoader.h"
#include "StdString.h"
#include "Util.h"
#include "utils/log.h"

Win32DllLoader::Win32DllLoader(const char *dll) : LibraryLoader(dll)
{
  m_dllHandle = NULL;
}

Win32DllLoader::~Win32DllLoader()
{
  if (m_dllHandle)
    Unload();
}

bool Win32DllLoader::Load()
{
  if (m_dllHandle != NULL)
    return true;
    
  CStdString strFileName= _P(GetFileName());
  CLog::Log(LOGDEBUG, "NativeDllLoader: Loading: %s\n", strFileName.c_str());  
  //int flags = RTLD_LAZY;
  //if (m_bGlobal) flags |= RTLD_GLOBAL;
  //m_soHandle = dlopen(strFileName.c_str(), flags);
  m_dllHandle = LoadLibrary(strFileName.c_str());
  if (!m_dllHandle)
  {
    CLog::Log(LOGERROR, "NativeDllLoader: Unable to load %s", strFileName.c_str());
    return false;
  }
  
  return true;  
}

void Win32DllLoader::Unload()
{
  CLog::Log(LOGDEBUG, "NativeDllLoader: Unloading: %s\n", GetName());

  if (m_dllHandle)
  {
    if (!FreeLibrary(m_dllHandle))
       CLog::Log(LOGERROR, "NativeDllLoader: Unable to unload %s", GetName());
  }
    
  m_dllHandle = NULL;
}
  
int Win32DllLoader::ResolveExport(const char* symbol, void** f)
{
  if (!m_dllHandle && !Load())
  {
    CLog::Log(LOGWARNING, "NativeDllLoader: Unable to resolve: %s %s, reason: DLL not loaded", GetName(), symbol);
    return 0;
  }
    
  void *s = GetProcAddress(m_dllHandle, symbol);

  if (!s)
  {
    CLog::Log(LOGWARNING, "NativeDllLoader: Unable to resolve: %s %s", GetName(), symbol);
    return 0;
  }
    
  *f = s;
  return 1;
}

bool Win32DllLoader::IsSystemDll()
{
  return false;
}

HMODULE Win32DllLoader::GetHModule()
{
  return m_dllHandle;
}

bool Win32DllLoader::HasSymbols()
{
  return false;
}  
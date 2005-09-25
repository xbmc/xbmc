
#include "../../stdafx.h"
#include "dll.h"
#include "DllLoader.h"
#include "DllLoaderContainer.h"

#define DEFAULT_DLLPATH "Q:\\system\\players\\mplayer\\codecs"

char* getpath(char *buf, const char *full)
{
  char* pos;
  if (pos = strrchr(full, '\\'))
  {
    strncpy(buf, full, pos - full);
    buf[pos - full] = 0;
    return buf;
  }
  else return NULL;
}

extern "C" HMODULE __stdcall dllLoadLibraryExtended(LPCSTR file, LPCSTR sourcedll)
{
  // we skip to the last backslash
  // this is effectively eliminating weird characters in
  // the text output windows

  char* libname = strrchr(file, '\\');
  if (libname) libname++;
  else libname = (char*)file;

  char llibname[MAX_PATH + 1]; // lower case
  strcpy(llibname, libname);
  strlwr(llibname);

  CLog::Log(LOGDEBUG, "LoadLibraryA('%s')", libname);
  char* l = llibname;
  
    char pfile[MAX_PATH + 1];
  if (strlen(file) > 1 && file[1] == ':')
    sprintf(pfile, "%s", (char *)file);
  else
  {
    if (sourcedll)
    {
      //Use calling dll's path as base address for this call
      char path[MAX_PATH + 1];
      getpath(path, sourcedll);

      //Handle mplayer case specially
      //it has all it's dlls in a codecs subdirectory
      if (strstr(sourcedll, "mplayer.dll"))
        sprintf(pfile, "%s\\codecs\\%s", path, (char *)libname);
      else
        sprintf(pfile, "%s\\%s", path, (char *)libname);
    }
    else
      sprintf(pfile, "%s\\%s", DEFAULT_DLLPATH , (char *)libname);
  }
  
  // Check if dll is already loaded and return its handle
  
  // first try filename only
  
  DllLoader* dll = g_dlls.LoadModule(file);
  // now with base address
  if (!dll) dll = g_dlls.LoadModule(pfile);
  
  if (dll)
  {
    CLog::Log(LOGDEBUG, "LoadLibrary('%s') returning: 0x%x", libname, dll);
    return (HMODULE)dll;
  }

  CLog::Log(LOGERROR, "LoadLibrary('%s') failed", libname);
  return NULL;
}

extern "C" HMODULE __stdcall dllLoadLibraryA(LPCSTR file)
{
  return dllLoadLibraryExtended(file, NULL);
}

#define DONT_RESOLVE_DLL_REFERENCES   0x00000001
#define LOAD_LIBRARY_AS_DATAFILE      0x00000002
#define LOAD_WITH_ALTERED_SEARCH_PATH 0x00000008
#define LOAD_IGNORE_CODE_AUTHZ_LEVEL  0x00000010

extern "C" HMODULE __stdcall dllLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
  char strFlags[64];
  strFlags[0] = '\0';

  if (dwFlags & DONT_RESOLVE_DLL_REFERENCES) strcat(strFlags, "\n - DONT_RESOLVE_DLL_REFERENCES");
  if (dwFlags & LOAD_IGNORE_CODE_AUTHZ_LEVEL) strcat(strFlags, "\n - LOAD_IGNORE_CODE_AUTHZ_LEVEL");
  if (dwFlags & LOAD_LIBRARY_AS_DATAFILE) strcat(strFlags, "\n - LOAD_LIBRARY_AS_DATAFILE");
  if (dwFlags & LOAD_WITH_ALTERED_SEARCH_PATH) strcat(strFlags, "\n - LOAD_WITH_ALTERED_SEARCH_PATH");

  CLog::Log(LOGDEBUG, "LoadLibraryExA called with flags: %s", strFlags);
  
  return dllLoadLibraryExtended(lpLibFileName, NULL);
}

extern "C" BOOL __stdcall dllFreeLibrary(HINSTANCE hLibModule)
{
  DllLoader* dllhandle = (DllLoader*)hLibModule;
  
  // to make sure systems dlls are never deleted
  if (dllhandle->IsSystemDll()) return 1;
  
  CLog::Log(LOGDEBUG, "FreeLibrary(%s) -> 0x%x", dllhandle->GetName(), dllhandle);

  g_dlls.ReleaseModule(dllhandle);

  return 1;
}

extern "C" FARPROC __stdcall dllGetProcAddress(HMODULE hModule, LPCSTR function)
{
  void* address = NULL;
  
  DllLoader* dll = (DllLoader*)hModule;
  dll->ResolveExport(function, &address);

  CLog::Log(LOGDEBUG, "%s!GetProcAddress(0x%x, '%s') => 0x%x", dll->GetName(), hModule, function, address);
  return (FARPROC)address;
}

extern "C" HMODULE WINAPI dllGetModuleHandleA(LPCSTR lpModuleName)
{
  /*
  If the file name extension is omitted, the default library extension .dll is appended.
  The file name string can include a trailing point character (.) to indicate that the module name has no extension.
  The string does not have to specify a path. When specifying a path, be sure to use backslashes (\), not forward slashes (/).
  The name is compared (case independently)
  If this parameter is NULL, GetModuleHandle returns a handle to the file used to create the calling process (.exe file).
  */
  char* strModuleName = new char[strlen(lpModuleName) + 5];
  strcpy(strModuleName, lpModuleName);

  if (strrchr(strModuleName, '.') == 0) strcat(strModuleName, ".dll");

  CLog::Log(LOGDEBUG, "GetModuleHandleA(%s) .. looking up", lpModuleName);

  HMODULE h = g_dlls.GetModuleAddress(strModuleName);
  if (h)
  {
    CLog::Log(LOGDEBUG, "GetModuleHandleA('%s') => 0x%x", lpModuleName, h);
    return h;
  }
 
  delete []strModuleName;

  return NULL;
}

extern "C" DWORD WINAPI dllGetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize)
{
  if (NULL == hModule)
  {
    strncpy(lpFilename, "xbmc.xbe", nSize);
    CLog::Log(LOGDEBUG, "GetModuleFileNameA(0x%x, 0x%x, %d) => '%s'\n",
              hModule, lpFilename, nSize, lpFilename);
    return 1;
  }

  char* sName = ((DllLoader*)hModule)->GetFileName();
  if (sName)
  {
    strncpy(lpFilename, sName, nSize);
    return 1;
  }
  
  return 0;
}
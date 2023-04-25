/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "dll.h"

#include "DllLoaderContainer.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <climits>

#define DEFAULT_DLLPATH "special://xbmc/system/players/mplayer/codecs/"
#define HIGH_WORD(a) ((uintptr_t)(a) >> 16)
#define LOW_WORD(a) ((unsigned short)(((uintptr_t)(a)) & MAXWORD))

//#define API_DEBUG

char* getpath(char *buf, const char *full)
{
  const char* pos;
  if ((pos = strrchr(full, PATH_SEPARATOR_CHAR)))
  {
    strncpy(buf, full, pos - full + 1 );
    buf[pos - full + 1] = 0;
    return buf;
  }
  else
  {
    buf[0] = 0;
    return buf;
  }
}

extern "C" HMODULE __stdcall dllLoadLibraryExtended(const char* lib_file, const char* sourcedll)
{
  char libname[MAX_PATH + 1] = {};
  char libpath[MAX_PATH + 1] = {};
  LibraryLoader* dll = NULL;

  /* extract name */
  const char* p = strrchr(lib_file, PATH_SEPARATOR_CHAR);
  if (p)
    strncpy(libname, p+1, sizeof(libname) - 1);
  else
    strncpy(libname, lib_file, sizeof(libname) - 1);
  libname[sizeof(libname) - 1] = '\0';

  if( libname[0] == '\0' )
    return NULL;

  /* extract path */
  getpath(libpath, lib_file);

  if (sourcedll)
  {
    /* also check for invalid paths which begin with a \ */
    if( libpath[0] == '\0' || libpath[0] == PATH_SEPARATOR_CHAR )
    {
      /* use calling dll's path as base address for this call */
      getpath(libpath, sourcedll);

      /* mplayer has all it's dlls in a codecs subdirectory */
      if (strstr(sourcedll, "mplayer.dll"))
        strcat(libpath, "codecs\\");
    }
  }

  /* if we still don't have a path, use default path */
  if( libpath[0] == '\0' )
    strcpy(libpath, DEFAULT_DLLPATH);

  /* msdn docs state */
  /* "If no file name extension is specified in the lpFileName parameter, the default library extension .dll is appended.  */
  /* However, the file name string can include a trailing point character (.) to indicate that the module name has no extension." */
  if( strrchr(libname, '.') == NULL )
    strcat(libname, ".dll");
  else if( libname[strlen(libname)-1] == '.' )
    libname[strlen(libname)-1] = '\0';

  dll = DllLoaderContainer::LoadModule(libname, libpath);

  if (dll)
    return (HMODULE)dll->GetHModule();

  CLog::Log(LOGERROR, "LoadLibrary('{}') failed", libname);
  return NULL;
}

extern "C" HMODULE __stdcall dllLoadLibraryA(const char* file)
{
  return dllLoadLibraryExtended(file, NULL);
}

#define DONT_RESOLVE_DLL_REFERENCES   0x00000001
#define LOAD_LIBRARY_AS_DATAFILE      0x00000002
#define LOAD_WITH_ALTERED_SEARCH_PATH 0x00000008
#define LOAD_IGNORE_CODE_AUTHZ_LEVEL  0x00000010

extern "C" HMODULE __stdcall dllLoadLibraryExExtended(const char* lpLibFileName, HANDLE hFile, DWORD dwFlags, const char* sourcedll)
{
  char strFlags[512];
  strFlags[0] = '\0';

  if (dwFlags & DONT_RESOLVE_DLL_REFERENCES) strcat(strFlags, "\n - DONT_RESOLVE_DLL_REFERENCES");
  if (dwFlags & LOAD_IGNORE_CODE_AUTHZ_LEVEL) strcat(strFlags, "\n - LOAD_IGNORE_CODE_AUTHZ_LEVEL");
  if (dwFlags & LOAD_LIBRARY_AS_DATAFILE) strcat(strFlags, "\n - LOAD_LIBRARY_AS_DATAFILE");
  if (dwFlags & LOAD_WITH_ALTERED_SEARCH_PATH) strcat(strFlags, "\n - LOAD_WITH_ALTERED_SEARCH_PATH");

  CLog::Log(LOGDEBUG, "LoadLibraryExA called with flags: {}", strFlags);

  return dllLoadLibraryExtended(lpLibFileName, sourcedll);
}

extern "C" HMODULE __stdcall dllLoadLibraryExA(const char* lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
  return dllLoadLibraryExExtended(lpLibFileName, hFile, dwFlags, NULL);
}

extern "C" int __stdcall dllFreeLibrary(HINSTANCE hLibModule)
{
  LibraryLoader* dllhandle = DllLoaderContainer::GetModule(hLibModule);

  if( !dllhandle )
  {
    CLog::Log(LOGERROR, "{} - Invalid hModule specified", __FUNCTION__);
    return 1;
  }

  // to make sure systems dlls are never deleted
  if (dllhandle->IsSystemDll()) return 1;

  DllLoaderContainer::ReleaseModule(dllhandle);

  return 1;
}

extern "C" HMODULE WINAPI dllGetModuleHandleA(const char* lpModuleName)
{
  /*
  If the file name extension is omitted, the default library extension .dll is appended.
  The file name string can include a trailing point character (.) to indicate that the module name has no extension.
  The string does not have to specify a path. When specifying a path, be sure to use backslashes (\), not forward slashes (/).
  The name is compared (case independently)
  If this parameter is NULL, GetModuleHandle returns a handle to the file used to create the calling process (.exe file).
  */

  if( lpModuleName == NULL )
    return NULL;

  char* strModuleName = new char[strlen(lpModuleName) + 5];
  strcpy(strModuleName, lpModuleName);

  if (strrchr(strModuleName, '.') == 0) strcat(strModuleName, ".dll");

  //CLog::Log(LOGDEBUG, "GetModuleHandleA({}) .. looking up", lpModuleName);

  LibraryLoader *p = DllLoaderContainer::GetModule(strModuleName);
  delete []strModuleName;

  if (p)
  {
    //CLog::Log(LOGDEBUG, "GetModuleHandleA('{}') => 0x{:x}", lpModuleName, h);
    return (HMODULE)p->GetHModule();
  }

  CLog::Log(LOGDEBUG, "GetModuleHandleA('{}') failed", lpModuleName);
  return NULL;
}

/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "dll.h"

#include "DllLoader.h"
#include "DllLoaderContainer.h"
#include "dll_tracker.h"
#include "dll_util.h"
#include "filesystem/SpecialProtocol.h"
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
    /* also check for invalid paths wich begin with a \ */
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

  CLog::Log(LOGERROR, "LoadLibrary('%s') failed", libname);
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

  CLog::Log(LOGDEBUG, "LoadLibraryExA called with flags: %s", strFlags);

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
    CLog::Log(LOGERROR, "%s - Invalid hModule specified",__FUNCTION__);
    return 1;
  }

  // to make sure systems dlls are never deleted
  if (dllhandle->IsSystemDll()) return 1;

  DllLoaderContainer::ReleaseModule(dllhandle);

  return 1;
}

extern "C" intptr_t (*__stdcall dllGetProcAddress(HMODULE hModule, const char* function))(void)
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();

  void* address = NULL;
  LibraryLoader* dll = DllLoaderContainer::GetModule(hModule);

  if( !dll )
  {
    CLog::Log(LOGERROR, "%s - Invalid hModule specified",__FUNCTION__);
    return NULL;
  }

  /* how can somebody get the stupid idea to create such a stupid function */
  /* where you never know if the given pointer is a pointer or a value */
  if( HIGH_WORD(function) == 0 && LOW_WORD(function) < 1000)
  {
    if( dll->ResolveOrdinal(LOW_WORD(function), &address) )
    {
      CLog::Log(LOGDEBUG, "%s(%p(%s), %d) => %p", __FUNCTION__, static_cast<void*>(hModule), dll->GetName(), LOW_WORD(function), address);
    }
    else if( dll->IsSystemDll() )
    {
      char ordinal[6] = {};
      sprintf(ordinal, "%u", LOW_WORD(function));
      address = (void*)create_dummy_function(dll->GetName(), ordinal);

      /* add to tracklist if we are tracking this source dll */
      DllTrackInfo* track = tracker_get_dlltrackinfo(loc);
      if( track )
        tracker_dll_data_track(track->pDll, (uintptr_t)address);

      CLog::Log(LOGDEBUG, "%s - created dummy function %s!%s",__FUNCTION__, dll->GetName(), ordinal);
    }
    else
    {
      address = NULL;
      CLog::Log(LOGDEBUG, "%s(%p(%s), '%s') => %p",__FUNCTION__ , static_cast<void*>(hModule), dll->GetName(), function, address);
    }
  }
  else
  {
    if( dll->ResolveExport(function, &address) )
    {
      CLog::Log(LOGDEBUG, "%s(%p(%s), '%s') => %p",__FUNCTION__ , static_cast<void*>(hModule), dll->GetName(), function, address);
    }
    else
    {
      DllTrackInfo* track = tracker_get_dlltrackinfo(loc);
      /* some dll's require us to always return a function or it will fail, other's  */
      /* decide functionality depending on if the functions exist and may fail      */
      if (dll->IsSystemDll() && track &&
          StringUtils::CompareNoCase(track->pDll->GetName(), "CoreAVCDecoder.ax") == 0)
      {
        address = (void*)create_dummy_function(dll->GetName(), function);
        tracker_dll_data_track(track->pDll, (uintptr_t)address);
        CLog::Log(LOGDEBUG, "%s - created dummy function %s!%s", __FUNCTION__, dll->GetName(), function);
      }
      else
      {
        address = NULL;
        CLog::Log(LOGDEBUG, "%s(%p(%s), '%s') => %p", __FUNCTION__, static_cast<void*>(hModule), dll->GetName(), function, address);
      }
    }
  }

  return (intptr_t(*)(void)) address;
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

  //CLog::Log(LOGDEBUG, "GetModuleHandleA(%s) .. looking up", lpModuleName);

  LibraryLoader *p = DllLoaderContainer::GetModule(strModuleName);
  delete []strModuleName;

  if (p)
  {
    //CLog::Log(LOGDEBUG, "GetModuleHandleA('%s') => 0x%x", lpModuleName, h);
    return (HMODULE)p->GetHModule();
  }

  CLog::Log(LOGDEBUG, "GetModuleHandleA('%s') failed", lpModuleName);
  return NULL;
}

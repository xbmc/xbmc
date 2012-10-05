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

#include "dll.h"
#include "DllLoader.h"
#include "DllLoaderContainer.h"
#include "dll_tracker.h"
#include "dll_util.h"
#include <climits>
#include "filesystem/SpecialProtocol.h"
#include "utils/log.h"

#define DEFAULT_DLLPATH "special://xbmc/system/players/mplayer/codecs/"
#define HIGH_WORD(a) ((uintptr_t)(a) >> 16)
#define LOW_WORD(a) ((WORD)(((uintptr_t)(a)) & MAXWORD))

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

extern "C" HMODULE __stdcall dllLoadLibraryExtended(LPCSTR lib_file, LPCSTR sourcedll)
{
  char libname[MAX_PATH + 1] = {};
  char libpath[MAX_PATH + 1] = {};
  LibraryLoader* dll = NULL;

  /* extract name */
  const char* p = strrchr(lib_file, PATH_SEPARATOR_CHAR);
  if (p)
    strcpy(libname, p+1);
  else
    strcpy(libname, lib_file);

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

extern "C" HMODULE __stdcall dllLoadLibraryA(LPCSTR file)
{
  return dllLoadLibraryExtended(file, NULL);
}

#define DONT_RESOLVE_DLL_REFERENCES   0x00000001
#define LOAD_LIBRARY_AS_DATAFILE      0x00000002
#define LOAD_WITH_ALTERED_SEARCH_PATH 0x00000008
#define LOAD_IGNORE_CODE_AUTHZ_LEVEL  0x00000010

extern "C" HMODULE __stdcall dllLoadLibraryExExtended(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags, LPCSTR sourcedll)
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

extern "C" HMODULE __stdcall dllLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
  return dllLoadLibraryExExtended(lpLibFileName, hFile, dwFlags, NULL);
}

extern "C" BOOL __stdcall dllFreeLibrary(HINSTANCE hLibModule)
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

extern "C" FARPROC __stdcall dllGetProcAddress(HMODULE hModule, LPCSTR function)
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
      CLog::Log(LOGDEBUG, "%s(%p(%s), %d) => %p", __FUNCTION__, hModule, dll->GetName(), LOW_WORD(function), address);
    }
    else if( dll->IsSystemDll() )
    {
      char ordinal[5];
      sprintf(ordinal, "%d", LOW_WORD(function));
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
      CLog::Log(LOGDEBUG, "%s(%p(%s), '%s') => %p",__FUNCTION__ , hModule, dll->GetName(), function, address);
    }
  }
  else
  {
    if( dll->ResolveExport(function, &address) )
    {
      CLog::Log(LOGDEBUG, "%s(%p(%s), '%s') => %p",__FUNCTION__ , hModule, dll->GetName(), function, address);
    }
    else
    {
      DllTrackInfo* track = tracker_get_dlltrackinfo(loc);
      /* some dll's require us to always return a function or it will fail, other's  */
      /* decide functionallity depending on if the functions exist and may fail      */
      if( dll->IsSystemDll() && track
       && stricmp(track->pDll->GetName(), "CoreAVCDecoder.ax") == 0 )
      {
        address = (void*)create_dummy_function(dll->GetName(), function);
        tracker_dll_data_track(track->pDll, (uintptr_t)address);
        CLog::Log(LOGDEBUG, "%s - created dummy function %s!%s", __FUNCTION__, dll->GetName(), function);
      }
      else
      {
        address = NULL;
        CLog::Log(LOGDEBUG, "%s(%p(%s), '%s') => %p", __FUNCTION__, hModule, dll->GetName(), function, address);
      }
    }
  }

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

extern "C" DWORD WINAPI dllGetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize)
{
  if (NULL == hModule)
  {
#ifdef _WIN32
    return GetModuleFileNameA(hModule, lpFilename, nSize);
#else
    CLog::Log(LOGDEBUG, "%s - No hModule specified", __FUNCTION__);
    return 0;
#endif
  }

  LibraryLoader* dll = DllLoaderContainer::GetModule(hModule);
  if( !dll )
  {
    CLog::Log(LOGERROR, "%s - Invalid hModule specified", __FUNCTION__);
    return 0;
  }

  char* sName = dll->GetFileName();
  if (sName)
  {
    strncpy(lpFilename, sName, nSize);
    lpFilename[nSize] = 0;
    return strlen(lpFilename);
  }

  return 0;
}

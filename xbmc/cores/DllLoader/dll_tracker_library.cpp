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

#include "dll_tracker_library.h"
#include "dll_tracker.h"
#include "dll.h"
#include "DllLoader.h"
#include "DllLoaderContainer.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

extern "C" inline void tracker_library_track(uintptr_t caller, HMODULE hHandle)
{
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(caller);
  if (pInfo && hHandle)
  {
    CSingleLock lock(g_trackerLock);
    pInfo->dllList.push_back(hHandle);
  }
}

extern "C" inline void tracker_library_free(uintptr_t caller, HMODULE hHandle)
{
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(caller);
  if (pInfo && hHandle)
  {
    CSingleLock lock(g_trackerLock);
    for (DllListIter it = pInfo->dllList.begin(); it != pInfo->dllList.end(); ++it)
    {
      if (*it == hHandle)
      {
        pInfo->dllList.erase(it);
        break;
      }
    }
  }
}

extern "C" void tracker_library_free_all(DllTrackInfo* pInfo)
{
  // unloading unloaded dll's
  if (!pInfo->dllList.empty())
  {
    CSingleLock lock(g_trackerLock);
    CLog::Log(LOGDEBUG,"%s: Detected %"PRIdS" unloaded dll's", pInfo->pDll->GetFileName(), pInfo->dllList.size());
    for (DllListIter it = pInfo->dllList.begin(); it != pInfo->dllList.end(); ++it)
    {
      LibraryLoader* pDll = DllLoaderContainer::GetModule((HMODULE)*it);
      if( !pDll)
      {
        CLog::Log(LOGERROR, "%s - Invalid module in tracker", __FUNCTION__);
        return;
      }

      if (!pDll->IsSystemDll())
      {
        if (strlen(pDll->GetFileName()) > 0) CLog::Log(LOGDEBUG,"  : %s", pDll->GetFileName());
      }
    }

    // now unload the dlls
    for (DllListIter it = pInfo->dllList.begin(); it != pInfo->dllList.end(); ++it)
    {
      LibraryLoader* pDll = DllLoaderContainer::GetModule((HMODULE)*it);
      if( !pDll)
      {
        CLog::Log(LOGERROR, "%s - Invalid module in tracker", __FUNCTION__);
        return;
      }

      if (!pDll->IsSystemDll())
      {
        dllFreeLibrary((HMODULE)pDll->GetHModule());
      }
    }
  }
}

extern "C" HMODULE __stdcall track_LoadLibraryA(LPCSTR file)
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();

  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);
  char* path = NULL;
  if (pInfo) path = pInfo->pDll->GetFileName();

  HMODULE hHandle = dllLoadLibraryExtended(file, path);
  tracker_library_track(loc, hHandle);

  return hHandle;
}

extern "C" HMODULE __stdcall track_LoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();

  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);
  char* path = NULL;
  if (pInfo) path = pInfo->pDll->GetFileName();

  HMODULE hHandle = dllLoadLibraryExExtended(lpLibFileName, hFile, dwFlags, path);
  tracker_library_track(loc, hHandle);

  return hHandle;
}

extern "C" BOOL __stdcall track_FreeLibrary(HINSTANCE hLibModule)
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();

  tracker_library_free(loc, hLibModule);

  return dllFreeLibrary(hLibModule);
}



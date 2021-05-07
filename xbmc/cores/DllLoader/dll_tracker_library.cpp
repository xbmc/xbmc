/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "dll_tracker_library.h"

#include "DllLoader.h"
#include "DllLoaderContainer.h"
#include "dll.h"
#include "dll_tracker.h"
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
    CLog::Log(LOGDEBUG,"{0}: Detected {1} unloaded dll's", pInfo->pDll->GetFileName(), pInfo->dllList.size());
    for (DllListIter it = pInfo->dllList.begin(); it != pInfo->dllList.end(); ++it)
    {
      LibraryLoader* pDll = DllLoaderContainer::GetModule((HMODULE)*it);
      if( !pDll)
      {
        CLog::Log(LOGERROR, "{} - Invalid module in tracker", __FUNCTION__);
        return;
      }

      if (!pDll->IsSystemDll())
      {
        if (strlen(pDll->GetFileName()) > 0)
          CLog::Log(LOGDEBUG, "  : {}", pDll->GetFileName());
      }
    }

    // now unload the dlls
    for (DllListIter it = pInfo->dllList.begin(); it != pInfo->dllList.end(); ++it)
    {
      LibraryLoader* pDll = DllLoaderContainer::GetModule((HMODULE)*it);
      if( !pDll)
      {
        CLog::Log(LOGERROR, "{} - Invalid module in tracker", __FUNCTION__);
        return;
      }

      if (!pDll->IsSystemDll())
      {
        dllFreeLibrary((HMODULE)pDll->GetHModule());
      }
    }
  }
}

extern "C" HMODULE __stdcall track_LoadLibraryA(const char* file)
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();

  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);
  const char* path = NULL;
  if (pInfo) path = pInfo->pDll->GetFileName();

  HMODULE hHandle = dllLoadLibraryExtended(file, path);
  tracker_library_track(loc, hHandle);

  return hHandle;
}

extern "C" HMODULE __stdcall track_LoadLibraryExA(const char* lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();

  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);
  const char* path = NULL;
  if (pInfo) path = pInfo->pDll->GetFileName();

  HMODULE hHandle = dllLoadLibraryExExtended(lpLibFileName, hFile, dwFlags, path);
  tracker_library_track(loc, hHandle);

  return hHandle;
}

extern "C" int __stdcall track_FreeLibrary(HINSTANCE hLibModule)
{
  uintptr_t loc = (uintptr_t)_ReturnAddress();

  tracker_library_free(loc, hLibModule);

  return dllFreeLibrary(hLibModule);
}

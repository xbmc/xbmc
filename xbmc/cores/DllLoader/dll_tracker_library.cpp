
#include "stdafx.h"
#include "dll_tracker_library.h"
#include "dll_tracker.h"
#include "dll.h"
#include "DllLoader.h"
#include "DllLoaderContainer.h"

extern "C" inline void tracker_library_track(unsigned long caller, HMODULE hHandle)
{
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(caller);
  if (pInfo && hHandle)
  {
    pInfo->dllList.push_back(hHandle);
  }
}

extern "C" inline void tracker_library_free(unsigned long caller, HMODULE hHandle)
{
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(caller);
  if (pInfo && hHandle)
  {
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
    CLog::DebugLog("%s: Detected %d unloaded dll's", pInfo->pDll->GetFileName(), pInfo->dllList.size());
    for (DllListIter it = pInfo->dllList.begin(); it != pInfo->dllList.end(); ++it)
    {
      DllLoader* pDll = DllLoaderContainer::GetModule((HMODULE)*it);
      if( !pDll)
      {
        CLog::Log(LOGERROR, "%s - Invalid module in tracker", __FUNCTION__);
        return;
      }

      if (!pDll->IsSystemDll())
      {
        if (strlen(pDll->GetFileName()) > 0) CLog::DebugLog("  : %s", pDll->GetFileName());
      }
    }
    
    // now unload the dlls
    for (DllListIter it = pInfo->dllList.begin(); it != pInfo->dllList.end(); ++it)
    {
      DllLoader* pDll = DllLoaderContainer::GetModule((HMODULE)*it);
      if( !pDll)
      {
        CLog::Log(LOGERROR, "%s - Invalid module in tracker", __FUNCTION__);
        return;
      }

      if (!pDll->IsSystemDll())
      {
        dllFreeLibrary((HMODULE)pDll->hModule);
      }
    }
  }
}
 
extern "C" HMODULE __stdcall track_LoadLibraryA(LPCSTR file)
{
  unsigned loc = (unsigned)_ReturnAddress();

  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);
  char* path = NULL;
  if (pInfo) path = pInfo->pDll->GetFileName();
  
  HMODULE hHandle = dllLoadLibraryExtended(file, path);
  tracker_library_track(loc, hHandle);
  
  return hHandle;
}

extern "C" HMODULE __stdcall track_LoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
  unsigned loc = (unsigned)_ReturnAddress();

  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);
  char* path = NULL;
  if (pInfo) path = pInfo->pDll->GetFileName();
  
  HMODULE hHandle = dllLoadLibraryExExtended(lpLibFileName, hFile, dwFlags, path);
  tracker_library_track(loc, hHandle);
  
  return hHandle;
}

extern "C" BOOL __stdcall track_FreeLibrary(HINSTANCE hLibModule)
{
  unsigned loc = (unsigned)_ReturnAddress();

  tracker_library_free(loc, hLibModule);

  return dllFreeLibrary(hLibModule);
}




#include "../../stdafx.h"
#include "dll_tracker.h"
#include "dll_tracker_memory.h"
#include "dll_tracker_library.h"
#include "DllLoader.h"

#ifdef _cplusplus
extern "C"
{
#endif

TrackedDllList g_trackedDlls;

void tracker_dll_add(DllLoader* pDll)
{
  DllTrackInfo* trackInfo = new DllTrackInfo;
  trackInfo->pDll = pDll;
  trackInfo->lMinAddr = 0;
  trackInfo->lMaxAddr = 0;
  g_trackedDlls.push_back(trackInfo);
}

void tracker_dll_free(DllLoader* pDll)
{
  for (TrackedDllsIter it = g_trackedDlls.begin(); it != g_trackedDlls.end(); ++it)
  {
    if ((*it)->pDll == pDll)
    {
      tracker_memory_free_all(*it);
      tracker_library_free_all(*it);
      
      // free all functions which where created at the time we loaded the dll
	    DummyListIter dit = (*it)->dummyList.begin();
	    while (dit != (*it)->dummyList.end()) { delete(*dit); dit++;	}
	    (*it)->dummyList.clear();
	
      delete (*it);
      it = g_trackedDlls.erase(it);
    }
  }
}

void tracker_dll_set_addr(DllLoader* pDll, unsigned int min, unsigned int max)
{
  for (TrackedDllsIter it = g_trackedDlls.begin(); it != g_trackedDlls.end(); ++it)
  {
    if ((*it)->pDll == pDll)
    {
      (*it)->lMinAddr = min;
      (*it)->lMaxAddr = max;
      break;
    }
  }
}

char* tracker_getdllname(unsigned long caller)
{
  for (TrackedDllsIter it = g_trackedDlls.begin(); it != g_trackedDlls.end(); ++it)
  {
    if (caller >= (*it)->lMinAddr && caller <= (*it)->lMaxAddr)
    {
      return (*it)->pDll->GetFileName();
    }
  }
  return "";
}

void* tracker_dll_get_function(DllLoader* pDll, char* sFunctionName)
{
  if (!strcmp(sFunctionName, "malloc") || !strcmp(sFunctionName, "??2@YAPAXI@Z"))
  {
    return track_malloc;
  }
  else if (!strcmp(sFunctionName, "calloc"))
  {
    return track_calloc;
  }
  else if (!strcmp(sFunctionName, "realloc"))
  {
    return track_realloc;
  }
  else if (!strcmp(sFunctionName, "free") ||
      !strcmp(sFunctionName, "??3@YAXPAX@Z") ||
      !strcmp(sFunctionName, "??_V@YAXPAX@Z"))
  {
    return track_free;
  }
  else if (!strcmp(sFunctionName, "_strdup"))
  {
    return track_strdup;
  }
  else if (!strcmp(sFunctionName, "LoadLibraryA"))
  {
    return track_LoadLibraryA;
  }
  else if (!strcmp(sFunctionName, "LoadLibraryExA"))
  {
    return track_LoadLibraryExA;
  }
  else if (!strcmp(sFunctionName, "FreeLibrary"))
  {
    return track_FreeLibrary;
  }
  return NULL;
}

DllTrackInfo* tracker_get_dlltrackinfo(unsigned long caller)
{
  for (TrackedDllsIter it = g_trackedDlls.begin(); it != g_trackedDlls.end(); ++it)
  {
    if (caller >= (*it)->lMinAddr && caller <= (*it)->lMaxAddr)
    {
      return *it;
    }
  }
  return NULL;
}

void tracker_dll_data_track(DllLoader* pDll, unsigned long addr)
{
  for (TrackedDllsIter it = g_trackedDlls.begin(); it != g_trackedDlls.end(); ++it)
  {
    if (pDll == (*it)->pDll)
    {
      (*it)->dummyList.push_back((unsigned long*)addr);
      break;
    }
  }
}

#ifdef _cplusplus
}
#endif
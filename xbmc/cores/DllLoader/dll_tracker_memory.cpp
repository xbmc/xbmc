
#include "../../stdafx.h"
#include "dll_tracker_memory.h"
#include "dll_tracker.h"
#include "DllLoader.h"

struct MSizeCount
{
  unsigned size;
  unsigned count;
};

typedef std::map<unsigned, MSizeCount> CallerMap;
typedef std::map<unsigned, MSizeCount>::iterator CallerMapIter;

extern "C" inline void tracker_memory_track(unsigned long caller, void* data_addr, unsigned long size)
{
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(caller);
  if (pInfo)
  {
    AllocLenCaller temp;
    temp.size = size;
    temp.calleraddr = caller;
    pInfo->dataList[(unsigned)data_addr] = temp;
  }
}

extern "C" inline void tracker_memory_free(DllTrackInfo* pInfo, void* data_addr)
{
  if (!pInfo || pInfo->dataList.erase((unsigned)data_addr) < 1)
  {
    // unable to delete the pointer from one of the trackers, but track_free is called!!
    // This will happen when memory is freed by another dll then the one which allocated the memory.
    // We, have to search every map for this memory pointer.
    // Yes, it's slow todo, but if we are freeing already freed pointers when unloading a dll
    // xbmc will crash.
    for (TrackedDllsIter it = g_trackedDlls.begin(); it != g_trackedDlls.end(); ++it)
    {
      // try to free the pointer from this list, and break if success
      if ((*it)->dataList.erase((unsigned)data_addr) > 0) break;
    }
  }
}

extern "C" void tracker_memory_free_all(DllTrackInfo* pInfo)
{
  if (!pInfo->dataList.empty())
  {
    CLog::DebugLog("%s: Detected memory leaks: %d leaks", pInfo->pDll->GetFileName(), pInfo->dataList.size());
    unsigned total = 0;
    CallerMap tempMap;
    CallerMapIter itt;
    for (DataListIter p = pInfo->dataList.begin(); p != pInfo->dataList.end(); ++p)
    {
      total += (p->second).size;
      free((void*)p->first);
      if ( ( itt = tempMap.find((p->second).calleraddr) ) != tempMap.end() )
      {
        (itt->second).size += (p->second).size;
        (itt->second).count++;
      }
      else
      {
        MSizeCount temp;
        temp.size = (p->second).size;
        temp.count = 1;
        tempMap.insert(std::make_pair( (p->second).calleraddr, temp ) );
      }
    }
    for ( itt = tempMap.begin(); itt != tempMap.end();++itt )
    {
      CLog::DebugLog("leak caller address %8x, size %8i, counter %4i", itt->first, (itt->second).size, (itt->second).count);
    }
    CLog::DebugLog("%s: Total bytes leaked: %d", pInfo->pDll->GetName(), total);
    tempMap.erase(tempMap.begin(), tempMap.end());
  }
  pInfo->dataList.erase(pInfo->dataList.begin(), pInfo->dataList.end());
}

extern "C" void* __cdecl track_malloc(size_t s)
{
  unsigned loc;
  __asm mov eax, [ebp + 4]
  __asm mov loc, eax

  void* p = malloc(s);
  if (!p) 
  {    
    CLog::Log(LOGDEBUG, "DLL: %s : malloc failed, crash imminent", tracker_getdllname(loc));
    return NULL;
  }

  tracker_memory_track(loc, p, s);

  return p;
}

extern "C" void* __cdecl track_calloc(size_t n, size_t s)
{
  unsigned loc;
  __asm mov eax, [ebp + 4]
  __asm mov loc, eax

  void* p = calloc(n, s);
  if (!p) 
  {    
    CLog::Log(LOGDEBUG, "DLL: %s : calloc failed, crash imminent", tracker_getdllname(loc));
    return NULL;
  }

  tracker_memory_track(loc, p, s);
  
  return p;
}

extern "C" void* __cdecl track_realloc(void* p, size_t s)
{
  unsigned loc;
  __asm mov eax, [ebp + 4]
  __asm mov loc, eax

  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);

  void* q = realloc(p, s);
  if (!q) 
  {
    //  a dll may realloc with a size of 0, so NULL is the correct return value is this case
    if (s > 0) CLog::Log(LOGDEBUG, "DLL: %s : realloc failed, crash imminent", tracker_getdllname(loc));
    return NULL;
  }

  if (pInfo)
  {
    if (p != q) tracker_memory_free(pInfo, p);
    AllocLenCaller temp;
    temp.size = s;
    temp.calleraddr = loc;
    pInfo->dataList[(unsigned)q] = temp;
  }

  return q;
}

extern "C" void __cdecl track_free(void* p)
{
  unsigned loc;
  __asm mov eax, [ebp + 4]
  __asm mov loc, eax

  tracker_memory_free(tracker_get_dlltrackinfo(loc), p);

  free(p);
}

extern "C" char* __cdecl track_strdup(const char* str)
{
  unsigned loc;
  __asm mov eax, [ebp + 4]
  __asm mov loc, eax

  char* pdup = strdup(str);

  tracker_memory_track(loc, pdup, 0);

  return pdup;
}

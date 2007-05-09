#include "stdafx.h"
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

extern "C" inline bool tracker_memory_free(DllTrackInfo* pInfo, void* data_addr)
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
      if ((*it)->dataList.erase((unsigned)data_addr) > 0) return true;
    }
    return false;
  }
  return true;
}

extern "C" void tracker_memory_free_all(DllTrackInfo* pInfo)
{
  if (!pInfo->dataList.empty() || !pInfo->virtualList.empty())
  {
    CLog::Log(LOGDEBUG,"%s (base %8x): Detected memory leaks: %d leaks", pInfo->pDll->GetFileName(), pInfo->pDll->hModule, pInfo->dataList.size() + pInfo->virtualList.size());
    unsigned total = 0;
    CallerMap tempMap;
    CallerMapIter itt;
    for (DataListIter p = pInfo->dataList.begin(); p != pInfo->dataList.end(); ++p)
    {
      try
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
      catch(...)
      {
        CLog::Log(LOGERROR, "failed to free memory. buffer overrun is likely cause");
      }

    }
#ifndef _LINUX
    for (VAllocListIter p = pInfo->virtualList.begin(); p != pInfo->virtualList.end(); ++p)
    {
      total += (p->second).size;
      VirtualFree((void*)p->first, 0, MEM_RELEASE);
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
#endif

    for ( itt = tempMap.begin(); itt != tempMap.end();++itt )
    {
      CLog::Log(LOGDEBUG,"leak caller address %8x, size %8i, counter %4i", itt->first, (itt->second).size, (itt->second).count);
    }
    CLog::Log(LOGDEBUG,"%s: Total bytes leaked: %d", pInfo->pDll->GetName(), total);
    tempMap.erase(tempMap.begin(), tempMap.end());
  }
  pInfo->dataList.erase(pInfo->dataList.begin(), pInfo->dataList.end());
  pInfo->virtualList.erase(pInfo->virtualList.begin(), pInfo->virtualList.end());
}

extern "C" void* __cdecl track_malloc(size_t s)
{
  unsigned loc = (unsigned)_ReturnAddress();

  void* p = malloc(s);
  if (!p) 
  {    
    CLog::Log(LOGSEVERE, "DLL: %s : malloc failed, crash imminent (Out of memory requesting %d bytes)", tracker_getdllname(loc), s);
    return NULL;
  }

  tracker_memory_track(loc, p, s);

  return p;
}

extern "C" void* __cdecl track_calloc(size_t n, size_t s)
{
  unsigned loc = (unsigned)_ReturnAddress();

  void* p = calloc(n, s);
  if (!p) 
  {    
    CLog::Log(LOGSEVERE, "DLL: %s : calloc failed, crash imminent (Out of memory)", tracker_getdllname(loc));
    return NULL;
  }

  tracker_memory_track(loc, p, s);
  
  return p;
}

extern "C" void* __cdecl track_realloc(void* p, size_t s)
{
  unsigned loc = (unsigned)_ReturnAddress();

  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);

  void* q = realloc(p, s);
  if (!q) 
  {
    //  a dll may realloc with a size of 0, so NULL is the correct return value is this case
    if (s > 0) CLog::Log(LOGSEVERE, "DLL: %s : realloc failed, crash imminent (Out of memory)", tracker_getdllname(loc));
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
  unsigned loc = (unsigned)_ReturnAddress();

  //Only call free if this is actually something that has been allocated
  if( tracker_memory_free(tracker_get_dlltrackinfo(loc), p) )
  {
    free(p);
  }
  else
  { 
    //Only do this on memmory that wasn't found in track list.
    //this should only be an exception and using exception handling
    //here in the normal case seems overkill
    try
    {
      //free(p);
    }
    catch(...)
    {
      CLog::Log(LOGDEBUG,"DLL tried to free memory not allocated by any DLL, skipping");
    }
  }
}

extern "C" char* __cdecl track_strdup(const char* str)
{
  unsigned loc = (unsigned)_ReturnAddress();

  char* pdup = strdup(str);

  tracker_memory_track(loc, pdup, 0);

  return pdup;
}

extern "C" void tracker_heapobjects_free_all(DllTrackInfo* pInfo)
{
#ifndef _LINUX
  if (!pInfo->heapobjectList.empty())
  {
    CLog::Log(LOGDEBUG,"%s: Detected heapobject leaks: %d leaks", pInfo->pDll->GetFileName(), pInfo->heapobjectList.size());

    for (HeapObjectListIter it = pInfo->heapobjectList.begin(); it != pInfo->heapobjectList.end(); ++it)
    {
      try
      {
        HeapDestroy(*it);
      }
      catch(...)
      {
        CLog::Log(LOGERROR, "failed to free heapobject. might have been freed from somewhere else");
      }
    }
    pInfo->heapobjectList.erase(pInfo->heapobjectList.begin(), pInfo->heapobjectList.end());
  }
#endif
}

#ifndef _LINUX

HANDLE
WINAPI
track_HeapCreate(
    IN DWORD flOptions,
    IN SIZE_T dwInitialSize,
    IN SIZE_T dwMaximumSize
    )
{
  unsigned loc = (unsigned)_ReturnAddress();

  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);
  HANDLE hHeap = HeapCreate(flOptions, dwInitialSize, dwMaximumSize);

  if( pInfo && hHeap )
  {
    pInfo->heapobjectList.push_back(hHeap);    
  }
  
  return hHeap;
}

BOOL
WINAPI
track_HeapDestroy(
    IN OUT HANDLE hHeap
    )
{
  unsigned loc = (unsigned)_ReturnAddress();


  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);
  if (pInfo && hHeap)
  {
    for (HeapObjectListIter it = pInfo->heapobjectList.begin(); it != pInfo->heapobjectList.end(); ++it)
    {
      if (*it == hHeap)
      {
        pInfo->heapobjectList.erase(it);
        break;
      }
    }
  }

  return HeapDestroy(hHeap);
}


BOOL WINAPI track_VirtualFreeEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType)
{
  unsigned loc = (unsigned)_ReturnAddress();

  if(dwFreeType == MEM_RELEASE)
  {    
    DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);
    if (pInfo)
      pInfo->virtualList.erase((unsigned)lpAddress);
  }
  return VirtualFreeEx(hProcess, lpAddress, dwSize, dwFreeType);
}


LPVOID WINAPI track_VirtualAllocEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
  unsigned loc = (unsigned)_ReturnAddress();

  LPVOID address = VirtualAllocEx(hProcess, lpAddress, dwSize, flAllocationType, flProtect);
  
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);
  if(pInfo)
  {
    // make sure we get the base address for this allocation
    MEMORY_BASIC_INFORMATION info;
    if(VirtualQueryEx(hProcess, address, &info, sizeof(info)))
    {
      AllocLenCaller temp;
      temp.size = (unsigned int)info.AllocationBase;
      temp.calleraddr = loc;    
      pInfo->virtualList[(unsigned)address] = temp;
    }
  }
  return address;
}

LPVOID WINAPI track_VirtualAlloc( LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
  unsigned loc = (unsigned)_ReturnAddress();

  LPVOID address = VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
  
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);
  if(pInfo)
  {
    // make sure we get the base address for this allocation
    MEMORY_BASIC_INFORMATION info;
    if(VirtualQuery(address, &info, sizeof(info)))
    {
      AllocLenCaller temp;
      temp.size = (unsigned int)dwSize;
      temp.calleraddr = loc;    
      pInfo->virtualList[(unsigned)info.AllocationBase] = temp;
    }
  }
  return address;
}

BOOL WINAPI track_VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType)
{
  unsigned loc = (unsigned)_ReturnAddress();

  if(dwFreeType == MEM_RELEASE)
  {    
    DllTrackInfo* pInfo = tracker_get_dlltrackinfo(loc);
    if (pInfo)
      pInfo->virtualList.erase((unsigned)lpAddress);
  }
  return VirtualFree(lpAddress, dwSize, dwFreeType);
}

#endif

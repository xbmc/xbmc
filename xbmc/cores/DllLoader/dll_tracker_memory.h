
#ifndef _DLL_TRACKER_MEMORY
#define _DLL_TRACKER_MEMORY

#include "dll_tracker.h"

extern "C" inline void tracker_memory_track(uintptr_t caller, void* data_addr, size_t size);
extern "C" void tracker_memory_free_all(DllTrackInfo* pInfo);
extern "C" void tracker_heapobjects_free_all(DllTrackInfo* pInfo);

extern "C" void* __cdecl track_malloc(size_t s);
extern "C" void* __cdecl track_calloc(size_t n, size_t s);
extern "C" void* __cdecl track_realloc(void* p, size_t s);
extern "C" void __cdecl track_free(void* p);
extern "C" char* __cdecl track_strdup(const char* str);

LPVOID WINAPI track_VirtualAllocEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
BOOL WINAPI track_VirtualFreeEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType);

LPVOID WINAPI track_VirtualAlloc( LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
BOOL WINAPI track_VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType);


HANDLE
WINAPI
track_HeapCreate(
    IN DWORD flOptions,
    IN SIZE_T dwInitialSize,
    IN SIZE_T dwMaximumSize
    );

BOOL
WINAPI
track_HeapDestroy(
    IN OUT HANDLE hHeap
    );

#endif // _DLL_TRACKER_MEMORY

#ifndef _DLL_TRACKER_MEMORY
#define _DLL_TRACKER_MEMORY

#include "dll_tracker.h"

extern "C" inline void tracker_memory_track(unsigned long caller, void* data_addr, unsigned long size);
extern "C" inline void tracker_memory_free_all(DllTrackInfo* pInfo);

extern "C" void* __cdecl track_malloc(size_t s);
extern "C" void* __cdecl track_calloc(size_t n, size_t s);
extern "C" void* __cdecl track_realloc(void* p, size_t s);
extern "C" void __cdecl track_free(void* p);
extern "C" char* __cdecl track_strdup(const char* str);

#endif // _DLL_TRACKER_MEMORY
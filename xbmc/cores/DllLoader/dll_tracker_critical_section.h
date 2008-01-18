
#ifndef _DLL_TRACKER_CRITICAL_SECTION
#define _DLL_TRACKER_CRITICAL_SECTION

#include "dll_tracker.h"

class CriticalSection_List;

extern "C" void tracker_critical_section_free_all(DllTrackInfo* pInfo);

extern "C" void __stdcall track_InitializeCriticalSection(LPCRITICAL_SECTION cs);
extern "C" void __stdcall track_DeleteCriticalSection(LPCRITICAL_SECTION cs);

#endif // _DLL_TRACKER_CRITICAL_SECTION
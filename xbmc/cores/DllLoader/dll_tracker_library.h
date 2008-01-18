
#ifndef _DLL_TRACKER_LIBRARY
#define _DLL_TRACKER_LIBRARY

#include "dll_tracker.h"

extern "C" void tracker_library_free_all(DllTrackInfo* pInfo);

extern "C" HMODULE __stdcall track_LoadLibraryA(LPCSTR file);
extern "C" HMODULE __stdcall track_LoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
extern "C" BOOL __stdcall track_FreeLibrary(HINSTANCE hLibModule);

#endif // _DLL_TRACKER_LIBRARY
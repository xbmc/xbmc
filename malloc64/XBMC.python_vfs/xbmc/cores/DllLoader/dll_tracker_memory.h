#ifndef _DLL_TRACKER_MEMORY
#define _DLL_TRACKER_MEMORY

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "dll_tracker.h"

extern "C" inline void tracker_memory_track(uintptr_t caller, void* data_addr, size_t size);
extern "C" void tracker_memory_free_all(DllTrackInfo* pInfo);
extern "C" void tracker_heapobjects_free_all(DllTrackInfo* pInfo);

extern "C" void* __cdecl track_malloc(size_t s);
extern "C" void* __cdecl track_calloc(size_t n, size_t s);
extern "C" void* __cdecl track_realloc(void* p, size_t s);
extern "C" void __cdecl track_free(void* p);
extern "C" char* __cdecl track_strdup(const char* str);

#ifndef _LINUX
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
#endif
#endif // _DLL_TRACKER_MEMORY

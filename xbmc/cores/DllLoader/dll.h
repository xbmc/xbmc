#ifndef __DLL_H_
#define __DLL_H_

#include "system.h"

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

extern "C" HMODULE __stdcall dllLoadLibraryExtended(LPCSTR file, LPCSTR sourcedll);
extern "C" HMODULE __stdcall dllLoadLibraryA(LPCSTR file);
extern "C" HMODULE __stdcall dllLoadLibraryExExtended(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags, LPCSTR sourcedll);
extern "C" HMODULE __stdcall dllLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
extern "C" BOOL __stdcall dllFreeLibrary(HINSTANCE hLibModule);
extern "C" FARPROC __stdcall dllGetProcAddress(HMODULE hModule, LPCSTR function);
extern "C" HMODULE WINAPI dllGetModuleHandleA(LPCSTR lpModuleName);
extern "C" DWORD WINAPI dllGetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize);

#endif

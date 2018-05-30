/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "PlatformDefs.h"

extern "C" HMODULE __stdcall dllLoadLibraryExtended(const char* file, const char* sourcedll);
extern "C" HMODULE __stdcall dllLoadLibraryA(const char* file);
extern "C" HMODULE __stdcall dllLoadLibraryExExtended(const char* lpLibFileName, HANDLE hFile, DWORD dwFlags, const char* sourcedll);
extern "C" HMODULE __stdcall dllLoadLibraryExA(const char* lpLibFileName, HANDLE hFile, DWORD dwFlags);
extern "C" int __stdcall dllFreeLibrary(HINSTANCE hLibModule);
extern "C" intptr_t (*__stdcall dllGetProcAddress(HMODULE hModule, const char* function))(void);
extern "C" HMODULE WINAPI dllGetModuleHandleA(const char* lpModuleName);


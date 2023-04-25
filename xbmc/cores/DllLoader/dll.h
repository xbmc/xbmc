/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PlatformDefs.h"

extern "C" HMODULE __stdcall dllLoadLibraryExtended(const char* file, const char* sourcedll);
extern "C" HMODULE __stdcall dllLoadLibraryA(const char* file);
extern "C" HMODULE __stdcall dllLoadLibraryExExtended(const char* lpLibFileName, HANDLE hFile, DWORD dwFlags, const char* sourcedll);
extern "C" HMODULE __stdcall dllLoadLibraryExA(const char* lpLibFileName, HANDLE hFile, DWORD dwFlags);
extern "C" int __stdcall dllFreeLibrary(HINSTANCE hLibModule);
extern "C" HMODULE WINAPI dllGetModuleHandleA(const char* lpModuleName);


/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dll_tracker.h"

extern "C" void tracker_library_free_all(DllTrackInfo* pInfo);

extern "C" HMODULE __stdcall track_LoadLibraryA(const char* file);
extern "C" HMODULE __stdcall track_LoadLibraryExA(const char* lpLibFileName, HANDLE hFile, DWORD dwFlags);
extern "C" int __stdcall track_FreeLibrary(HINSTANCE hLibModule);

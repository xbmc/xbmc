#pragma once

/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "dll_tracker.h"

extern "C" void tracker_library_free_all(DllTrackInfo* pInfo);

extern "C" HMODULE __stdcall track_LoadLibraryA(const char* file);
extern "C" HMODULE __stdcall track_LoadLibraryExA(const char* lpLibFileName, HANDLE hFile, DWORD dwFlags);
extern "C" int __stdcall track_FreeLibrary(HINSTANCE hLibModule);

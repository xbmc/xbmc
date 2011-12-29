#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef TARGET_WINDOWS

#define WIN32_LEAN_AND_MEAN           // Enable LEAN_AND_MEAN support
#include <windows.h>
#include <process.h>

typedef HANDLE wait_event_t;
typedef CRITICAL_SECTION criticalsection_t;
typedef unsigned __int32 uint;
typedef DWORD tThreadId;

#ifndef va_copy
#define va_copy(x, y) x = y
#endif


/* Platform dependent path separator */
#define PATH_SEPARATOR_CHAR '\\'

#endif //TARGET_WINDOWS

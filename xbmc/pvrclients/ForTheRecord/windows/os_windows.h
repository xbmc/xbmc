#pragma once
/*
 *      Copyright (C) 2010 Marcel Groothuis
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

#define WIN32_LEAN_AND_MEAN           // Enable LEAN_AND_MEAN support
#include <windows.h>

typedef signed __int8 int8_t;
typedef signed __int16 int16_t;
typedef signed __int32 int32_t;
typedef signed __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
typedef HANDLE pthread_t;
typedef HANDLE pthread_mutex_t;
typedef HANDLE wait_event_t;
typedef CRITICAL_SECTION criticalsection_t;
typedef unsigned __int32 uint;

#ifndef va_copy
#define va_copy(x, y) x = y
#endif

#define snprintf _snprintf

#define PATH_SEPARATOR_CHAR '\\'

static inline void usleep(unsigned long usec)
{
  Sleep(usec/1000);
}

#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#define WIN32_LEAN_AND_MEAN           // Enable LEAN_AND_MEAN support
#include <windows.h>

//#ifndef _WINSOCKAPI_
//#define _WINSOCKAPI_
//#endif
//#pragma warning(disable:4005) // Disable "warning C4005: '_WINSOCKAPI_' : macro redefinition"
//#include <winsock2.h>
//#pragma warning(default:4005)
//#include <stdlib.h>
//#include <stdio.h>
//#include <stdarg.h>
//#include <signal.h>
//#include <time.h>
//#include <sys/types.h>
//#include <sys/timeb.h>

//#if defined(DLL_IMPORT)
//#define LIBTYPE __declspec( dllexport )
//#elif  defined(DLL_EXPORT)
//#define LIBTYPE __declspec( dllimport )
//#else
//#define LIBTYPE
//#endif


typedef HANDLE wait_event_t;
typedef CRITICAL_SECTION criticalsection_t;
typedef unsigned __int32 uint;

#ifndef va_copy
#define va_copy(x, y) x = y
#endif


#define PATH_SEPARATOR_CHAR '\\'
//
//static inline void usleep(unsigned long usec)
//{
//  Sleep(usec/1000);
//}

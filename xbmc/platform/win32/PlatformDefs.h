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

#ifdef TARGET_WINDOWS

#include <windows.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

typedef __int64       off64_t;
typedef __int64       fpos64_t;
typedef __int64       __off64_t;
typedef long          __off_t;

#if !defined(_SSIZE_T_DEFINED) && !defined(HAVE_SSIZE_T)
typedef intptr_t      ssize_t;
#define _SSIZE_T_DEFINED
#endif // !_SSIZE_T_DEFINED
#ifndef SSIZE_MAX
#define SSIZE_MAX INTPTR_MAX
#endif // !SSIZE_MAX

#define ftello64 _ftelli64
#define fseeko64 _fseeki64
#ifndef strcasecmp
#define strcasecmp strcmpi
#endif
#ifndef strncasecmp
#define strncasecmp strnicmp
#endif

#if defined TARGET_WINDOWS_DESKTOP
#define popen   _popen
#define pclose  _pclose
#endif

#if 0
// big endian
#define PIXEL_ASHIFT 0
#define PIXEL_RSHIFT 8
#define PIXEL_GSHIFT 16
#define PIXEL_BSHIFT 24
#else
// little endian
#define PIXEL_ASHIFT 24
#define PIXEL_RSHIFT 16
#define PIXEL_GSHIFT 8
#define PIXEL_BSHIFT 0
#endif

extern "C" char * strptime(const char *buf, const char *fmt, struct tm *tm);
extern "C" char * strcasestr(const char* haystack, const char* needle);

#endif // TARGET_WINDOWS


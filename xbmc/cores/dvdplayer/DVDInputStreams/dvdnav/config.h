#pragma once

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

/* config.h.  Generated by hand.  */
#ifdef _XBOX
#include <xtl.h>
#elif defined(_LINUX)
#include "PlatformInclude.h"
#else
#include <windows.h>
#endif
#include <stdio.h>

//#define HAVE_DLFCN_H 1
#define HAVE_DVDCSS_DVDCSS_H 1
/* #undef HAVE_DVDCSS_DVDCSS_H*/
/* #undef HAVE_INTTYPES_H */
#define HAVE_MEMORY_H 1
/* #undef HAVE_STDINT_H */
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
/* #undef HAVE_UNISTD_H */
#define PACKAGE "libdvdread"
#ifndef PACKAGE_BUGREPORT
#define PACKAGE_BUGREPORT ""
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME ""
#endif
#ifndef PACKAGE_STRING
#define PACKAGE_STRING ""
#endif
#ifndef PACKAGE_TARNAME
#define PACKAGE_TARNAME ""
#endif
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION ""
#endif
#define STDC_HEADERS 1
#define VERSION "1.2.6"
/* #undef WORDS_BIGENDIAN */
/* #undef __DARWIN__ */
/* #undef const */
#define inline __inline
/* #undef size_t */

#define ssize_t int

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

#define strcasecmp stricmp
#define strncasecmp strnicmp

#ifndef S_ISDIR
#define S_ISDIR(m) ((m) & _S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(m) ((m) & _S_IFREG)
#endif
#ifndef S_ISBLK
#define S_ISBLK(m) 0
#endif
#ifndef S_ISCHR
#define S_ISCHR(m) 0
#endif

/* Fallback types (very x86-centric, sorry) */
typedef unsigned char       uint8_t;
typedef signed char         int8_t;
typedef unsigned short      uint16_t;
typedef signed short        int16_t;
typedef unsigned int        uint32_t;
typedef signed int          int32_t;
#if __WORDSIZE == 64
typedef unsigned long       uint64_t;
typedef signed long         int64_t;
#else /* __WORDSIZE != 64 */
typedef unsigned __int64    uint64_t;
typedef signed __int64      int64_t;
#endif
#ifndef __APPLE__
#if __WORDSIZE == 64
typedef unsigned long       uintptr_t;
#else /* __WORDSIZE != 64 */
typedef unsigned int        uintptr_t;
#endif /* __WORDSIZE == 64 */
#endif /* !__APPLE__ */


#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

/* config.h.  Generated by hand.  */
#if defined(TARGET_POSIX)
#include "PlatformDefs.h"
#else
#include <windows.h>
#endif
#include <stdio.h>

//#define HAVE_DLFCN_H 1
#define HAVE_DVDCSS_DVDCSS_H 1
/* #undef HAVE_DVDCSS_DVDCSS_H*/
/* #undef HAVE_INTTYPES_H */
#define HAVE_MEMORY_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
/* #undef HAVE_UNISTD_H */
#ifndef PACKAGE
#define PACKAGE "libdvdread"
#endif
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
#ifndef VERSION
#define VERSION "1.2.6"
#endif
/* #undef WORDS_BIGENDIAN */
/* #undef __DARWIN__ */
/* #undef const */
#define inline __inline
/* #undef size_t */

#define ssize_t int

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

#ifndef strcasecmp
#define strcasecmp stricmp
#endif
#ifndef strncasecmp
#define strncasecmp strnicmp
#endif

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

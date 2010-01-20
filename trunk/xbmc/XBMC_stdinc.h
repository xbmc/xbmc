/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/* This is a general header that includes C language support */

#ifndef _XBMC_stdinc_h
#define _XBMC_stdinc_h

#ifdef HAVE_CONFIG_H
#ifndef _WIN32 // HAVE_CONFIG_H is defined somewhere in the dvdplayer stuff, which isn't the config.h we're wanting here
#include "config.h"
#endif
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#if defined(STDC_HEADERS)
# include <stdlib.h>
# include <stddef.h>
# include <stdarg.h>
#else
# if defined(HAVE_STDLIB_H)
#  include <stdlib.h>
# elif defined(HAVE_MALLOC_H)
#  include <malloc.h>
# endif
# if defined(HAVE_STDDEF_H)
#  include <stddef.h>
# endif
# if defined(HAVE_STDARG_H)
#  include <stdarg.h>
# endif
#endif
#ifdef HAVE_STRING_H
# if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#  include <memory.h>
# endif
# include <string.h>
#endif
#if defined(_WIN32) // WIN32INCLUDES
# include "win32/stdint.h"
#else
# if defined(HAVE_INTTYPES_H)
#  include <inttypes.h>
# elif defined(HAVE_STDINT_H)
#  include <stdint.h>
# endif
#endif
#ifdef HAVE_CTYPE_H
# include <ctype.h>
#endif
#ifdef HAVE_ICONV_H
# include <iconv.h>
#endif

/* The number of elements in an array */
#define XBMC_arraysize(array)	(sizeof(array)/sizeof(array[0]))
#define XBMC_TABLESIZE(table)	XBMC_arraysize(table)

#endif /* _SDL_stdinc_h */

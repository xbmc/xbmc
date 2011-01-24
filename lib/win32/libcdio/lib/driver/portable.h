/*
    $Id: portable.h,v 1.4 2004/11/07 21:13:10 rocky Exp $

    Copyright (C) Rocky Bernstein <rocky@panix.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* 
   This file contains definitions to fill in for differences or
   deficiencies to OS or compiler irregularities.  If this file is
   included other routines can be more portable.
*/

#ifndef __CDIO_PORTABLE_H__
#define __CDIO_PORTABLE_H__

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#if !defined(HAVE_FTRUNCATE)
# if defined ( WIN32 )
#  define ftruncate chsize
# endif
#endif /*HAVE_FTRUNCATE*/

#if !defined(HAVE_SNPRINTF)
# if defined ( MSVC )
#  define snprintf _snprintf
# endif
#endif /*HAVE_SNPRINTF*/

#if !defined(HAVE_VSNPRINTF)
# if defined ( MSVC )
#  define vsnprintf _vsnprintf
# endif
#endif /*HAVE_SNPRINTF*/

#ifdef MSVC
# include <io.h>

# ifndef S_ISBLK
#  define _S_IFBLK        0060000  /* Block Special */
#  define S_ISBLK(x) (x & _S_IFBLK)
# endif

# ifndef S_ISCHR
#  ifndef _S_IFCHR
#   define	_S_IFCHR 0020000	/* character special */
#  endif
#  define S_ISCHR(x) (x & _S_IFCHR)
# endif
#endif /*MSVC*/

#ifdef HAVE_MEMSET
# define BZERO(ptr, size) memset(ptr, 0, size)
#elif  HAVE_BZERO
# define BZERO(ptr, size) bzero(ptr, size)
#else 
  Error -- you need either memset or bzero
#endif

#endif /* __CDIO_PORTABLE_H__ */

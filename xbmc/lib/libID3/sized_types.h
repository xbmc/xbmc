// -*- C++ -*-
/* $Id$

 * id3lib: a C++ library for creating and manipulating id3v1/v2 tags Copyright
 * 1999, 2000 Scott Thomas Haug

 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 * The id3lib authors encourage improvements and optimisations to be sent to
 * the id3lib coordinator.  Please see the README file for details on where to
 * send such submissions.  See the AUTHORS file for a list of people who have
 * contributed to id3lib.  See the ChangeLog file for a list of changes to
 * id3lib.  These files are distributed with id3lib at
 * http://download.sourceforge.net/id3lib/ 
 */

/**
 ** This file defines size-specific typedefs based on the macros defined in
 ** limits.h
 **/

#ifndef _SIZED_TYPES_H_
#define _SIZED_TYPES_H_ 

#include <limits.h>

/* define our datatypes */

/* Define 8-bit types */
#if UCHAR_MAX == 0xff

typedef unsigned char   uint8;
typedef signed char      int8;

#else
#error This machine has no 8-bit type; report compiler, and the contents of your limits.h to the persons in the AUTHORS file
#endif /* UCHAR_MAX == 0xff */

/* Define 16-bit types */
#if UINT_MAX == 0xffff

typedef unsigned int    uint16;
typedef int              int16;

#elif USHRT_MAX == 0xffff

typedef unsigned short  uint16;
typedef short            int16;

#else
#error This machine has no 16-bit type; report compiler, and the contents of your limits.h to the persons in the AUTHORS file
#endif /* UINT_MAX == 0xffff */

/* Define 32-bit types */
#if UINT_MAX == 0xfffffffful

typedef unsigned int    uint32;
typedef int              int32;

#elif ULONG_MAX == 0xfffffffful

typedef unsigned long   uint32;
typedef long             int32;

#elif USHRT_MAX == 0xfffffffful

typedef unsigned short  uint32;
typedef short            int32;

#else
#error This machine has no 32-bit type; report compiler, and the contents of your limits.h to the persons in the AUTHORS file
#endif /* UINT_MAX == 0xfffffffful */

#endif /* _SIZED_TYPES_H_ */


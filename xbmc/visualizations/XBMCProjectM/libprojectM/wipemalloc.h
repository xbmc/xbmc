/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2007 projectM Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * See 'LICENSE.txt' included within this release
 *
 */
/**
 * $Id: wipemalloc.h,v 1.1.1.1 2005/12/23 18:05:03 psperl Exp $
 *
 * Contains an inline function which can replace malloc() that also 
 * call memset( ..., 0, sizeof( ... ) ) on the allocated block for
 * safe initialization
 *
 * $Log$
 */

#ifndef _WIPEMALLOC_H
#define _WIPEMALLOC_H

#ifndef MACOS
#ifndef HAVE_AIX /** AIX has malloc() defined in a strange place... */
#ifdef WIN32
#include <malloc.h>
#endif
#include <string.h>
#include <stdlib.h>
#else
#include <stdlib.h>
#endif /** !HAVE_AIX */
#else
#include <string.h>
#include <stdlib.h>
#endif /** !MACOS */
#include <stdio.h>

#ifdef PANTS
#if defined(WIN32) && !defined(inline)
#define inline
#endif
#endif

/** Safe memory allocator */
 void *wipemalloc( size_t count );
 void wipefree( void *ptr );

#endif /** !_WIPEMALLOC_H */

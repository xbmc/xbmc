/*
 * Copyright (c) 1999-2000 Image Power, Inc. and the University of
 *   British Columbia.
 * Copyright (c) 2001-2002 Michael David Adams.
 * All rights reserved.
 */

/* __START_OF_JASPER_LICENSE__
 * 
 * JasPer License Version 2.0
 * 
 * Copyright (c) 2001-2006 Michael David Adams
 * Copyright (c) 1999-2000 Image Power, Inc.
 * Copyright (c) 1999-2000 The University of British Columbia
 * 
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person (the
 * "User") obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 * 
 * 1.  The above copyright notices and this permission notice (which
 * includes the disclaimer below) shall be included in all copies or
 * substantial portions of the Software.
 * 
 * 2.  The name of a copyright holder shall not be used to endorse or
 * promote products derived from the Software without specific prior
 * written permission.
 * 
 * THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL PART OF THIS
 * LICENSE.  NO USE OF THE SOFTWARE IS AUTHORIZED HEREUNDER EXCEPT UNDER
 * THIS DISCLAIMER.  THE SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS
 * "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
 * INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.  NO ASSURANCES ARE
 * PROVIDED BY THE COPYRIGHT HOLDERS THAT THE SOFTWARE DOES NOT INFRINGE
 * THE PATENT OR OTHER INTELLECTUAL PROPERTY RIGHTS OF ANY OTHER ENTITY.
 * EACH COPYRIGHT HOLDER DISCLAIMS ANY LIABILITY TO THE USER FOR CLAIMS
 * BROUGHT BY ANY OTHER ENTITY BASED ON INFRINGEMENT OF INTELLECTUAL
 * PROPERTY RIGHTS OR OTHERWISE.  AS A CONDITION TO EXERCISING THE RIGHTS
 * GRANTED HEREUNDER, EACH USER HEREBY ASSUMES SOLE RESPONSIBILITY TO SECURE
 * ANY OTHER INTELLECTUAL PROPERTY RIGHTS NEEDED, IF ANY.  THE SOFTWARE
 * IS NOT FAULT-TOLERANT AND IS NOT INTENDED FOR USE IN MISSION-CRITICAL
 * SYSTEMS, SUCH AS THOSE USED IN THE OPERATION OF NUCLEAR FACILITIES,
 * AIRCRAFT NAVIGATION OR COMMUNICATION SYSTEMS, AIR TRAFFIC CONTROL
 * SYSTEMS, DIRECT LIFE SUPPORT MACHINES, OR WEAPONS SYSTEMS, IN WHICH
 * THE FAILURE OF THE SOFTWARE OR SYSTEM COULD LEAD DIRECTLY TO DEATH,
 * PERSONAL INJURY, OR SEVERE PHYSICAL OR ENVIRONMENTAL DAMAGE ("HIGH
 * RISK ACTIVITIES").  THE COPYRIGHT HOLDERS SPECIFICALLY DISCLAIM ANY
 * EXPRESS OR IMPLIED WARRANTY OF FITNESS FOR HIGH RISK ACTIVITIES.
 * 
 * __END_OF_JASPER_LICENSE__
 */

/*
 * Sun Rasterfile Library
 *
 * $Id$
 */

/******************************************************************************\
* Sun Rasterfile
\******************************************************************************/

#ifndef	RAS_COD_H
#define	RAS_COD_H

/******************************************************************************\
* Includes.
\******************************************************************************/

#include "jasper/jas_types.h"

/******************************************************************************\
* Constants.
\******************************************************************************/

#define	RAS_MAGIC	0x59a66a95
#define	RAS_MAGICLEN	4

#define	RAS_TYPE_OLD	0
#define	RAS_TYPE_STD	1
#define	RAS_TYPE_RLE	2

#define	RAS_MT_NONE	0
#define	RAS_MT_EQUALRGB	1
#define	RAS_MT_RAW	2

/******************************************************************************\
* Types.
\******************************************************************************/

/* Sun Rasterfile header. */
typedef struct {

	int_fast32_t magic;
	/* signature */

	int_fast32_t width;
	/* width of image (in pixels) */

	int_fast32_t height;
	/* height of image (in pixels) */

	int_fast32_t depth;
	/* number of bits per pixel */

	int_fast32_t length;
	/* length of image data (in bytes) */

	int_fast32_t type;
	/* format of image data */

	int_fast32_t maptype;
	/* colormap type */

	int_fast32_t maplength;
	/* length of colormap data (in bytes) */

} ras_hdr_t;

#define	RAS_CMAP_MAXSIZ	256

/* Color map. */
typedef struct {

	int len;
	/* The number of entries in the color map. */

	int data[RAS_CMAP_MAXSIZ];
	/* The color map data. */

} ras_cmap_t;

/******************************************************************************\
* Macros.
\******************************************************************************/

#define RAS_GETBLUE(x)	(((x) >> 16) & 0xff)
#define	RAS_GETGREEN(x)	(((x) >> 8) & 0xff)
#define RAS_GETRED(x)	((x) & 0xff)

#define RAS_BLUE(x)	(((x) & 0xff) << 16)
#define RAS_GREEN(x)	(((x) & 0xff) << 8)
#define	RAS_RED(x)	((x) & 0xff)

#define RAS_ROWSIZE(hdr) \
	((((hdr)->width * (hdr)->depth + 15) / 16) * 2)
#define RAS_ISRGB(hdr) \
	((hdr)->depth == 24 || (hdr)->depth == 32)

#define	RAS_ONES(n) \
	(((n) == 32) ? 0xffffffffUL : ((1UL << (n)) - 1))

#endif

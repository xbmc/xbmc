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
 * Windows Bitmap File Library
 *
 * $Id$
 */

#ifndef BMP_COD_H
#define BMP_COD_H

/******************************************************************************\
* Includes.
\******************************************************************************/

#include "jasper/jas_types.h"

/******************************************************************************\
* Constants and macros.
\******************************************************************************/

#define	BMP_MAGIC	0x4d42
/* The signature for a BMP file. */

#define	BMP_HDRLEN	14
/* The nominal header length. */

#define BMP_INFOLEN	40
/* The nominal info length. */

#define BMP_PALLEN(info)	((info)->numcolors * 4)
/* The length of the palette. */

#define	BMP_HASPAL(info)	((info)->numcolors > 0)
/* Is this a palettized image? */

/* Encoding types. */
#define	BMP_ENC_RGB		0 /* No special encoding. */
#define	BMP_ENC_RLE8	1 /* Run length encoding. */
#define	BMP_ENC_RLE4	2 /* Run length encoding. */

/******************************************************************************\
* Types.
\******************************************************************************/

/* BMP header. */
typedef struct {

	int_fast16_t magic;
	/* The signature (a.k.a. the magic number). */

	int_fast32_t siz;
	/* The size of the file in 32-bit words. */

	int_fast16_t reserved1;
	/* Ask Bill Gates what this is all about. */

	int_fast16_t reserved2;
	/* Ditto. */

	int_fast32_t off;
	/* The offset of the bitmap data from the bitmap file header in bytes. */

} bmp_hdr_t;

/* Palette entry. */
typedef struct {

	int_fast16_t red;
	/* The red component. */

	int_fast16_t grn;
	/* The green component. */

	int_fast16_t blu;
	/* The blue component. */

	int_fast16_t res;
	/* Reserved. */

} bmp_palent_t;

/* BMP info. */
typedef struct {

	int_fast32_t len;
	/* The length of the bitmap information header in bytes. */

	int_fast32_t width;
	/* The width of the bitmap in pixels. */

	int_fast32_t height;
	/* The height of the bitmap in pixels. */

	int_fast8_t topdown;
	/* The bitmap data is specified in top-down order. */

	int_fast16_t numplanes;
	/* The number of planes.  This must be set to a value of one. */

	int_fast16_t depth;
	/* The number of bits per pixel. */

	int_fast32_t enctype;
	/* The type of compression used. */

	int_fast32_t siz;
	/* The size of the image in bytes. */

	int_fast32_t hres;
	/* The horizontal resolution in pixels/metre. */

	int_fast32_t vres;
	/* The vertical resolution in pixels/metre. */

	int_fast32_t numcolors;
	/* The number of color indices used by the bitmap. */

	int_fast32_t mincolors;
	/* The number of color indices important for displaying the bitmap. */

	bmp_palent_t *palents;
	/* The colors should be listed in order of importance. */

} bmp_info_t;

/******************************************************************************\
* Functions and macros.
\******************************************************************************/

#define	bmp_issupported(hdr, info) \
	((hdr)->magic == BMP_MAGIC && !(hdr)->reserved1 && \
	  !(hdr)->reserved2 && (info)->numplanes == 1 && \
	  ((info)->depth == 8 || (info)->depth == 24) && \
	  (info)->enctype == BMP_ENC_RGB)
/* Is this type of BMP file supported? */

#define	bmp_haspal(info) \
	((info)->depth == 8)
/* Is there a palette? */

int bmp_numcmpts(bmp_info_t *info);
/* Get the number of components. */

bmp_info_t *bmp_info_create(void);
/* Create BMP information. */

void bmp_info_destroy(bmp_info_t *info);
/* Destroy BMP information. */

int bmp_isgrayscalepal(bmp_palent_t *palents, int numpalents);
/* Does the specified palette correspond to a grayscale image? */

#endif

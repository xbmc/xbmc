/*
 * Copyright (c) 1999-2000 Image Power, Inc. and the University of
 *   British Columbia.
 * Copyright (c) 2001-2002 Michael David Adams.
 * All rights reserved.
 */

/* __START_OF_JASPER_LICENSE__
 * 
 * JasPer Software License
 * 
 * IMAGE POWER JPEG-2000 PUBLIC LICENSE
 * ************************************
 * 
 * GRANT:
 * 
 * Permission is hereby granted, free of charge, to any person (the "User")
 * obtaining a copy of this software and associated documentation, to deal
 * in the JasPer Software without restriction, including without limitation
 * the right to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the JasPer Software (in source and binary forms),
 * and to permit persons to whom the JasPer Software is furnished to do so,
 * provided further that the License Conditions below are met.
 * 
 * License Conditions
 * ******************
 * 
 * A.  Redistributions of source code must retain the above copyright notice,
 * and this list of conditions, and the following disclaimer.
 * 
 * B.  Redistributions in binary form must reproduce the above copyright
 * notice, and this list of conditions, and the following disclaimer in
 * the documentation and/or other materials provided with the distribution.
 * 
 * C.  Neither the name of Image Power, Inc. nor any other contributor
 * (including, but not limited to, the University of British Columbia and
 * Michael David Adams) may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * 
 * D.  User agrees that it shall not commence any action against Image Power,
 * Inc., the University of British Columbia, Michael David Adams, or any
 * other contributors (collectively "Licensors") for infringement of any
 * intellectual property rights ("IPR") held by the User in respect of any
 * technology that User owns or has a right to license or sublicense and
 * which is an element required in order to claim compliance with ISO/IEC
 * 15444-1 (i.e., JPEG-2000 Part 1).  "IPR" means all intellectual property
 * rights worldwide arising under statutory or common law, and whether
 * or not perfected, including, without limitation, all (i) patents and
 * patent applications owned or licensable by User; (ii) rights associated
 * with works of authorship including copyrights, copyright applications,
 * copyright registrations, mask work rights, mask work applications,
 * mask work registrations; (iii) rights relating to the protection of
 * trade secrets and confidential information; (iv) any right analogous
 * to those set forth in subsections (i), (ii), or (iii) and any other
 * proprietary rights relating to intangible property (other than trademark,
 * trade dress, or service mark rights); and (v) divisions, continuations,
 * renewals, reissues and extensions of the foregoing (as and to the extent
 * applicable) now existing, hereafter filed, issued or acquired.
 * 
 * E.  If User commences an infringement action against any Licensor(s) then
 * such Licensor(s) shall have the right to terminate User's license and
 * all sublicenses that have been granted hereunder by User to other parties.
 * 
 * F.  This software is for use only in hardware or software products that
 * are compliant with ISO/IEC 15444-1 (i.e., JPEG-2000 Part 1).  No license
 * or right to this Software is granted for products that do not comply
 * with ISO/IEC 15444-1.  The JPEG-2000 Part 1 standard can be purchased
 * from the ISO.
 * 
 * THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL PART OF THIS LICENSE.
 * NO USE OF THE JASPER SOFTWARE IS AUTHORIZED HEREUNDER EXCEPT UNDER
 * THIS DISCLAIMER.  THE JASPER SOFTWARE IS PROVIDED BY THE LICENSORS AND
 * CONTRIBUTORS UNDER THIS LICENSE ON AN ``AS-IS'' BASIS, WITHOUT WARRANTY
 * OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION,
 * WARRANTIES THAT THE JASPER SOFTWARE IS FREE OF DEFECTS, IS MERCHANTABLE,
 * IS FIT FOR A PARTICULAR PURPOSE OR IS NON-INFRINGING.  THOSE INTENDING
 * TO USE THE JASPER SOFTWARE OR MODIFICATIONS THEREOF FOR USE IN HARDWARE
 * OR SOFTWARE PRODUCTS ARE ADVISED THAT THEIR USE MAY INFRINGE EXISTING
 * PATENTS, COPYRIGHTS, TRADEMARKS, OR OTHER INTELLECTUAL PROPERTY RIGHTS.
 * THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE JASPER SOFTWARE
 * IS WITH THE USER.  SHOULD ANY PART OF THE JASPER SOFTWARE PROVE DEFECTIVE
 * IN ANY RESPECT, THE USER (AND NOT THE INITIAL DEVELOPERS, THE UNIVERSITY
 * OF BRITISH COLUMBIA, IMAGE POWER, INC., MICHAEL DAVID ADAMS, OR ANY
 * OTHER CONTRIBUTOR) SHALL ASSUME THE COST OF ANY NECESSARY SERVICING,
 * REPAIR OR CORRECTION.  UNDER NO CIRCUMSTANCES AND UNDER NO LEGAL THEORY,
 * WHETHER TORT (INCLUDING NEGLIGENCE), CONTRACT, OR OTHERWISE, SHALL THE
 * INITIAL DEVELOPER, THE UNIVERSITY OF BRITISH COLUMBIA, IMAGE POWER, INC.,
 * MICHAEL DAVID ADAMS, ANY OTHER CONTRIBUTOR, OR ANY DISTRIBUTOR OF THE
 * JASPER SOFTWARE, OR ANY SUPPLIER OF ANY OF SUCH PARTIES, BE LIABLE TO
 * THE USER OR ANY OTHER PERSON FOR ANY INDIRECT, SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES OF ANY CHARACTER INCLUDING, WITHOUT LIMITATION,
 * DAMAGES FOR LOSS OF GOODWILL, WORK STOPPAGE, COMPUTER FAILURE OR
 * MALFUNCTION, OR ANY AND ALL OTHER COMMERCIAL DAMAGES OR LOSSES, EVEN IF
 * SUCH PARTY HAD BEEN INFORMED, OR OUGHT TO HAVE KNOWN, OF THE POSSIBILITY
 * OF SUCH DAMAGES.  THE JASPER SOFTWARE AND UNDERLYING TECHNOLOGY ARE NOT
 * FAULT-TOLERANT AND ARE NOT DESIGNED, MANUFACTURED OR INTENDED FOR USE OR
 * RESALE AS ON-LINE CONTROL EQUIPMENT IN HAZARDOUS ENVIRONMENTS REQUIRING
 * FAIL-SAFE PERFORMANCE, SUCH AS IN THE OPERATION OF NUCLEAR FACILITIES,
 * AIRCRAFT NAVIGATION OR COMMUNICATION SYSTEMS, AIR TRAFFIC CONTROL, DIRECT
 * LIFE SUPPORT MACHINES, OR WEAPONS SYSTEMS, IN WHICH THE FAILURE OF THE
 * JASPER SOFTWARE OR UNDERLYING TECHNOLOGY OR PRODUCT COULD LEAD DIRECTLY
 * TO DEATH, PERSONAL INJURY, OR SEVERE PHYSICAL OR ENVIRONMENTAL DAMAGE
 * ("HIGH RISK ACTIVITIES").  LICENSOR SPECIFICALLY DISCLAIMS ANY EXPRESS
 * OR IMPLIED WARRANTY OF FITNESS FOR HIGH RISK ACTIVITIES.  USER WILL NOT
 * KNOWINGLY USE, DISTRIBUTE OR RESELL THE JASPER SOFTWARE OR UNDERLYING
 * TECHNOLOGY OR PRODUCTS FOR HIGH RISK ACTIVITIES AND WILL ENSURE THAT ITS
 * CUSTOMERS AND END-USERS OF ITS PRODUCTS ARE PROVIDED WITH A COPY OF THE
 * NOTICE SPECIFIED IN THIS SECTION.
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

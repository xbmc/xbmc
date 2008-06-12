/*
 * Copyright (c) 1999-2000 Image Power, Inc. and the University of
 *   British Columbia.
 * Copyright (c) 2001-2003 Michael David Adams.
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

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <assert.h>

#include "jasper/jas_types.h"
#include "jasper/jas_stream.h"
#include "jasper/jas_image.h"
#include "jasper/jas_debug.h"

#include "bmp_enc.h"
#include "bmp_cod.h"

/******************************************************************************\
* Local prototypes.
\******************************************************************************/

static int bmp_puthdr(jas_stream_t *out, bmp_hdr_t *hdr);
static int bmp_putinfo(jas_stream_t *out, bmp_info_t *info);
static int bmp_putdata(jas_stream_t *out, bmp_info_t *info, jas_image_t *image, int *cmpts);
static int bmp_putint16(jas_stream_t *in, int_fast16_t val);
static int bmp_putint32(jas_stream_t *out, int_fast32_t val);

/******************************************************************************\
* Interface functions.
\******************************************************************************/

int bmp_encode(jas_image_t *image, jas_stream_t *out, char *optstr)
{
	jas_image_coord_t width;
	jas_image_coord_t height;
	int depth;
	int cmptno;
	bmp_hdr_t hdr;
	bmp_info_t *info;
	int_fast32_t datalen;
	int numpad;
	bmp_enc_t encbuf;
	bmp_enc_t *enc = &encbuf;
	jas_clrspc_t clrspc;

	if (optstr) {
		fprintf(stderr, "warning: ignoring BMP encoder options\n");
	}

	clrspc = jas_image_clrspc(image);
	switch (jas_clrspc_fam(clrspc)) {
	case JAS_CLRSPC_FAM_RGB:
		if (clrspc != JAS_CLRSPC_SRGB)
			jas_eprintf("warning: inaccurate color\n");
		break;
	case JAS_CLRSPC_FAM_GRAY:
		if (clrspc != JAS_CLRSPC_SGRAY)
			jas_eprintf("warning: inaccurate color\n");
		break;
	default:
		jas_eprintf("error: BMP format does not support color space\n");
		return -1;
		break;
	}

	switch (jas_clrspc_fam(clrspc)) {
	case JAS_CLRSPC_FAM_RGB:
		enc->numcmpts = 3;
		if ((enc->cmpts[0] = jas_image_getcmptbytype(image,
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_R))) < 0 ||
		  (enc->cmpts[1] = jas_image_getcmptbytype(image,
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_G))) < 0 ||
		  (enc->cmpts[2] = jas_image_getcmptbytype(image,
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_B))) < 0) {
			jas_eprintf("error: missing color component\n");
			return -1;
		}
		break;
	case JAS_CLRSPC_FAM_GRAY:
		enc->numcmpts = 1;
		if ((enc->cmpts[0] = jas_image_getcmptbytype(image,
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_GRAY_Y))) < 0) {
			jas_eprintf("error: missing color component\n");
			return -1;
		}
		break;
	default:
		abort();
		break;
	}

	width = jas_image_cmptwidth(image, enc->cmpts[0]);
	height = jas_image_cmptheight(image, enc->cmpts[0]);
	depth = jas_image_cmptprec(image, enc->cmpts[0]);

	/* Check to ensure that the image to be saved can actually be represented
	  using the BMP format. */
	for (cmptno = 0; cmptno < enc->numcmpts; ++cmptno) {
		if (jas_image_cmptwidth(image, enc->cmpts[cmptno]) != width ||
		  jas_image_cmptheight(image, enc->cmpts[cmptno]) != height ||
		  jas_image_cmptprec(image, enc->cmpts[cmptno]) != depth ||
		  jas_image_cmptsgnd(image, enc->cmpts[cmptno]) != false ||
		  jas_image_cmpttlx(image, enc->cmpts[cmptno]) != 0 ||
		  jas_image_cmpttly(image, enc->cmpts[cmptno]) != 0) {
			fprintf(stderr, "The BMP format cannot be used to represent an image with this geometry.\n");
			return -1;
		}
	}

	/* The component depths must be 1, 4, or 8. */
	if (depth != 1 && depth != 4 && depth != 8) {
		return -1;
	}

	numpad = (width * enc->numcmpts) % 4;
	if (numpad) {
		numpad = 4 - numpad;
	}
	datalen = (enc->numcmpts * width + numpad) * height;

	if (!(info = bmp_info_create())) {
		return -1;
	}
	info->len = BMP_INFOLEN;
	info->width = width;
	info->height = height;
	info->numplanes = 1;
	info->depth = enc->numcmpts * depth;
	info->enctype = BMP_ENC_RGB;
	info->siz = datalen;
	info->hres = 0;
	info->vres = 0;
	info->numcolors = (enc->numcmpts == 1) ? 256 : 0;
	info->mincolors = 0;

	hdr.magic = BMP_MAGIC;
	hdr.siz = BMP_HDRLEN + BMP_INFOLEN + 0 + datalen;
	hdr.off = BMP_HDRLEN + BMP_INFOLEN + BMP_PALLEN(info);

	/* Write the bitmap header. */
	if (bmp_puthdr(out, &hdr)) {
		return -1;
	}

	/* Write the bitmap information. */
	if (bmp_putinfo(out, info)) {
		return -1;
	}

	/* Write the bitmap data. */
	if (bmp_putdata(out, info, image, enc->cmpts)) {
		return -1;
	}

	bmp_info_destroy(info);

	return 0;
}

/******************************************************************************\
* Code for aggregate types.
\******************************************************************************/

static int bmp_puthdr(jas_stream_t *out, bmp_hdr_t *hdr)
{
	assert(hdr->magic == BMP_MAGIC);
	if (bmp_putint16(out, hdr->magic) || bmp_putint32(out, hdr->siz) ||
	  bmp_putint32(out, 0) || bmp_putint32(out, hdr->off)) {
		return -1;
	}
	return 0;
}

static int bmp_putinfo(jas_stream_t *out, bmp_info_t *info)
{
	int i;

	info->len = 40;
	if (bmp_putint32(out, info->len) ||
	  bmp_putint32(out, info->width) ||
	  bmp_putint32(out, info->height) ||
	  bmp_putint16(out, info->numplanes) ||
	  bmp_putint16(out, info->depth) ||
	  bmp_putint32(out, info->enctype) ||
	  bmp_putint32(out, info->siz) ||
	  bmp_putint32(out, info->hres) ||
	  bmp_putint32(out, info->vres) ||
	  bmp_putint32(out, info->numcolors) ||
	  bmp_putint32(out, info->mincolors)) {
		return -1;
	}

	for (i = 0; i < info->numcolors; ++i)
	{
		if (jas_stream_putc(out, i) == EOF ||
		  jas_stream_putc(out, i) == EOF ||
		  jas_stream_putc(out, i) == EOF ||
		  jas_stream_putc(out, 0) == EOF)
		{
			return -1;
		}
	}

	return 0;
}

static int bmp_putdata(jas_stream_t *out, bmp_info_t *info, jas_image_t *image,
  int *cmpts)
{
	int i;
	int j;
	jas_matrix_t *bufs[3];
	int numpad;
	unsigned char red;
	unsigned char grn;
	unsigned char blu;
	int ret;
	int numcmpts;
	int v;
	int cmptno;

	numcmpts = (info->depth == 24) ? 3:1;

	/* We do not support palettized images. */
	if (BMP_HASPAL(info) && numcmpts == 3) {
		fprintf(stderr, "no palettized image support for BMP format\n");
		return -1;
	}

	ret = 0;
	for (i = 0; i < numcmpts; ++i) {
		bufs[i] = 0;
	}

	/* Create temporary matrices to hold component data. */
	for (i = 0; i < numcmpts; ++i) {
		if (!(bufs[i] = jas_matrix_create(1, info->width))) {
			ret = -1;
			goto bmp_putdata_done;
		}
	}

	/* Calculate number of padding bytes per row of image data. */
	numpad = (numcmpts * info->width) % 4;
	if (numpad) {
		numpad = 4 - numpad;
	}

	/* Put the image data. */
	for (i = info->height - 1; i >= 0; --i) {
		for (cmptno = 0; cmptno < numcmpts; ++cmptno) {
			if (jas_image_readcmpt(image, cmptno, 0, i, info->width,
			  1, bufs[cmpts[cmptno]])) {
				ret = -1;
				goto bmp_putdata_done;
			}
		}
		for (j = 0; j < info->width; ++j) {
			if (numcmpts == 3) {
				red = (jas_matrix_getv(bufs[0], j));
				grn = (jas_matrix_getv(bufs[1], j));
				blu = (jas_matrix_getv(bufs[2], j));
				if (jas_stream_putc(out, blu) == EOF ||
				  jas_stream_putc(out, grn) == EOF ||
				  jas_stream_putc(out, red) == EOF) {
					ret = -1;
					goto bmp_putdata_done;
				}
			} else if (numcmpts == 1) {
				v = (jas_matrix_getv(bufs[cmpts[0]], j));
				if (jas_stream_putc(out, v) == EOF) {
					ret = -1;
					goto bmp_putdata_done;
				}
			} else {
				abort();
			}
		}
		for (j = numpad; j > 0; --j) {
			if (jas_stream_putc(out, 0) == EOF) {
				ret = -1;
				goto bmp_putdata_done;
			}
		}
	}

bmp_putdata_done:
	/* Destroy the temporary matrices. */
	for (i = 0; i < numcmpts; ++i) {
		if (bufs[i]) {
			jas_matrix_destroy(bufs[i]);
		}
	}

	return ret;
}

/******************************************************************************\
* Code for primitive types.
\******************************************************************************/

static int bmp_putint16(jas_stream_t *in, int_fast16_t val)
{
	if (jas_stream_putc(in, val & 0xff) == EOF || jas_stream_putc(in, (val >> 8) &
	  0xff) == EOF) {
		return -1;
	}
	return 0;
}

static int bmp_putint32(jas_stream_t *out, int_fast32_t val)
{
	int n;
	int_fast32_t v;

	/* This code needs to be changed if we want to handle negative values. */
	assert(val >= 0);
	v = val;
	for (n = 4;;) {
		if (jas_stream_putc(out, v & 0xff) == EOF) {
			return -1;
		}
		if (--n <= 0) {
			break;
		}
		v >>= 8;
	}
	return 0;
}

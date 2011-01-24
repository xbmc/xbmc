/*
 * Copyright (c) 1999-2000 Image Power, Inc. and the University of
 *   British Columbia.
 * Copyright (c) 2001-2003 Michael David Adams.
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
		jas_eprintf("warning: ignoring BMP encoder options\n");
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
			jas_eprintf("The BMP format cannot be used to represent an image with this geometry.\n");
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
		jas_eprintf("no palettized image support for BMP format\n");
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

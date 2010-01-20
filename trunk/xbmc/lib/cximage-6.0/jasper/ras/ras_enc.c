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
 * Sun Rasterfile Library
 *
 * $Id$
 */

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <assert.h>
#include <stdlib.h>

#include "jasper/jas_image.h"
#include "jasper/jas_stream.h"
#include "jasper/jas_debug.h"

#include "ras_cod.h"
#include "ras_enc.h"

/******************************************************************************\
* Prototypes.
\******************************************************************************/

static int ras_puthdr(jas_stream_t *out, ras_hdr_t *hdr);
static int ras_putint(jas_stream_t *out, int val);

static int ras_putdata(jas_stream_t *out, ras_hdr_t *hdr, jas_image_t *image, int numcmpts, int *cmpts);
static int ras_putdatastd(jas_stream_t *out, ras_hdr_t *hdr, jas_image_t *image, int numcmpts, int *cmpts);

/******************************************************************************\
* Code.
\******************************************************************************/

int ras_encode(jas_image_t *image, jas_stream_t *out, char *optstr)
{
	int_fast32_t width;
	int_fast32_t height;
	int_fast32_t depth;
	int cmptno;
#if 0
	uint_fast16_t numcmpts;
#endif
	int i;
	ras_hdr_t hdr;
	int rowsize;
	ras_enc_t encbuf;
	ras_enc_t *enc = &encbuf;

	if (optstr) {
		jas_eprintf("warning: ignoring RAS encoder options\n");
	}

	switch (jas_clrspc_fam(jas_image_clrspc(image))) {
	case JAS_CLRSPC_FAM_RGB:
		if (jas_image_clrspc(image) != JAS_CLRSPC_SRGB)
			jas_eprintf("warning: inaccurate color\n");
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
		if (jas_image_clrspc(image) != JAS_CLRSPC_SGRAY)
			jas_eprintf("warning: inaccurate color\n");
		enc->numcmpts = 1;
		if ((enc->cmpts[0] = jas_image_getcmptbytype(image,
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_GRAY_Y))) < 0) {
			jas_eprintf("error: missing color component\n");
			return -1;
		}
		break;
	default:
		jas_eprintf("error: unsupported color space\n");
		return -1;
		break;
	}

	width = jas_image_cmptwidth(image, enc->cmpts[0]);
	height = jas_image_cmptheight(image, enc->cmpts[0]);
	depth = jas_image_cmptprec(image, enc->cmpts[0]);

	for (cmptno = 0; cmptno < enc->numcmpts; ++cmptno) {
		if (jas_image_cmptwidth(image, enc->cmpts[cmptno]) != width ||
		  jas_image_cmptheight(image, enc->cmpts[cmptno]) != height ||
		  jas_image_cmptprec(image, enc->cmpts[cmptno]) != depth ||
		  jas_image_cmptsgnd(image, enc->cmpts[cmptno]) != false ||
		  jas_image_cmpttlx(image, enc->cmpts[cmptno]) != 0 ||
		  jas_image_cmpttly(image, enc->cmpts[cmptno]) != 0) {
			jas_eprintf("The RAS format cannot be used to represent an image with this geometry.\n");
			return -1;
		}
	}

	/* Ensure that the image can be encoded in the desired format. */
	if (enc->numcmpts == 3) {

		/* All three components must have the same subsampling
		  factor and have a precision of eight bits. */
		if (enc->numcmpts > 1) {
			for (i = 0; i < enc->numcmpts; ++i) {
				if (jas_image_cmptprec(image, enc->cmpts[i]) != 8) {
					return -1;
				}
			}
		}
	} else if (enc->numcmpts == 1) {
		/* NOP */
	} else {
		return -1;
	}

	hdr.magic = RAS_MAGIC;
	hdr.width = width;
	hdr.height = height;
	hdr.depth = (enc->numcmpts == 3) ? 24 : depth;

	rowsize = RAS_ROWSIZE(&hdr);
	hdr.length = rowsize * hdr.height;
	hdr.type = RAS_TYPE_STD;

	hdr.maptype = RAS_MT_NONE;
	hdr.maplength = 0;

	if (ras_puthdr(out, &hdr)) {
		return -1;
	}

	if (ras_putdata(out, &hdr, image, enc->numcmpts, enc->cmpts)) {
		return -1;
	}

	return 0;
}

static int ras_putdata(jas_stream_t *out, ras_hdr_t *hdr, jas_image_t *image, int numcmpts, int *cmpts)
{
	int ret;

	switch (hdr->type) {
	case RAS_TYPE_STD:
		ret = ras_putdatastd(out, hdr, image, numcmpts, cmpts);
		break;
	default:
		ret = -1;
		break;
	}
	return ret;
}

static int ras_putdatastd(jas_stream_t *out, ras_hdr_t *hdr, jas_image_t *image,  int numcmpts, int *cmpts)
{
	int rowsize;
	int pad;
	unsigned int z;
	int nz;
	int c;
	int x;
	int y;
	int v;
	jas_matrix_t *data[3];
	int i;

	for (i = 0; i < numcmpts; ++i) {
		data[i] = jas_matrix_create(jas_image_height(image), jas_image_width(image));
		assert(data[i]);
	}

	rowsize = RAS_ROWSIZE(hdr);
	pad = rowsize - (hdr->width * hdr->depth + 7) / 8;

	hdr->length = hdr->height * rowsize;

	for (y = 0; y < hdr->height; y++) {
		for (i = 0; i < numcmpts; ++i) {
			jas_image_readcmpt(image, cmpts[i], 0, y, jas_image_width(image),
			  1, data[i]);
		}
		z = 0;
		nz = 0;
		for (x = 0; x < hdr->width; x++) {
			z <<= hdr->depth;
			if (RAS_ISRGB(hdr)) {
				v = RAS_RED((jas_matrix_getv(data[0], x))) |
				  RAS_GREEN((jas_matrix_getv(data[1], x))) |
				  RAS_BLUE((jas_matrix_getv(data[2], x)));
			} else {
				v = (jas_matrix_getv(data[0], x));
			}
			z |= v & RAS_ONES(hdr->depth);
			nz += hdr->depth;
			while (nz >= 8) {
				c = (z >> (nz - 8)) & 0xff;
				if (jas_stream_putc(out, c) == EOF) {
					return -1;
				}
				nz -= 8;
				z &= RAS_ONES(nz);
			}
		}
		if (nz > 0) {
			c = (z >> (8 - nz)) & RAS_ONES(nz);
			if (jas_stream_putc(out, c) == EOF) {
				return -1;
			}
		}
		if (pad % 2) {
			if (jas_stream_putc(out, 0) == EOF) {
				return -1;
			}
		}
	}

	for (i = 0; i < numcmpts; ++i) {
		jas_matrix_destroy(data[i]);
	}

	return 0;
}

static int ras_puthdr(jas_stream_t *out, ras_hdr_t *hdr)
{
	if (ras_putint(out, RAS_MAGIC) || ras_putint(out, hdr->width) ||
	  ras_putint(out, hdr->height) || ras_putint(out, hdr->depth) ||
	  ras_putint(out, hdr->length) || ras_putint(out, hdr->type) ||
	  ras_putint(out, hdr->maptype) || ras_putint(out, hdr->maplength)) {
		return -1;
	}

	return 0;
}

static int ras_putint(jas_stream_t *out, int val)
{
	int x;
	int i;
	int c;

	x = val;
	for (i = 0; i < 4; i++) {
		c = (x >> 24) & 0xff;
		if (jas_stream_putc(out, c) == EOF) {
			return -1;
		}
		x <<= 8;
	}

	return 0;
}

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
#include "jasper/jas_malloc.h"

#include "bmp_cod.h"

/******************************************************************************\
* Local prototypes.
\******************************************************************************/

static int bmp_gethdr(jas_stream_t *in, bmp_hdr_t *hdr);
static bmp_info_t *bmp_getinfo(jas_stream_t *in);
static int bmp_getdata(jas_stream_t *in, bmp_info_t *info, jas_image_t *image);
static int bmp_getint16(jas_stream_t *in, int_fast16_t *val);
static int bmp_getint32(jas_stream_t *in, int_fast32_t *val);
static int bmp_gobble(jas_stream_t *in, long n);

/******************************************************************************\
* Interface functions.
\******************************************************************************/

jas_image_t *bmp_decode(jas_stream_t *in, char *optstr)
{
	jas_image_t *image;
	bmp_hdr_t hdr;
	bmp_info_t *info;
	uint_fast16_t cmptno;
	jas_image_cmptparm_t cmptparms[3];
	jas_image_cmptparm_t *cmptparm;
	uint_fast16_t numcmpts;
	long n;

	if (optstr) {
		jas_eprintf("warning: ignoring BMP decoder options\n");
	}

	jas_eprintf(
	  "THE BMP FORMAT IS NOT FULLY SUPPORTED!\n"
	  "THAT IS, THE JASPER SOFTWARE CANNOT DECODE ALL TYPES OF BMP DATA.\n"
	  "IF YOU HAVE ANY PROBLEMS, PLEASE TRY CONVERTING YOUR IMAGE DATA\n"
	  "TO THE PNM FORMAT, AND USING THIS FORMAT INSTEAD.\n"
	  );

	/* Read the bitmap header. */
	if (bmp_gethdr(in, &hdr)) {
		jas_eprintf("cannot get header\n");
		return 0;
	}

	/* Read the bitmap information. */
	if (!(info = bmp_getinfo(in))) {
		jas_eprintf("cannot get info\n");
		return 0;
	}

	/* Ensure that we support this type of BMP file. */
	if (!bmp_issupported(&hdr, info)) {
		jas_eprintf("error: unsupported BMP encoding\n");
		bmp_info_destroy(info);
		return 0;
	}

	/* Skip over any useless data between the end of the palette
	  and start of the bitmap data. */
	if ((n = hdr.off - (BMP_HDRLEN + BMP_INFOLEN + BMP_PALLEN(info))) < 0) {
		jas_eprintf("error: possibly bad bitmap offset?\n");
		return 0;
	}
	if (n > 0) {
		jas_eprintf("skipping unknown data in BMP file\n");
		if (bmp_gobble(in, n)) {
			bmp_info_destroy(info);
			return 0;
		}
	}

	/* Get the number of components. */
	numcmpts = bmp_numcmpts(info);

	for (cmptno = 0, cmptparm = cmptparms; cmptno < numcmpts; ++cmptno,
	  ++cmptparm) {
		cmptparm->tlx = 0;
		cmptparm->tly = 0;
		cmptparm->hstep = 1;
		cmptparm->vstep = 1;
		cmptparm->width = info->width;
		cmptparm->height = info->height;
		cmptparm->prec = 8;
		cmptparm->sgnd = false;
	}

	/* Create image object. */
	if (!(image = jas_image_create(numcmpts, cmptparms,
	  JAS_CLRSPC_UNKNOWN))) {
		bmp_info_destroy(info);
		return 0;
	}

	if (numcmpts == 3) {
		jas_image_setclrspc(image, JAS_CLRSPC_SRGB);
		jas_image_setcmpttype(image, 0,
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_R));
		jas_image_setcmpttype(image, 1,
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_G));
		jas_image_setcmpttype(image, 2,
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_B));
	} else {
		jas_image_setclrspc(image, JAS_CLRSPC_SGRAY);
		jas_image_setcmpttype(image, 0,
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_GRAY_Y));
	}

	/* Read the bitmap data. */
	if (bmp_getdata(in, info, image)) {
		bmp_info_destroy(info);
		jas_image_destroy(image);
		return 0;
	}

	bmp_info_destroy(info);

	return image;
}

int bmp_validate(jas_stream_t *in)
{
	int n;
	int i;
	uchar buf[2];

	assert(JAS_STREAM_MAXPUTBACK >= 2);

	/* Read the first two characters that constitute the signature. */
	if ((n = jas_stream_read(in, (char *) buf, 2)) < 0) {
		return -1;
	}
	/* Put the characters read back onto the stream. */
	for (i = n - 1; i >= 0; --i) {
		if (jas_stream_ungetc(in, buf[i]) == EOF) {
			return -1;
		}
	}
	/* Did we read enough characters? */
	if (n < 2) {
		return -1;
	}
	/* Is the signature correct for the BMP format? */
	if (buf[0] == (BMP_MAGIC & 0xff) && buf[1] == (BMP_MAGIC >> 8)) {
		return 0;
	}
	return -1;
}

/******************************************************************************\
* Code for aggregate types.
\******************************************************************************/

static int bmp_gethdr(jas_stream_t *in, bmp_hdr_t *hdr)
{
	if (bmp_getint16(in, &hdr->magic) || hdr->magic != BMP_MAGIC ||
	  bmp_getint32(in, &hdr->siz) || bmp_getint16(in, &hdr->reserved1) ||
	  bmp_getint16(in, &hdr->reserved2) || bmp_getint32(in, &hdr->off)) {
		return -1;
	}
	return 0;
}

static bmp_info_t *bmp_getinfo(jas_stream_t *in)
{
	bmp_info_t *info;
	int i;
	bmp_palent_t *palent;

	if (!(info = bmp_info_create())) {
		return 0;
	}

	if (bmp_getint32(in, &info->len) || info->len != 40 ||
	  bmp_getint32(in, &info->width) || bmp_getint32(in, &info->height) ||
	  bmp_getint16(in, &info->numplanes) ||
	  bmp_getint16(in, &info->depth) || bmp_getint32(in, &info->enctype) ||
	  bmp_getint32(in, &info->siz) ||
	  bmp_getint32(in, &info->hres) || bmp_getint32(in, &info->vres) ||
	  bmp_getint32(in, &info->numcolors) ||
	  bmp_getint32(in, &info->mincolors)) {
		bmp_info_destroy(info);
		return 0;
	}

	if (info->height < 0) {
		info->topdown = 1;
		info->height = -info->height;
	} else {
		info->topdown = 0;
	}

	if (info->width <= 0 || info->height <= 0 || info->numplanes <= 0 ||
	  info->depth <= 0  || info->numcolors < 0 || info->mincolors < 0) {
		bmp_info_destroy(info);
		return 0;
	}

	if (info->enctype != BMP_ENC_RGB) {
		jas_eprintf("unsupported BMP encoding\n");
		bmp_info_destroy(info);
		return 0;
	}

	if (info->numcolors > 0) {
		if (!(info->palents = jas_malloc(info->numcolors *
		  sizeof(bmp_palent_t)))) {
			bmp_info_destroy(info);
			return 0;
		}
	} else {
		info->palents = 0;
	}

	for (i = 0; i < info->numcolors; ++i) {
		palent = &info->palents[i];
		if ((palent->blu = jas_stream_getc(in)) == EOF ||
		  (palent->grn = jas_stream_getc(in)) == EOF ||
		  (palent->red = jas_stream_getc(in)) == EOF ||
		  (palent->res = jas_stream_getc(in)) == EOF) {
			bmp_info_destroy(info);
			return 0;
		}
	}

	return info;
}

static int bmp_getdata(jas_stream_t *in, bmp_info_t *info, jas_image_t *image)
{
	int i;
	int j;
	int y;
	jas_matrix_t *cmpts[3];
	int numpad;
	int red;
	int grn;
	int blu;
	int ret;
	int numcmpts;
	int cmptno;
	int ind;
	bmp_palent_t *palent;
	int mxind;
	int haspal;

	assert(info->depth == 8 || info->depth == 24);
	assert(info->enctype == BMP_ENC_RGB);

	numcmpts = bmp_numcmpts(info);
	haspal = bmp_haspal(info);

	ret = 0;
	for (i = 0; i < numcmpts; ++i) {
		cmpts[i] = 0;
	}

	/* Create temporary matrices to hold component data. */
	for (i = 0; i < numcmpts; ++i) {
		if (!(cmpts[i] = jas_matrix_create(1, info->width))) {
			ret = -1;
			goto bmp_getdata_done;
		}
	}

	/* Calculate number of padding bytes per row of image data. */
	numpad = (numcmpts * info->width) % 4;
	if (numpad) {
		numpad = 4 - numpad;
	}

	mxind = (1 << info->depth) - 1;
	for (i = 0; i < info->height; ++i) {
		for (j = 0; j < info->width; ++j) {
			if (haspal) {
				if ((ind = jas_stream_getc(in)) == EOF) {
					ret = -1;
					goto bmp_getdata_done;
				}
				if (ind > mxind) {
					ret = -1;
					goto bmp_getdata_done;
				}
				if (ind < info->numcolors) {
					palent = &info->palents[ind];
					red = palent->red;
					grn = palent->grn;
					blu = palent->blu;
				} else {
					red = ind;
					grn = ind;
					blu = ind;
				}
			} else {
				if ((blu = jas_stream_getc(in)) == EOF ||
				  (grn = jas_stream_getc(in)) == EOF ||
				  (red = jas_stream_getc(in)) == EOF) {
					ret = -1;
					goto bmp_getdata_done;
				}
			}
			if (numcmpts == 3) {
				jas_matrix_setv(cmpts[0], j, red);
				jas_matrix_setv(cmpts[1], j, grn);
				jas_matrix_setv(cmpts[2], j, blu);
			} else {
				jas_matrix_setv(cmpts[0], j, red);
			}
		}
		for (j = numpad; j > 0; --j) {
				if (jas_stream_getc(in) == EOF) {
					ret = -1;
					goto bmp_getdata_done;
				}
		}
		for (cmptno = 0; cmptno < numcmpts; ++cmptno) {
			y = info->topdown ? i : (info->height - 1 - i);
			if (jas_image_writecmpt(image, cmptno, 0, y, info->width,
			  1, cmpts[cmptno])) {
				ret = -1;
				goto bmp_getdata_done;
			}
		}
	}

bmp_getdata_done:
	/* Destroy the temporary matrices. */
	for (i = 0; i < numcmpts; ++i) {
		if (cmpts[i]) {
			jas_matrix_destroy(cmpts[i]);
		}
	}

	return ret;
}

/******************************************************************************\
* Code for primitive types.
\******************************************************************************/

static int bmp_getint16(jas_stream_t *in, int_fast16_t *val)
{
	int lo;
	int hi;
	if ((lo = jas_stream_getc(in)) == EOF || (hi = jas_stream_getc(in)) == EOF) {
		return -1;
	}
	if (val) {
		*val = (hi << 8) | lo;
	}
	return 0;
}

static int bmp_getint32(jas_stream_t *in, int_fast32_t *val)
{
	int n;
	uint_fast32_t v;
	int c;
	for (n = 4, v = 0;;) {
		if ((c = jas_stream_getc(in)) == EOF) {
			return -1;
		}
		v |= (c << 24);
		if (--n <= 0) {
			break;
		}
		v >>= 8;
	}
	if (val) {
		*val = v;
	}
	return 0;
}

static int bmp_gobble(jas_stream_t *in, long n)
{
	while (--n >= 0) {
		if (jas_stream_getc(in) == EOF) {
			return -1;
		}
	}
	return 0;
}

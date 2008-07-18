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
 * Portable Pixmap/Graymap Format Support
 *
 * $Id$
 */

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

#include "jasper/jas_types.h"
#include "jasper/jas_stream.h"
#include "jasper/jas_image.h"

#include "pnm_cod.h"

/******************************************************************************\
* Local function prototypes.
\******************************************************************************/

static int pnm_gethdr(jas_stream_t *in, pnm_hdr_t *hdr);
static int pnm_getdata(jas_stream_t *in, pnm_hdr_t *hdr, jas_image_t *image);

static int pnm_getsintstr(jas_stream_t *in, int_fast32_t *val);
static int pnm_getuintstr(jas_stream_t *in, uint_fast32_t *val);
static int pnm_getbitstr(jas_stream_t *in, int *val);
static int pnm_getc(jas_stream_t *in);

static int pnm_getsint(jas_stream_t *in, int wordsize, int_fast32_t *val);
static int pnm_getuint(jas_stream_t *in, int wordsize, uint_fast32_t *val);
static int pnm_getint16(jas_stream_t *in, int *val);
#define	pnm_getuint32(in, val)	pnm_getuint(in, 32, val)

/******************************************************************************\
* Local data.
\******************************************************************************/

static int pnm_allowtrunc = 1;

/******************************************************************************\
* Load function.
\******************************************************************************/

jas_image_t *pnm_decode(jas_stream_t *in, char *opts)
{
	pnm_hdr_t hdr;
	jas_image_t *image;
	jas_image_cmptparm_t cmptparms[3];
	jas_image_cmptparm_t *cmptparm;
	int i;

	if (opts) {
		jas_eprintf("warning: ignoring options\n");
	}

	/* Read the file header. */
	if (pnm_gethdr(in, &hdr)) {
		return 0;
	}

	/* Create an image of the correct size. */
	for (i = 0, cmptparm = cmptparms; i < hdr.numcmpts; ++i, ++cmptparm) {
		cmptparm->tlx = 0;
		cmptparm->tly = 0;
		cmptparm->hstep = 1;
		cmptparm->vstep = 1;
		cmptparm->width = hdr.width;
		cmptparm->height = hdr.height;
		cmptparm->prec = pnm_maxvaltodepth(hdr.maxval);
		cmptparm->sgnd = hdr.sgnd;
	}
	if (!(image = jas_image_create(hdr.numcmpts, cmptparms, JAS_CLRSPC_UNKNOWN))) {
		return 0;
	}

	if (hdr.numcmpts == 3) {
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

	/* Read image data from stream into image. */
	if (pnm_getdata(in, &hdr, image)) {
		jas_image_destroy(image);
		return 0;
	}

	return image;
}

/******************************************************************************\
* Validation function.
\******************************************************************************/

int pnm_validate(jas_stream_t *in)
{
	uchar buf[2];
	int i;
	int n;

	assert(JAS_STREAM_MAXPUTBACK >= 2);

	/* Read the first two characters that constitute the signature. */
	if ((n = jas_stream_read(in, buf, 2)) < 0) {
		return -1;
	}
	/* Put these characters back to the stream. */
	for (i = n - 1; i >= 0; --i) {
		if (jas_stream_ungetc(in, buf[i]) == EOF) {
			return -1;
		}
	}
	/* Did we read enough data? */
	if (n < 2) {
		return -1;
	}
	/* Is this the correct signature for a PNM file? */
	if (buf[0] == 'P' && isdigit(buf[1])) {
		return 0;
	}
	return -1;
}

/******************************************************************************\
* Functions for reading the header.
\******************************************************************************/

static int pnm_gethdr(jas_stream_t *in, pnm_hdr_t *hdr)
{
	int_fast32_t maxval;
	int_fast32_t width;
	int_fast32_t height;
	if (pnm_getint16(in, &hdr->magic) || pnm_getsintstr(in, &width) ||
	  pnm_getsintstr(in, &height)) {
		return -1;
	}
	hdr->width = width;
	hdr->height = height;
	if (pnm_type(hdr->magic) != PNM_TYPE_PBM) {
		if (pnm_getsintstr(in, &maxval)) {
			return -1;
		}
	} else {
		maxval = 1;
	}
	if (maxval < 0) {
		hdr->maxval = -maxval;
		hdr->sgnd = true;
	} else {
		hdr->maxval = maxval;
		hdr->sgnd = false;
	}

	switch (pnm_type(hdr->magic)) {
	case PNM_TYPE_PBM:
	case PNM_TYPE_PGM:
		hdr->numcmpts = 1;
		break;
	case PNM_TYPE_PPM:
		hdr->numcmpts = 3;
		break;
	default:
		abort();
		break;
	}

	return 0;
}

/******************************************************************************\
* Functions for processing the sample data.
\******************************************************************************/

static int pnm_getdata(jas_stream_t *in, pnm_hdr_t *hdr, jas_image_t *image)
{
	int ret;
#if 0
	int numcmpts;
#endif
	int cmptno;
	int fmt;
	jas_matrix_t *data[3];
	int x;
	int y;
	int_fast64_t v;
	int depth;
	int type;
	int c;
	int n;

	ret = -1;

#if 0
	numcmpts = jas_image_numcmpts(image);
#endif
	fmt = pnm_fmt(hdr->magic);
	type = pnm_type(hdr->magic);
	depth = pnm_maxvaltodepth(hdr->maxval);

	data[0] = 0;
	data[1] = 0;
	data[2] = 0;
	for (cmptno = 0; cmptno < hdr->numcmpts; ++cmptno) {
		if (!(data[cmptno] = jas_matrix_create(1, hdr->width))) {
			goto done;
		}
	}

	for (y = 0; y < hdr->height; ++y) {
		if (type == PNM_TYPE_PBM) {
			if (fmt == PNM_FMT_BIN) {
				for (x = 0; x < hdr->width;) {
					if ((c = jas_stream_getc(in)) == EOF) {
						goto done;
					}
					n = 8;
					while (n > 0 && x < hdr->width) {
						jas_matrix_set(data[0], 0, x, 1 - ((c >> 7) & 1));
						c <<= 1;
						--n;
						++x;
					}
				}
			} else {
				for (x = 0; x < hdr->width; ++x) {
					int uv;
					if (pnm_getbitstr(in, &uv)) {
						goto done;
					}
					jas_matrix_set(data[0], 0, x, 1 - uv);
				}
			}
		} else {
			for (x = 0; x < hdr->width; ++x) {
				for (cmptno = 0; cmptno < hdr->numcmpts; ++cmptno) {
					if (fmt == PNM_FMT_BIN) {
						/* The sample data is in binary format. */
						if (hdr->sgnd) {
							/* The sample data is signed. */
							int_fast32_t sv;
							if (pnm_getsint(in, depth, &sv)) {
								if (!pnm_allowtrunc) {
									goto done;
								}
								sv = 0;
							}
							v = sv;
						} else {
							/* The sample data is unsigned. */
							uint_fast32_t uv;
							if (pnm_getuint(in, depth, &uv)) {
								if (!pnm_allowtrunc) {
									goto done;
								}
								uv = 0;
							}
							v = uv;
						}
					} else {
						/* The sample data is in text format. */
						if (hdr->sgnd) {
							/* The sample data is signed. */
							int_fast32_t sv;
							if (pnm_getsintstr(in, &sv)) {
								if (!pnm_allowtrunc) {
									goto done;
								}
								sv = 0;
							}
							v = sv;
						} else {
							/* The sample data is unsigned. */
							uint_fast32_t uv;
							if (pnm_getuintstr(in, &uv)) {
								if (!pnm_allowtrunc) {
									goto done;
								}
								uv = 0;
							}
							v = uv;
						}
					}
					jas_matrix_set(data[cmptno], 0, x, v);
				}
			}
		}
		for (cmptno = 0; cmptno < hdr->numcmpts; ++cmptno) {
			if (jas_image_writecmpt(image, cmptno, 0, y, hdr->width, 1,
			  data[cmptno])) {
				goto done;
			}
		}
	}

	ret = 0;

done:

	for (cmptno = 0; cmptno < hdr->numcmpts; ++cmptno) {
		if (data[cmptno]) {
			jas_matrix_destroy(data[cmptno]);
		}
	}

	return ret;
}

/******************************************************************************\
* Miscellaneous functions.
\******************************************************************************/

static int pnm_getsint(jas_stream_t *in, int wordsize, int_fast32_t *val)
{
	uint_fast32_t tmpval;

	if (pnm_getuint(in, wordsize, &tmpval)) {
		return -1;
	}
	if (val) {
		assert((tmpval & (1 << (wordsize - 1))) == 0);
		*val = tmpval;
	}

	return 0;
}

static int pnm_getuint(jas_stream_t *in, int wordsize, uint_fast32_t *val)
{
	uint_fast32_t tmpval;
	int c;
	int n;

	tmpval = 0;
	n = (wordsize + 7) / 8;
	while (--n >= 0) {
		if ((c = jas_stream_getc(in)) == EOF) {
			return -1;
		}
		tmpval = (tmpval << 8) | c;
	}
	tmpval &= (((uint_fast64_t) 1) << wordsize) - 1;
	if (val) {
		*val = tmpval;
	}

	return 0;
}

static int pnm_getbitstr(jas_stream_t *in, int *val)
{
	int c;
	int_fast32_t v;
	for (;;) {
		if ((c = pnm_getc(in)) == EOF) {
			return -1;
		}
		if (c == '#') {
			for (;;) {
				if ((c = pnm_getc(in)) == EOF) {
					return -1;
				}
				if (c == '\n') {
					break;
				}
			}
		} else if (c == '0' || c == '1') {
			v = c - '0';
			break;
		}
	}
	if (val) {
		*val = v;
	}
	return 0;
}

static int pnm_getuintstr(jas_stream_t *in, uint_fast32_t *val)
{
	uint_fast32_t v;
	int c;

	/* Discard any leading whitespace. */
	do {
		if ((c = pnm_getc(in)) == EOF) {
			return -1;
		}
	} while (isspace(c));

	/* Parse the number. */
	v = 0;
	while (isdigit(c)) {
		v = 10 * v + c - '0';
		if ((c = pnm_getc(in)) < 0) {
			return -1;
		}
	}

	/* The number must be followed by whitespace. */
	if (!isspace(c)) {
		return -1;
	}

	if (val) {
		*val = v;
	}
	return 0;
}

static int pnm_getsintstr(jas_stream_t *in, int_fast32_t *val)
{
	int c;
	int s;
	int_fast32_t v;

	/* Discard any leading whitespace. */
	do {
		if ((c = pnm_getc(in)) == EOF) {
			return -1;
		}
	} while (isspace(c));

	/* Get the number, allowing for a negative sign. */
	s = 1;
	if (c == '-') {
		s = -1;
		if ((c = pnm_getc(in)) == EOF) {
			return -1;
		}
	} else if (c == '+') {
		if ((c = pnm_getc(in)) == EOF) {
			return -1;
		}
	}
	v = 0;
	while (isdigit(c)) {
		v = 10 * v + c - '0';
		if ((c = pnm_getc(in)) < 0) {
			return -1;
		}
	}

	/* The number must be followed by whitespace. */
	if (!isspace(c)) {
		return -1;
	}

	if (val) {
		*val = (s >= 0) ? v : (-v);
	}

	return 0;
}

static int pnm_getc(jas_stream_t *in)
{
	int c;
	for (;;) {
		if ((c = jas_stream_getc(in)) == EOF) {
			return -1;
		}
		if (c != '#') {
			return c;
		}
		do {
			if ((c = jas_stream_getc(in)) == EOF) {
				return -1;
			}
		} while (c != '\n' && c != '\r');
	}
}

static int pnm_getint16(jas_stream_t *in, int *val)
{
	int v;
	int c;

	if ((c = jas_stream_getc(in)) == EOF) {
		return -1;
	}
	v = c & 0xff;
	if ((c = jas_stream_getc(in)) == EOF) {
		return -1;
	}
	v = (v << 8) | (c & 0xff);
	*val = v;

	return 0;
}

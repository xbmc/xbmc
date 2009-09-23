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
#include "jasper/jas_tvp.h"
#include "jasper/jas_image.h"
#include "jasper/jas_stream.h"
#include "jasper/jas_debug.h"

#include "pnm_cod.h"
#include "pnm_enc.h"

/******************************************************************************\
* Local types.
\******************************************************************************/

typedef struct {
	bool bin;
} pnm_encopts_t;

typedef enum {
	OPT_TEXT
} pnm_optid_t;

jas_taginfo_t pnm_opttab[] = {
	{OPT_TEXT, "text"},
	{-1, 0}
};

/******************************************************************************\
* Local function prototypes.
\******************************************************************************/

static int pnm_parseencopts(char *optstr, pnm_encopts_t *encopts);
static int pnm_puthdr(jas_stream_t *out, pnm_hdr_t *hdr);
static int pnm_putdata(jas_stream_t *out, pnm_hdr_t *hdr, jas_image_t *image, int numcmpts, int *cmpts);

static int pnm_putsint(jas_stream_t *out, int wordsize, int_fast32_t *val);
static int pnm_putuint(jas_stream_t *out, int wordsize, uint_fast32_t *val);
static int pnm_putuint16(jas_stream_t *out, uint_fast16_t val);

/******************************************************************************\
* Save function.
\******************************************************************************/

int pnm_encode(jas_image_t *image, jas_stream_t *out, char *optstr)
{
	int width;
	int height;
	int cmptno;
	pnm_hdr_t hdr;
	pnm_encopts_t encopts;
	int prec;
	int sgnd;
	pnm_enc_t encbuf;
	pnm_enc_t *enc = &encbuf;

	/* Parse the encoder option string. */
	if (pnm_parseencopts(optstr, &encopts)) {
		jas_eprintf("invalid PNM encoder options specified\n");
		return -1;
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
	prec = jas_image_cmptprec(image, enc->cmpts[0]);
	sgnd = jas_image_cmptsgnd(image, enc->cmpts[0]);

	/* The PNM format is quite limited in the set of image geometries
	  that it can handle.  Here, we check to ensure that the image to
	  be saved can actually be represented reasonably accurately using the
	  PNM format. */
	/* All of the components must have the same width and height. */
	/* All of the components must have unsigned samples with the same
	  precision.*/
	/* All of the components must have their top-left corner located at
	  the origin. */
	for (cmptno = 0; cmptno < enc->numcmpts; ++cmptno) {
		if (jas_image_cmptwidth(image, enc->cmpts[cmptno]) != width ||
		  jas_image_cmptheight(image, enc->cmpts[cmptno]) != height ||
		  jas_image_cmptprec(image, enc->cmpts[cmptno]) != prec ||
		  jas_image_cmptsgnd(image, enc->cmpts[cmptno]) != sgnd ||
		  jas_image_cmpthstep(image, enc->cmpts[cmptno]) != jas_image_cmpthstep(image, 0) ||
		  jas_image_cmptvstep(image, enc->cmpts[cmptno]) != jas_image_cmptvstep(image, 0) ||
		  jas_image_cmpttlx(image, enc->cmpts[cmptno]) != jas_image_cmpttlx(image, 0) ||
		  jas_image_cmpttly(image, enc->cmpts[cmptno]) != jas_image_cmpttly(image, 0)) {
			jas_eprintf("The PNM format cannot be used to represent an image with this geometry.\n");
			return -1;
		}
	}

	if (sgnd) {
		jas_eprintf("warning: support for signed sample data requires use of nonstandard extension to PNM format\n");
		jas_eprintf("You may not be able to read or correctly display the resulting PNM data with other software.\n");
	}

	/* Initialize the header. */
	if (enc->numcmpts == 1) {
		hdr.magic = encopts.bin ? PNM_MAGIC_BINPGM : PNM_MAGIC_TXTPGM;
	} else if (enc->numcmpts == 3) {
		hdr.magic = encopts.bin ? PNM_MAGIC_BINPPM : PNM_MAGIC_TXTPPM;
	} else {
		return -1;
	}
	hdr.width = width;
	hdr.height = height;
	hdr.maxval = (1 << prec) - 1;
	hdr.sgnd = sgnd;

	/* Write the header. */
	if (pnm_puthdr(out, &hdr)) {
		return -1;
	}

	/* Write the image data. */
	if (pnm_putdata(out, &hdr, image, enc->numcmpts, enc->cmpts)) {
		return -1;
	}

	/* Flush the output stream. */
	if (jas_stream_flush(out)) {
		return -1;
	}

	return 0;
}

/******************************************************************************\
* Code for parsing options.
\******************************************************************************/

/* Parse the encoder options string. */
static int pnm_parseencopts(char *optstr, pnm_encopts_t *encopts)
{
	jas_tvparser_t *tvp;
	int ret;

	tvp = 0;

	/* Initialize default values for encoder options. */
	encopts->bin = true;

	/* Create the tag-value parser. */
	if (!(tvp = jas_tvparser_create(optstr ? optstr : ""))) {
		goto error;
	}

	/* Get tag-value pairs, and process as necessary. */
	while (!(ret = jas_tvparser_next(tvp))) {
		switch (jas_taginfo_nonull(jas_taginfos_lookup(pnm_opttab,
		  jas_tvparser_gettag(tvp)))->id) {
		case OPT_TEXT:
			encopts->bin = false;
			break;
		default:
			jas_eprintf("warning: ignoring invalid option %s\n",
			  jas_tvparser_gettag(tvp));
			break;
		}	
	}
	if (ret < 0) {
		goto error;
	}

	/* Destroy the tag-value parser. */
	jas_tvparser_destroy(tvp);

	return 0;

error:
	if (tvp) {
		jas_tvparser_destroy(tvp);
	}
	return -1;
}

/******************************************************************************\
* Function for writing header.
\******************************************************************************/

/* Write the header. */
static int pnm_puthdr(jas_stream_t *out, pnm_hdr_t *hdr)
{
	int_fast32_t maxval;

	if (pnm_putuint16(out, hdr->magic)) {
		return -1;
	}
	if (hdr->sgnd) {
		maxval = -hdr->maxval;
	} else {
		maxval = hdr->maxval;
	}
	jas_stream_printf(out, "\n%lu %lu\n%ld\n", (unsigned long) hdr->width,
	  (unsigned long) hdr->height, (long) maxval);
	if (jas_stream_error(out)) {
		return -1;
	}
	return 0;
}

/******************************************************************************\
* Functions for processing the sample data.
\******************************************************************************/

/* Write the image sample data. */
static int pnm_putdata(jas_stream_t *out, pnm_hdr_t *hdr, jas_image_t *image, int numcmpts, int *cmpts)
{
	int ret;
	int cmptno;
	int x;
	int y;
	jas_matrix_t *data[3];
	int fmt;
	jas_seqent_t *d[3];
	jas_seqent_t v;
	int minval;
	int linelen;
	int n;
	char buf[256];
	int depth;

	ret = -1;
	fmt = pnm_fmt(hdr->magic);
	minval = -((int) hdr->maxval + 1);
	depth = pnm_maxvaltodepth(hdr->maxval);

	data[0] = 0;
	data[1] = 0;
	data[2] = 0;
	for (cmptno = 0; cmptno < numcmpts; ++cmptno) {
		if (!(data[cmptno] = jas_matrix_create(1, hdr->width))) {
			goto done;
		}
	}

	for (y = 0; y < hdr->height; ++y) {
		for (cmptno = 0; cmptno < numcmpts; ++cmptno) {
			if (jas_image_readcmpt(image, cmpts[cmptno], 0, y, hdr->width, 1,
			  data[cmptno])) {
				goto done;
			}
			d[cmptno] = jas_matrix_getref(data[cmptno], 0, 0);
		}
		linelen = 0;
		for (x = 0; x < hdr->width; ++x) {
			for (cmptno = 0; cmptno < numcmpts; ++cmptno) {
				v = *d[cmptno];
				if (v < minval) {
					v = minval;
				}
				if (v > ((int) hdr->maxval)) {
					v = hdr->maxval;
				}
				if (fmt == PNM_FMT_BIN) {
					if (hdr->sgnd) {
						int_fast32_t sv;
						sv = v;
						if (pnm_putsint(out, depth, &sv)) {
							goto done;
						}
					} else {
						uint_fast32_t uv;
						uv = v;
						if (pnm_putuint(out, depth, &uv)) {
							goto done;
						}
					}
				} else {
					n = sprintf(buf, "%s%ld", ((!(!x && !cmptno)) ? " " : ""),
					  (long) v);
					if (linelen > 0 && linelen + n > PNM_MAXLINELEN) {
						jas_stream_printf(out, "\n");
						linelen = 0;
					}
					jas_stream_printf(out, "%s", buf);
					linelen += n;
				}
				++d[cmptno];
			}
		}
		if (fmt != PNM_FMT_BIN) {
			jas_stream_printf(out, "\n");
			linelen = 0;
		}
		if (jas_stream_error(out)) {
			goto done;
		}
	}

	ret = 0;

done:

	for (cmptno = 0; cmptno < numcmpts; ++cmptno) {
		if (data[cmptno]) {
			jas_matrix_destroy(data[cmptno]);
		}
	}

	return ret;
}

/******************************************************************************\
* Miscellaneous functions.
\******************************************************************************/

static int pnm_putsint(jas_stream_t *out, int wordsize, int_fast32_t *val)
{
	uint_fast32_t tmpval;
	tmpval = (*val < 0) ?
	  ((~(JAS_CAST(uint_fast32_t, -(*val)) + 1)) & PNM_ONES(wordsize)) :
	  JAS_CAST(uint_fast32_t, (*val));
	return pnm_putuint(out, wordsize, &tmpval);
}

static int pnm_putuint(jas_stream_t *out, int wordsize, uint_fast32_t *val)
{
	int n;
	uint_fast32_t tmpval;
	int c;

	n = (wordsize + 7) / 8;
	tmpval = (*val);
	tmpval &= PNM_ONES(8 * n);
	tmpval = tmpval << (8 * (4 - n));
	while (--n >= 0) {
		c = (tmpval >> 24) & 0xff;
		if (jas_stream_putc(out, c) == EOF) {
			return -1;
		}
		tmpval = (tmpval << 8) & 0xffffffff;
	}
	return 0;
}

/* Write a 16-bit unsigned integer to a stream. */
static int pnm_putuint16(jas_stream_t *out, uint_fast16_t val)
{
	if (jas_stream_putc(out, (unsigned char)(val >> 8)) == EOF ||
	  jas_stream_putc(out, (unsigned char)(val & 0xff)) == EOF) {
		return -1;
	}
	return 0;
}

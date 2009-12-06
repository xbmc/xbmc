/*
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

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <assert.h>

#include "jasper/jas_tvp.h"
#include "jasper/jas_stream.h"
#include "jasper/jas_image.h"
#include "jasper/jas_string.h"
#include "jasper/jas_debug.h"

#include "pgx_cod.h"
#include "pgx_enc.h"

/******************************************************************************\
* Local functions.
\******************************************************************************/

static int pgx_puthdr(jas_stream_t *out, pgx_hdr_t *hdr);
static int pgx_putdata(jas_stream_t *out, pgx_hdr_t *hdr, jas_image_t *image, int cmpt);
static int pgx_putword(jas_stream_t *out, bool bigendian, int prec,
  uint_fast32_t val);
static uint_fast32_t pgx_inttoword(int_fast32_t val, int prec, bool sgnd);

/******************************************************************************\
* Code for save operation.
\******************************************************************************/

/* Save an image to a stream in the the PGX format. */

int pgx_encode(jas_image_t *image, jas_stream_t *out, char *optstr)
{
	pgx_hdr_t hdr;
	uint_fast32_t width;
	uint_fast32_t height;
	bool sgnd;
	int prec;
	pgx_enc_t encbuf;
	pgx_enc_t *enc = &encbuf;

	/* Avoid compiler warnings about unused parameters. */
	optstr = 0;

	switch (jas_clrspc_fam(jas_image_clrspc(image))) {
	case JAS_CLRSPC_FAM_GRAY:
		if ((enc->cmpt = jas_image_getcmptbytype(image,
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_GRAY_Y))) < 0) {
			jas_eprintf("error: missing color component\n");
			return -1;
		}
		break;
	default:
		jas_eprintf("error: BMP format does not support color space\n");
		return -1;
		break;
	}

	width = jas_image_cmptwidth(image, enc->cmpt);
	height = jas_image_cmptheight(image, enc->cmpt);
	prec = jas_image_cmptprec(image, enc->cmpt);
	sgnd = jas_image_cmptsgnd(image, enc->cmpt);

	/* The PGX format is quite limited in the set of image geometries
	  that it can handle.  Here, we check to ensure that the image to
	  be saved can actually be represented reasonably accurately using the
	  PGX format. */
	/* There must be exactly one component. */
	if (jas_image_numcmpts(image) > 1 || prec > 16) {
		jas_eprintf("The PNM format cannot be used to represent an image with this geometry.\n");
		return -1;
	}

	hdr.magic = PGX_MAGIC;
	hdr.bigendian = true;
	hdr.sgnd = sgnd;
	hdr.prec = prec;
	hdr.width = width;
	hdr.height = height;

#ifdef PGX_DEBUG
	pgx_dumphdr(stderr, &hdr);
#endif

	if (pgx_puthdr(out, &hdr)) {
		return -1;
	}

	if (pgx_putdata(out, &hdr, image, enc->cmpt)) {
		return -1;
	}

	return 0;
}

/******************************************************************************\
\******************************************************************************/

static int pgx_puthdr(jas_stream_t *out, pgx_hdr_t *hdr)
{
	jas_stream_printf(out, "%c%c", hdr->magic >> 8, hdr->magic & 0xff);
	jas_stream_printf(out, " %s %s %d %ld %ld\n", hdr->bigendian ? "ML" : "LM",
	  hdr->sgnd ? "-" : "+", hdr->prec, (long) hdr->width, (long) hdr->height);
	if (jas_stream_error(out)) {
		return -1;
	}
	return 0;
}

static int pgx_putdata(jas_stream_t *out, pgx_hdr_t *hdr, jas_image_t *image, int cmpt)
{
	jas_matrix_t *data;
	uint_fast32_t x;
	uint_fast32_t y;
	int_fast32_t v;
	uint_fast32_t word;

	data = 0;

	if (!(data = jas_matrix_create(1, hdr->width))) {
		goto error;
	}
	for (y = 0; y < hdr->height; ++y) {
		if (jas_image_readcmpt(image, cmpt, 0, y, hdr->width, 1, data)) {
			goto error;
		}
		for (x = 0; x < hdr->width; ++x) {
			v = jas_matrix_get(data, 0, x);
			word = pgx_inttoword(v, hdr->prec, hdr->sgnd);
			if (pgx_putword(out, hdr->bigendian, hdr->prec, word)) {
				goto error;
			}
		}
	}
	jas_matrix_destroy(data);
	data = 0;
	return 0;

error:
	if (data) {
		jas_matrix_destroy(data);
	}
	return -1;
}

static int pgx_putword(jas_stream_t *out, bool bigendian, int prec,
  uint_fast32_t val)
{
	int i;
	int j;
	int wordsize;

	val &= (1 << prec) - 1;
	wordsize = (prec + 7) /8;
	for (i = 0; i < wordsize; ++i) {
		j = bigendian ? (wordsize - 1 - i) : i;
		if (jas_stream_putc(out, (val >> (8 * j)) & 0xff) == EOF) {
			return -1;
		}
	}
	return 0;
}

static uint_fast32_t pgx_inttoword(jas_seqent_t v, int prec, bool sgnd)
{
	uint_fast32_t ret;
	ret = ((sgnd && v < 0) ? ((1 << prec) + v) : v) & ((1 << prec) - 1);
	return ret;
}

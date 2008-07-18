/*
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

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <assert.h>

#include "jasper/jas_tvp.h"
#include "jasper/jas_stream.h"
#include "jasper/jas_image.h"
#include "jasper/jas_string.h"
#include "jasper/jas_malloc.h"

#include "mif_cod.h"

/******************************************************************************\
* Local types.
\******************************************************************************/

typedef enum {
	MIF_END = 0,
	MIF_CMPT
} mif_tagid2_t;

typedef enum {
	MIF_TLX = 0,
	MIF_TLY,
	MIF_WIDTH,
	MIF_HEIGHT,
	MIF_HSAMP,
	MIF_VSAMP,
	MIF_PREC,
	MIF_SGND,
	MIF_DATA
} mif_tagid_t;

/******************************************************************************\
* Local functions.
\******************************************************************************/

static mif_hdr_t *mif_hdr_create(int maxcmpts);
static void mif_hdr_destroy(mif_hdr_t *hdr);
static int mif_hdr_growcmpts(mif_hdr_t *hdr, int maxcmpts);
static mif_hdr_t *mif_hdr_get(jas_stream_t *in);
static int mif_process_cmpt(mif_hdr_t *hdr, char *buf);
static int mif_hdr_put(mif_hdr_t *hdr, jas_stream_t *out);
static int mif_hdr_addcmpt(mif_hdr_t *hdr, int cmptno, mif_cmpt_t *cmpt);
static mif_cmpt_t *mif_cmpt_create(void);
static void mif_cmpt_destroy(mif_cmpt_t *cmpt);
static char *mif_getline(jas_stream_t *jas_stream, char *buf, int bufsize);
static int mif_getc(jas_stream_t *in);
static mif_hdr_t *mif_makehdrfromimage(jas_image_t *image);

/******************************************************************************\
* Local data.
\******************************************************************************/

jas_taginfo_t mif_tags2[] = {
	{MIF_CMPT, "component"},
	{MIF_END, "end"},
	{-1, 0}
};

jas_taginfo_t mif_tags[] = {
	{MIF_TLX, "tlx"},
	{MIF_TLY, "tly"},
	{MIF_WIDTH, "width"},
	{MIF_HEIGHT, "height"},
	{MIF_HSAMP, "sampperx"},
	{MIF_VSAMP, "samppery"},
	{MIF_PREC, "prec"},
	{MIF_SGND, "sgnd"},
	{MIF_DATA, "data"},
	{-1, 0}
};

/******************************************************************************\
* Code for load operation.
\******************************************************************************/

/* Load an image from a stream in the MIF format. */

jas_image_t *mif_decode(jas_stream_t *in, char *optstr)
{
	mif_hdr_t *hdr;
	jas_image_t *image;
	jas_image_t *tmpimage;
	jas_stream_t *tmpstream;
	int cmptno;
	mif_cmpt_t *cmpt;
	jas_image_cmptparm_t cmptparm;
	jas_seq2d_t *data;
	int_fast32_t x;
	int_fast32_t y;
	int bias;

	/* Avoid warnings about unused parameters. */
	optstr = 0;

	hdr = 0;
	image = 0;
	tmpimage = 0;
	tmpstream = 0;
	data = 0;

	if (!(hdr = mif_hdr_get(in))) {
		goto error;
	}

	if (!(image = jas_image_create0())) {
		goto error;
	}

	for (cmptno = 0; cmptno < hdr->numcmpts; ++cmptno) {
		cmpt = hdr->cmpts[cmptno];
		tmpstream = cmpt->data ? jas_stream_fopen(cmpt->data, "rb") : in;
		if (!tmpstream) {
			goto error;
		}
		if (!(tmpimage = jas_image_decode(tmpstream, -1, 0))) {
			goto error;
		}
		if (tmpstream != in) {
			jas_stream_close(tmpstream);
			tmpstream = 0;
		}
		if (!cmpt->width) {
			cmpt->width = jas_image_cmptwidth(tmpimage, 0);
		}
		if (!cmpt->height) {
			cmpt->height = jas_image_cmptwidth(tmpimage, 0);
		}
		if (!cmpt->prec) {
			cmpt->prec = jas_image_cmptprec(tmpimage, 0);
		}
		if (cmpt->sgnd < 0) {
			cmpt->sgnd = jas_image_cmptsgnd(tmpimage, 0);
		}
		cmptparm.tlx = cmpt->tlx;
		cmptparm.tly = cmpt->tly;
		cmptparm.hstep = cmpt->sampperx;
		cmptparm.vstep = cmpt->samppery;
		cmptparm.width = cmpt->width;
		cmptparm.height = cmpt->height;
		cmptparm.prec = cmpt->prec;
		cmptparm.sgnd = cmpt->sgnd;
		if (jas_image_addcmpt(image, jas_image_numcmpts(image), &cmptparm)) {
			goto error;
		}
		if (!(data = jas_seq2d_create(0, 0, cmpt->width, cmpt->height))) {
			goto error;
		}
		if (jas_image_readcmpt(tmpimage, 0, 0, 0, cmpt->width, cmpt->height,
		  data)) {
			goto error;
		}
		if (cmpt->sgnd) {
			bias = 1 << (cmpt->prec - 1);
			for (y = 0; y < cmpt->height; ++y) {
				for (x = 0; x < cmpt->width; ++x) {
					*jas_seq2d_getref(data, x, y) -= bias;
				}
			}
		}
		if (jas_image_writecmpt(image, jas_image_numcmpts(image) - 1, 0, 0,
		  cmpt->width, cmpt->height, data)) {
			goto error;
		}
		jas_seq2d_destroy(data);
		data = 0;
		jas_image_destroy(tmpimage);
		tmpimage = 0;
	}

	mif_hdr_destroy(hdr);
	hdr = 0;
	return image;

error:
	if (image) {
		jas_image_destroy(image);
	}
	if (hdr) {
		mif_hdr_destroy(hdr);
	}
	if (tmpstream && tmpstream != in) {
		jas_stream_close(tmpstream);
	}
	if (tmpimage) {
		jas_image_destroy(tmpimage);
	}
	if (data) {
		jas_seq2d_destroy(data);
	}
	return 0;
}

/******************************************************************************\
* Code for save operation.
\******************************************************************************/

/* Save an image to a stream in the the MIF format. */

int mif_encode(jas_image_t *image, jas_stream_t *out, char *optstr)
{
	mif_hdr_t *hdr;
	jas_image_t *tmpimage;
	int fmt;
	int cmptno;
	mif_cmpt_t *cmpt;
	jas_image_cmptparm_t cmptparm;
	jas_seq2d_t *data;
	int_fast32_t x;
	int_fast32_t y;
	int bias;

	hdr = 0;
	tmpimage = 0;
	data = 0;

	if (optstr && *optstr != '\0') {
		jas_eprintf("warning: ignoring unsupported options\n");
	}

	if ((fmt = jas_image_strtofmt("pnm")) < 0) {
		jas_eprintf("error: PNM support required\n");
		goto error;
	}

	if (!(hdr = mif_makehdrfromimage(image))) {
		goto error;
	}
	if (mif_hdr_put(hdr, out)) {
		goto error;
	}

	/* Output component data. */
	for (cmptno = 0; cmptno < hdr->numcmpts; ++cmptno) {
		cmpt = hdr->cmpts[cmptno];
		if (!cmpt->data) {
			if (!(tmpimage = jas_image_create0())) {
				goto error;
			}	
			cmptparm.tlx = 0;
			cmptparm.tly = 0;
			cmptparm.hstep = cmpt->sampperx;
			cmptparm.vstep = cmpt->samppery;
			cmptparm.width = cmpt->width;
			cmptparm.height = cmpt->height;
			cmptparm.prec = cmpt->prec;
			cmptparm.sgnd = false;
			if (jas_image_addcmpt(tmpimage, jas_image_numcmpts(tmpimage), &cmptparm)) {
				goto error;
			}
			if (!(data = jas_seq2d_create(0, 0, cmpt->width, cmpt->height))) {
				goto error;
			}
			if (jas_image_readcmpt(image, cmptno, 0, 0, cmpt->width, cmpt->height,
			  data)) {
				goto error;
			}
			if (cmpt->sgnd) {
				bias = 1 << (cmpt->prec - 1);
				for (y = 0; y < cmpt->height; ++y) {
					for (x = 0; x < cmpt->width; ++x) {
						*jas_seq2d_getref(data, x, y) += bias;
					}
				}
			}
			if (jas_image_writecmpt(tmpimage, 0, 0, 0, cmpt->width, cmpt->height,
			  data)) {
				goto error;
			}
			jas_seq2d_destroy(data);
			data = 0;
			if (jas_image_encode(tmpimage, out, fmt, 0)) {
				goto error;
			}
			jas_image_destroy(tmpimage);
			tmpimage = 0;
		}
	}

	mif_hdr_destroy(hdr);

	return 0;

error:
	if (hdr) {
		mif_hdr_destroy(hdr);
	}
	if (tmpimage) {
		jas_image_destroy(tmpimage);
	}
	if (data) {
		jas_seq2d_destroy(data);
	}
	return -1;
}

/******************************************************************************\
* Code for validate operation.
\******************************************************************************/

int mif_validate(jas_stream_t *in)
{
	uchar buf[MIF_MAGICLEN];
	uint_fast32_t magic;
	int i;
	int n;

	assert(JAS_STREAM_MAXPUTBACK >= MIF_MAGICLEN);

	/* Read the validation data (i.e., the data used for detecting
	  the format). */
	if ((n = jas_stream_read(in, buf, MIF_MAGICLEN)) < 0) {
		return -1;
	}

	/* Put the validation data back onto the stream, so that the
	  stream position will not be changed. */
	for (i = n - 1; i >= 0; --i) {
		if (jas_stream_ungetc(in, buf[i]) == EOF) {
			return -1;
		}
	}

	/* Was enough data read? */
	if (n < MIF_MAGICLEN) {
		return -1;
	}

	/* Compute the signature value. */
	magic = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

	/* Ensure that the signature is correct for this format. */
	if (magic != MIF_MAGIC) {
		return -1;
	}

	return 0;
}

/******************************************************************************\
* Code for MIF header class.
\******************************************************************************/

static mif_hdr_t *mif_hdr_create(int maxcmpts)
{
	mif_hdr_t *hdr;
	if (!(hdr = jas_malloc(sizeof(mif_hdr_t)))) {
		return 0;
	}
	hdr->numcmpts = 0;
	hdr->maxcmpts = 0;
	hdr->cmpts = 0;
	if (mif_hdr_growcmpts(hdr, maxcmpts)) {
		mif_hdr_destroy(hdr);
		return 0;
	}
	return hdr;
}

static void mif_hdr_destroy(mif_hdr_t *hdr)
{
	int cmptno;
	if (hdr->cmpts) {
		for (cmptno = 0; cmptno < hdr->numcmpts; ++cmptno) {
			mif_cmpt_destroy(hdr->cmpts[cmptno]);
		}
		jas_free(hdr->cmpts);
	}
	jas_free(hdr);
}

static int mif_hdr_growcmpts(mif_hdr_t *hdr, int maxcmpts)
{
	int cmptno;
	mif_cmpt_t **newcmpts;
	assert(maxcmpts >= hdr->numcmpts);
	newcmpts = (!hdr->cmpts) ? jas_malloc(maxcmpts * sizeof(mif_cmpt_t *)) :
	  jas_realloc(hdr->cmpts, maxcmpts * sizeof(mif_cmpt_t *));
	if (!newcmpts) {
		return -1;
	}
	hdr->maxcmpts = maxcmpts;
	hdr->cmpts = newcmpts;
	for (cmptno = hdr->numcmpts; cmptno < hdr->maxcmpts; ++cmptno) {
		hdr->cmpts[cmptno] = 0;
	}
	return 0;
}

static mif_hdr_t *mif_hdr_get(jas_stream_t *in)
{
	uchar magicbuf[MIF_MAGICLEN];
	char buf[4096];
	mif_hdr_t *hdr;
	bool done;
	jas_tvparser_t *tvp;
	int id;

	hdr = 0;

	if (jas_stream_read(in, magicbuf, MIF_MAGICLEN) != MIF_MAGICLEN) {
		goto error;
	}
	if (magicbuf[0] != (MIF_MAGIC >> 24) || magicbuf[1] != ((MIF_MAGIC >> 16) &
	  0xff) || magicbuf[2] != ((MIF_MAGIC >> 8) & 0xff) || magicbuf[3] !=
	  (MIF_MAGIC & 0xff)) {
		jas_eprintf("error: bad signature\n");
		goto error;
	}

	if (!(hdr = mif_hdr_create(0))) {
		goto error;
	}

	done = false;
	do {
		if (!mif_getline(in, buf, sizeof(buf))) {
			goto error;
		}
		if (buf[0] == '\0') {
			continue;
		}
		if (!(tvp = jas_tvparser_create(buf))) {
			goto error;
		}
		if (jas_tvparser_next(tvp)) {
			abort();
		}
		id = jas_taginfo_nonull(jas_taginfos_lookup(mif_tags2, jas_tvparser_gettag(tvp)))->id;
		jas_tvparser_destroy(tvp);
		switch (id) {
		case MIF_CMPT:
			mif_process_cmpt(hdr, buf);
			break;
		case MIF_END:
			done = 1;
			break;
		}
	} while (!done);

	return hdr;

error:
	if (hdr) {
		mif_hdr_destroy(hdr);
	}
	return 0;
}

static int mif_process_cmpt(mif_hdr_t *hdr, char *buf)
{
	jas_tvparser_t *tvp;
	mif_cmpt_t *cmpt;
	int id;

	cmpt = 0;
	tvp = 0;

	if (!(cmpt = mif_cmpt_create())) {
		goto error;
	}
	cmpt->tlx = 0;
	cmpt->tly = 0;
	cmpt->sampperx = 0;
	cmpt->samppery = 0;
	cmpt->width = 0;
	cmpt->height = 0;
	cmpt->prec = 0;
	cmpt->sgnd = -1;
	cmpt->data = 0;

	if (!(tvp = jas_tvparser_create(buf))) {
		goto error;
	}
	while (!(id = jas_tvparser_next(tvp))) {
		switch (jas_taginfo_nonull(jas_taginfos_lookup(mif_tags,
		  jas_tvparser_gettag(tvp)))->id) {
		case MIF_TLX:
			cmpt->tlx = atoi(jas_tvparser_getval(tvp));
			break;
		case MIF_TLY:
			cmpt->tly = atoi(jas_tvparser_getval(tvp));
			break;
		case MIF_WIDTH:
			cmpt->width = atoi(jas_tvparser_getval(tvp));
			break;
		case MIF_HEIGHT:
			cmpt->height = atoi(jas_tvparser_getval(tvp));
			break;
		case MIF_HSAMP:
			cmpt->sampperx = atoi(jas_tvparser_getval(tvp));
			break;
		case MIF_VSAMP:
			cmpt->samppery = atoi(jas_tvparser_getval(tvp));
			break;
		case MIF_PREC:
			cmpt->prec = atoi(jas_tvparser_getval(tvp));
			break;
		case MIF_SGND:
			cmpt->sgnd = atoi(jas_tvparser_getval(tvp));
			break;
		case MIF_DATA:
			if (!(cmpt->data = jas_strdup(jas_tvparser_getval(tvp)))) {
				return -1;
			}
			break;
		}
	}
	jas_tvparser_destroy(tvp);
	if (!cmpt->sampperx || !cmpt->samppery) {
		goto error;
	}
	if (mif_hdr_addcmpt(hdr, hdr->numcmpts, cmpt)) {
		goto error;
	}
	return 0;

error:
	if (cmpt) {
		mif_cmpt_destroy(cmpt);
	}
	if (tvp) {
		jas_tvparser_destroy(tvp);
	}
	return -1;
}

static int mif_hdr_put(mif_hdr_t *hdr, jas_stream_t *out)
{
	int cmptno;
	mif_cmpt_t *cmpt;

	/* Output signature. */
	jas_stream_putc(out, (MIF_MAGIC >> 24) & 0xff);
	jas_stream_putc(out, (MIF_MAGIC >> 16) & 0xff);
	jas_stream_putc(out, (MIF_MAGIC >> 8) & 0xff);
	jas_stream_putc(out, MIF_MAGIC & 0xff);

	/* Output component information. */
	for (cmptno = 0; cmptno < hdr->numcmpts; ++cmptno) {
		cmpt = hdr->cmpts[cmptno];
		jas_stream_printf(out, "component tlx=%ld tly=%ld "
		  "sampperx=%ld samppery=%ld width=%ld height=%ld prec=%d sgnd=%d",
		  cmpt->tlx, cmpt->tly, cmpt->sampperx, cmpt->samppery, cmpt->width,
		  cmpt->height, cmpt->prec, cmpt->sgnd);
		if (cmpt->data) {
			jas_stream_printf(out, " data=%s", cmpt->data);
		}
		jas_stream_printf(out, "\n");
	}

	/* Output end of header indicator. */
	jas_stream_printf(out, "end\n");

	return 0;
}

static int mif_hdr_addcmpt(mif_hdr_t *hdr, int cmptno, mif_cmpt_t *cmpt)
{
	assert(cmptno >= hdr->numcmpts);
	if (hdr->numcmpts >= hdr->maxcmpts) {
		if (mif_hdr_growcmpts(hdr, hdr->numcmpts + 128)) {
			return -1;
		}
	}
	hdr->cmpts[hdr->numcmpts] = cmpt;
	++hdr->numcmpts;
	return 0;
}

/******************************************************************************\
* Code for MIF component class.
\******************************************************************************/

static mif_cmpt_t *mif_cmpt_create()
{
	mif_cmpt_t *cmpt;
	if (!(cmpt = jas_malloc(sizeof(mif_cmpt_t)))) {
		return 0;
	}
	memset(cmpt, 0, sizeof(mif_cmpt_t));
	return cmpt;
}

static void mif_cmpt_destroy(mif_cmpt_t *cmpt)
{
	if (cmpt->data) {
		jas_free(cmpt->data);
	}
	jas_free(cmpt);
}

/******************************************************************************\
* MIF parsing code.
\******************************************************************************/

static char *mif_getline(jas_stream_t *stream, char *buf, int bufsize)
{
	int c;
	char *bufptr;
	assert(bufsize > 0);

	bufptr = buf;
	while (bufsize > 1) {
		if ((c = mif_getc(stream)) == EOF) {
			break;
		}
		*bufptr++ = c;
		--bufsize;
		if (c == '\n') {
			break;
		}
	}
	*bufptr = '\0';
	if (!(bufptr = strchr(buf, '\n'))) {
		return 0;
	}
	*bufptr = '\0';
	return buf;
}

static int mif_getc(jas_stream_t *in)
{
	int c;
	bool done;

	done = false;
	do {
		switch (c = jas_stream_getc(in)) {
		case EOF:
			done = 1;
			break;
		case '#':
			for (;;) {
				if ((c = jas_stream_getc(in)) == EOF) {
					done = 1;
					break;
				}	
				if (c == '\n') {
					break;
				}
			}
			break;
		case '\\':
			if (jas_stream_peekc(in) == '\n') {
				jas_stream_getc(in);
			}
			break;
		default:
			done = 1;
			break;
		}
	} while (!done);

	return c;
}

/******************************************************************************\
* Miscellaneous functions.
\******************************************************************************/

static mif_hdr_t *mif_makehdrfromimage(jas_image_t *image)
{
	mif_hdr_t *hdr;
	int cmptno;
	mif_cmpt_t *cmpt;

	if (!(hdr = mif_hdr_create(jas_image_numcmpts(image)))) {
		return 0;
	}
	hdr->magic = MIF_MAGIC;
	hdr->numcmpts = jas_image_numcmpts(image);
	for (cmptno = 0; cmptno < hdr->numcmpts; ++cmptno) {
		hdr->cmpts[cmptno] = jas_malloc(sizeof(mif_cmpt_t));
		cmpt = hdr->cmpts[cmptno];
		cmpt->tlx = jas_image_cmpttlx(image, cmptno);
		cmpt->tly = jas_image_cmpttly(image, cmptno);
		cmpt->width = jas_image_cmptwidth(image, cmptno);
		cmpt->height = jas_image_cmptheight(image, cmptno);
		cmpt->sampperx = jas_image_cmpthstep(image, cmptno);
		cmpt->samppery = jas_image_cmptvstep(image, cmptno);
		cmpt->prec = jas_image_cmptprec(image, cmptno);
		cmpt->sgnd = jas_image_cmptsgnd(image, cmptno);
		cmpt->data = 0;
	}
	return hdr;
}

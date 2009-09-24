/*
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

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <assert.h>
#include <ctype.h>

#include "jasper/jas_tvp.h"
#include "jasper/jas_stream.h"
#include "jasper/jas_image.h"
#include "jasper/jas_string.h"

#include "pgx_cod.h"

/******************************************************************************\
* Local prototypes.
\******************************************************************************/

static int pgx_gethdr(jas_stream_t *in, pgx_hdr_t *hdr);
static int pgx_getdata(jas_stream_t *in, pgx_hdr_t *hdr, jas_image_t *image);
static int_fast32_t pgx_getword(jas_stream_t *in, bool bigendian, int prec);
static int pgx_getsgnd(jas_stream_t *in, bool *sgnd);
static int pgx_getbyteorder(jas_stream_t *in, bool *bigendian);
static int pgx_getc(jas_stream_t *in);
static int pgx_getuint32(jas_stream_t *in, uint_fast32_t *val);
static jas_seqent_t pgx_wordtoint(uint_fast32_t word, int prec, bool sgnd);

/******************************************************************************\
* Code for load operation.
\******************************************************************************/

/* Load an image from a stream in the PGX format. */

jas_image_t *pgx_decode(jas_stream_t *in, char *optstr)
{
	jas_image_t *image;
	pgx_hdr_t hdr;
	jas_image_cmptparm_t cmptparm;

	/* Avoid compiler warnings about unused parameters. */
	optstr = 0;

	image = 0;

	if (pgx_gethdr(in, &hdr)) {
		goto error;
	}

#ifdef PGX_DEBUG
	pgx_dumphdr(stderr, &hdr);
#endif

	if (!(image = jas_image_create0())) {
		goto error;
	}
	cmptparm.tlx = 0;
	cmptparm.tly = 0;
	cmptparm.hstep = 1;
	cmptparm.vstep = 1;
	cmptparm.width = hdr.width;
	cmptparm.height = hdr.height;
	cmptparm.prec = hdr.prec;
	cmptparm.sgnd = hdr.sgnd;
	if (jas_image_addcmpt(image, 0, &cmptparm)) {
		goto error;
	}
	if (pgx_getdata(in, &hdr, image)) {
		goto error;
	}

	jas_image_setclrspc(image, JAS_CLRSPC_SGRAY);
	jas_image_setcmpttype(image, 0,
	  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_GRAY_Y));

	return image;

error:
	if (image) {
		jas_image_destroy(image);
	}
	return 0;
}

/******************************************************************************\
* Code for validate operation.
\******************************************************************************/

int pgx_validate(jas_stream_t *in)
{
	uchar buf[PGX_MAGICLEN];
	uint_fast32_t magic;
	int i;
	int n;

	assert(JAS_STREAM_MAXPUTBACK >= PGX_MAGICLEN);

	/* Read the validation data (i.e., the data used for detecting
	  the format). */
	if ((n = jas_stream_read(in, buf, PGX_MAGICLEN)) < 0) {
		return -1;
	}

	/* Put the validation data back onto the stream, so that the
	  stream position will not be changed. */
	for (i = n - 1; i >= 0; --i) {
		if (jas_stream_ungetc(in, buf[i]) == EOF) {
			return -1;
		}
	}

	/* Did we read enough data? */
	if (n < PGX_MAGICLEN) {
		return -1;
	}

	/* Compute the signature value. */
	magic = (buf[0] << 8) | buf[1];

	/* Ensure that the signature is correct for this format. */
	if (magic != PGX_MAGIC) {
		return -1;
	}

	return 0;
}

/******************************************************************************\
*
\******************************************************************************/

static int pgx_gethdr(jas_stream_t *in, pgx_hdr_t *hdr)
{
	int c;
	uchar buf[2];

	if ((c = jas_stream_getc(in)) == EOF) {
		goto error;
	}
	buf[0] = c;
	if ((c = jas_stream_getc(in)) == EOF) {
		goto error;
	}
	buf[1] = c;
	hdr->magic = buf[0] << 8 | buf[1];
	if (hdr->magic != PGX_MAGIC) {
		goto error;
	}
	if ((c = pgx_getc(in)) == EOF || !isspace(c)) {
		goto error;
	}
	if (pgx_getbyteorder(in, &hdr->bigendian)) {
		goto error;
	}
	if (pgx_getsgnd(in, &hdr->sgnd)) {
		goto error;
	}
	if (pgx_getuint32(in, &hdr->prec)) {
		goto error;
	}
	if (pgx_getuint32(in, &hdr->width)) {
		goto error;
	}
	if (pgx_getuint32(in, &hdr->height)) {
		goto error;
	}
	return 0;

error:
	return -1;
}

static int pgx_getdata(jas_stream_t *in, pgx_hdr_t *hdr, jas_image_t *image)
{
	jas_matrix_t *data;
	uint_fast32_t x;
	uint_fast32_t y;
	uint_fast32_t word;
	int_fast32_t v;

	data = 0;

	if (!(data = jas_matrix_create(1, hdr->width))) {
		goto error;
	}
	for (y = 0; y < hdr->height; ++y) {
		for (x = 0; x < hdr->width; ++x) {
			/* Need to adjust signed value. */
			if ((v = pgx_getword(in, hdr->bigendian, hdr->prec)) < 0) {
				goto error;
			}
			word = v;
			v = pgx_wordtoint(word, hdr->prec, hdr->sgnd);
			jas_matrix_set(data, 0, x, v);
		}
		if (jas_image_writecmpt(image, 0, 0, y, hdr->width, 1, data)) {
			goto error;
		}
	}
	jas_matrix_destroy(data);
	return 0;

error:
	if (data) {
		jas_matrix_destroy(data);
	}
	return -1;
}

static int_fast32_t pgx_getword(jas_stream_t *in, bool bigendian, int prec)
{
	uint_fast32_t val;
	int i;
	int j;
	int c;
	int wordsize;

	wordsize = (prec + 7) / 8;

	if (prec > 32) {
		goto error;
	}

	val = 0;
	for (i = 0; i < wordsize; ++i) {
		if ((c = jas_stream_getc(in)) == EOF) {
			goto error;
		}
		j = bigendian ? (wordsize - 1 - i) : i;
		val = val | ((c & 0xff) << (8 * j));
	}
	val &= (1 << prec) - 1;
	return val;

error:
	return -1;
}

static int pgx_getc(jas_stream_t *in)
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

static int pgx_getbyteorder(jas_stream_t *in, bool *bigendian)
{
	int c;
	char buf[2];

	do {
		if ((c = pgx_getc(in)) == EOF) {
			return -1;
		}
	} while (isspace(c));

	buf[0] = c;
	if ((c = pgx_getc(in)) == EOF) {
		goto error;
	}
	buf[1] = c;
	if (buf[0] == 'M' && buf[1] == 'L') {
		*bigendian = true;
	} else if (buf[0] == 'L' && buf[1] == 'M') {
		*bigendian = false;
	} else {
		goto error;
	}

	while ((c = pgx_getc(in)) != EOF && !isspace(c)) {
		;
	}
	if (c == EOF) {
		goto error;
	}
	return 0;

error:
	return -1;
}

static int pgx_getsgnd(jas_stream_t *in, bool *sgnd)
{
	int c;

	do {
		if ((c = pgx_getc(in)) == EOF) {
			return -1;
		}
	} while (isspace(c));

	if (c == '+') {
		*sgnd = false;
	} else if (c == '-') {
		*sgnd = true;
	} else {
		goto error;
	}
	while ((c = pgx_getc(in)) != EOF && !isspace(c)) {
		;
	}
	if (c == EOF) {
		goto error;
	}
	return 0;

error:
	return -1;
}

static int pgx_getuint32(jas_stream_t *in, uint_fast32_t *val)
{
	int c;
	uint_fast32_t v;

	do {
		if ((c = pgx_getc(in)) == EOF) {
			return -1;
		}
	} while (isspace(c));

	v = 0;
	while (isdigit(c)) {
		v = 10 * v + c - '0';
		if ((c = pgx_getc(in)) < 0) {
			return -1;
		}
	}
	if (!isspace(c)) {
		return -1;
	}
	*val = v;

	return 0;
}

static jas_seqent_t pgx_wordtoint(uint_fast32_t v, int prec, bool sgnd)
{
	jas_seqent_t ret;
	v &= (1 << prec) - 1;
	ret = (sgnd && (v & (1 << (prec - 1)))) ? (v - (1 << prec)) : v;
	return ret;
}

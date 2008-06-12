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
		fprintf(stderr, "The PNM format cannot be used to represent an image with this geometry.\n");
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

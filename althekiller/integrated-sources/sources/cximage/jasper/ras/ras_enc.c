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
		fprintf(stderr, "warning: ignoring RAS encoder options\n");
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
			fprintf(stderr, "The RAS format cannot be used to represent an image with this geometry.\n");
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

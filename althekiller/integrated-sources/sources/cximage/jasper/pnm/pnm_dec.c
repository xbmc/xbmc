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
		fprintf(stderr, "warning: ignoring options\n");
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
	if (pnm_getint16(in, &hdr->magic) || pnm_getsintstr(in, &hdr->width) ||
	  pnm_getsintstr(in, &hdr->height)) {
		return -1;
	}
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

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
 * JP2 Library
 *
 * $Id$
 */

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <assert.h>
#include "jasper/jas_malloc.h"
#include "jasper/jas_image.h"
#include "jasper/jas_stream.h"
#include "jasper/jas_cm.h"
#include "jasper/jas_icc.h"
#include "jp2_cod.h"

static uint_fast32_t jp2_gettypeasoc(int colorspace, int ctype);
static int clrspctojp2(jas_clrspc_t clrspc);

/******************************************************************************\
* Functions.
\******************************************************************************/

int jp2_encode(jas_image_t *image, jas_stream_t *out, char *optstr)
{
	jp2_box_t *box;
	jp2_ftyp_t *ftyp;
	jp2_ihdr_t *ihdr;
	jas_stream_t *tmpstream;
	int allcmptssame;
	jp2_bpcc_t *bpcc;
	long len;
	uint_fast16_t cmptno;
	jp2_colr_t *colr;
	char buf[4096];
	uint_fast32_t overhead;
	jp2_cdefchan_t *cdefchanent;
	jp2_cdef_t *cdef;
	int i;
	uint_fast32_t typeasoc;
jas_iccprof_t *iccprof;
jas_stream_t *iccstream;
int pos;
int needcdef;
int prec;
int sgnd;

	box = 0;
	tmpstream = 0;

	allcmptssame = 1;
	sgnd = jas_image_cmptsgnd(image, 0);
	prec = jas_image_cmptprec(image, 0);
	for (i = 1; i < jas_image_numcmpts(image); ++i) {
		if (jas_image_cmptsgnd(image, i) != sgnd ||
		  jas_image_cmptprec(image, i) != prec) {
			allcmptssame = 0;
			break;
		}
	}

	/* Output the signature box. */

	if (!(box = jp2_box_create(JP2_BOX_JP))) {
		goto error;
	}
	box->data.jp.magic = JP2_JP_MAGIC;
	if (jp2_box_put(box, out)) {
		goto error;
	}
	jp2_box_destroy(box);
	box = 0;

	/* Output the file type box. */

	if (!(box = jp2_box_create(JP2_BOX_FTYP))) {
		goto error;
	}
	ftyp = &box->data.ftyp;
	ftyp->majver = JP2_FTYP_MAJVER;
	ftyp->minver = JP2_FTYP_MINVER;
	ftyp->numcompatcodes = 1;
	ftyp->compatcodes[0] = JP2_FTYP_COMPATCODE;
	if (jp2_box_put(box, out)) {
		goto error;
	}
	jp2_box_destroy(box);
	box = 0;

	/*
	 * Generate the data portion of the JP2 header box.
	 * We cannot simply output the header for this box
	 * since we do not yet know the correct value for the length
	 * field.
	 */

	if (!(tmpstream = jas_stream_memopen(0, 0))) {
		goto error;
	}

	/* Generate image header box. */

	if (!(box = jp2_box_create(JP2_BOX_IHDR))) {
		goto error;
	}
	ihdr = &box->data.ihdr;
	ihdr->width = jas_image_width(image);
	ihdr->height = jas_image_height(image);
	ihdr->numcmpts = jas_image_numcmpts(image);
	ihdr->bpc = allcmptssame ? JP2_SPTOBPC(jas_image_cmptsgnd(image, 0),
	  jas_image_cmptprec(image, 0)) : JP2_IHDR_BPCNULL;
	ihdr->comptype = JP2_IHDR_COMPTYPE;
	ihdr->csunk = 0;
	ihdr->ipr = 0;
	if (jp2_box_put(box, tmpstream)) {
		goto error;
	}
	jp2_box_destroy(box);
	box = 0;

	/* Generate bits per component box. */

	if (!allcmptssame) {
		if (!(box = jp2_box_create(JP2_BOX_BPCC))) {
			goto error;
		}
		bpcc = &box->data.bpcc;
		bpcc->numcmpts = jas_image_numcmpts(image);
		if (!(bpcc->bpcs = jas_malloc(bpcc->numcmpts *
		  sizeof(uint_fast8_t)))) {
			goto error;
		}
		for (cmptno = 0; cmptno < bpcc->numcmpts; ++cmptno) {
			bpcc->bpcs[cmptno] = JP2_SPTOBPC(jas_image_cmptsgnd(image,
			  cmptno), jas_image_cmptprec(image, cmptno));
		}
		if (jp2_box_put(box, tmpstream)) {
			goto error;
		}
		jp2_box_destroy(box);
		box = 0;
	}

	/* Generate color specification box. */

	if (!(box = jp2_box_create(JP2_BOX_COLR))) {
		goto error;
	}
	colr = &box->data.colr;
	switch (jas_image_clrspc(image)) {
	case JAS_CLRSPC_SRGB:
	case JAS_CLRSPC_SYCBCR:
	case JAS_CLRSPC_SGRAY:
		colr->method = JP2_COLR_ENUM;
		colr->csid = clrspctojp2(jas_image_clrspc(image));
		colr->pri = JP2_COLR_PRI;
		colr->approx = 0;
		break;
	default:
		colr->method = JP2_COLR_ICC;
		colr->pri = JP2_COLR_PRI;
		colr->approx = 0;
		iccprof = jas_iccprof_createfromcmprof(jas_image_cmprof(image));
		assert(iccprof);
		iccstream = jas_stream_memopen(0, 0);
		assert(iccstream);
		if (jas_iccprof_save(iccprof, iccstream))
			abort();
		if ((pos = jas_stream_tell(iccstream)) < 0)
			abort();
		colr->iccplen = pos;
		colr->iccp = jas_malloc(pos);
		assert(colr->iccp);
		jas_stream_rewind(iccstream);
		if (jas_stream_read(iccstream, colr->iccp, colr->iccplen) != colr->iccplen)
			abort();
		jas_stream_close(iccstream);
		jas_iccprof_destroy(iccprof);
		break;
	}
	if (jp2_box_put(box, tmpstream)) {
		goto error;
	}
	jp2_box_destroy(box);
	box = 0;

	needcdef = 1;
	switch (jas_clrspc_fam(jas_image_clrspc(image))) {
	case JAS_CLRSPC_FAM_RGB:
		if (jas_image_cmpttype(image, 0) ==
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_R) &&
		  jas_image_cmpttype(image, 1) ==
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_G) &&
		  jas_image_cmpttype(image, 2) ==
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_B))
			needcdef = 0;
		break;
	case JAS_CLRSPC_FAM_YCBCR:
		if (jas_image_cmpttype(image, 0) ==
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_Y) &&
		  jas_image_cmpttype(image, 1) ==
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_CB) &&
		  jas_image_cmpttype(image, 2) ==
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_CR))
			needcdef = 0;
		break;
	case JAS_CLRSPC_FAM_GRAY:
		if (jas_image_cmpttype(image, 0) ==
		  JAS_IMAGE_CT_COLOR(JAS_IMAGE_CT_GRAY_Y))
			needcdef = 0;
		break;
	default:
		abort();
		break;
	}

	if (needcdef) {
		if (!(box = jp2_box_create(JP2_BOX_CDEF))) {
			goto error;
		}
		cdef = &box->data.cdef;
		cdef->numchans = jas_image_numcmpts(image);
		cdef->ents = jas_malloc(cdef->numchans * sizeof(jp2_cdefchan_t));
		for (i = 0; i < jas_image_numcmpts(image); ++i) {
			cdefchanent = &cdef->ents[i];
			cdefchanent->channo = i;
			typeasoc = jp2_gettypeasoc(jas_image_clrspc(image), jas_image_cmpttype(image, i));
			cdefchanent->type = typeasoc >> 16;
			cdefchanent->assoc = typeasoc & 0x7fff;
		}
		if (jp2_box_put(box, tmpstream)) {
			goto error;
		}
		jp2_box_destroy(box);
		box = 0;
	}

	/* Determine the total length of the JP2 header box. */

	len = jas_stream_tell(tmpstream);
	jas_stream_rewind(tmpstream);

	/*
	 * Output the JP2 header box and all of the boxes which it contains.
	 */

	if (!(box = jp2_box_create(JP2_BOX_JP2H))) {
		goto error;
	}
	box->len = len + JP2_BOX_HDRLEN;
	if (jp2_box_put(box, out)) {
		goto error;
	}
	jp2_box_destroy(box);
	box = 0;

	if (jas_stream_copy(out, tmpstream, len)) {
		goto error;
	}

	jas_stream_close(tmpstream);
	tmpstream = 0;

	/*
	 * Output the contiguous code stream box.
	 */

	if (!(box = jp2_box_create(JP2_BOX_JP2C))) {
		goto error;
	}
	box->len = 0;
	if (jp2_box_put(box, out)) {
		goto error;
	}
	jp2_box_destroy(box);
	box = 0;

	/* Output the JPEG-2000 code stream. */

	overhead = jas_stream_getrwcount(out);
	sprintf(buf, "%s\n_jp2overhead=%lu\n", (optstr ? optstr : ""),
	  (unsigned long) overhead);

	if (jpc_encode(image, out, buf)) {
		goto error;
	}

	return 0;
	abort();

error:

	if (box) {
		jp2_box_destroy(box);
	}
	if (tmpstream) {
		jas_stream_close(tmpstream);
	}
	return -1;
}

static uint_fast32_t jp2_gettypeasoc(int colorspace, int ctype)
{
	int type;
	int asoc;

	if (ctype & JAS_IMAGE_CT_OPACITY) {
		type = JP2_CDEF_TYPE_OPACITY;
		asoc = JP2_CDEF_ASOC_ALL;
		goto done;
	}

	type = JP2_CDEF_TYPE_UNSPEC;
	asoc = JP2_CDEF_ASOC_NONE;
	switch (jas_clrspc_fam(colorspace)) {
	case JAS_CLRSPC_FAM_RGB:
		switch (JAS_IMAGE_CT_COLOR(ctype)) {
		case JAS_IMAGE_CT_RGB_R:
			type = JP2_CDEF_TYPE_COLOR;
			asoc = JP2_CDEF_RGB_R;
			break;
		case JAS_IMAGE_CT_RGB_G:
			type = JP2_CDEF_TYPE_COLOR;
			asoc = JP2_CDEF_RGB_G;
			break;
		case JAS_IMAGE_CT_RGB_B:
			type = JP2_CDEF_TYPE_COLOR;
			asoc = JP2_CDEF_RGB_B;
			break;
		}
		break;
	case JAS_CLRSPC_FAM_YCBCR:
		switch (JAS_IMAGE_CT_COLOR(ctype)) {
		case JAS_IMAGE_CT_YCBCR_Y:
			type = JP2_CDEF_TYPE_COLOR;
			asoc = JP2_CDEF_YCBCR_Y;
			break;
		case JAS_IMAGE_CT_YCBCR_CB:
			type = JP2_CDEF_TYPE_COLOR;
			asoc = JP2_CDEF_YCBCR_CB;
			break;
		case JAS_IMAGE_CT_YCBCR_CR:
			type = JP2_CDEF_TYPE_COLOR;
			asoc = JP2_CDEF_YCBCR_CR;
			break;
		}
		break;
	case JAS_CLRSPC_FAM_GRAY:
		type = JP2_CDEF_TYPE_COLOR;
		asoc = JP2_CDEF_GRAY_Y;
		break;
	}

done:
	return (type << 16) | asoc;
}

static int clrspctojp2(jas_clrspc_t clrspc)
{
	switch (clrspc) {
	case JAS_CLRSPC_SRGB:
		return JP2_COLR_SRGB;
	case JAS_CLRSPC_SYCBCR:
		return JP2_COLR_SYCC;
	case JAS_CLRSPC_SGRAY:
		return JP2_COLR_SGRAY;
	default:
		abort();
		break;
	}
}

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

#include "jasper/jas_image.h"
#include "jasper/jas_stream.h"
#include "jasper/jas_math.h"
#include "jasper/jas_debug.h"
#include "jasper/jas_malloc.h"
#include "jasper/jas_version.h"

#include "jp2_cod.h"
#include "jp2_dec.h"

#define	JP2_VALIDATELEN	(JAS_MIN(JP2_JP_LEN + 16, JAS_STREAM_MAXPUTBACK))

static jp2_dec_t *jp2_dec_create(void);
static void jp2_dec_destroy(jp2_dec_t *dec);
static int jp2_getcs(jp2_colr_t *colr);
static int fromiccpcs(int cs);
static int jp2_getct(int colorspace, int type, int assoc);

/******************************************************************************\
* Functions.
\******************************************************************************/

jas_image_t *jp2_decode(jas_stream_t *in, char *optstr)
{
	jp2_box_t *box;
	int found;
	jas_image_t *image;
	jp2_dec_t *dec;
	bool samedtype;
	int dtype;
	unsigned int i;
	jp2_cmap_t *cmapd;
	jp2_pclr_t *pclrd;
	jp2_cdef_t *cdefd;
	unsigned int channo;
	int newcmptno;
	int_fast32_t *lutents;
#if 0
	jp2_cdefchan_t *cdefent;
	int cmptno;
#endif
	jp2_cmapent_t *cmapent;
	jas_icchdr_t icchdr;
	jas_iccprof_t *iccprof;

	dec = 0;
	box = 0;
	image = 0;

	if (!(dec = jp2_dec_create())) {
		goto error;
	}

	/* Get the first box.  This should be a JP box. */
	if (!(box = jp2_box_get(in))) {
		jas_eprintf("error: cannot get box\n");
		goto error;
	}
	if (box->type != JP2_BOX_JP) {
		jas_eprintf("error: expecting signature box\n");
		goto error;
	}
	if (box->data.jp.magic != JP2_JP_MAGIC) {
		jas_eprintf("incorrect magic number\n");
		goto error;
	}
	jp2_box_destroy(box);
	box = 0;

	/* Get the second box.  This should be a FTYP box. */
	if (!(box = jp2_box_get(in))) {
		goto error;
	}
	if (box->type != JP2_BOX_FTYP) {
		jas_eprintf("expecting file type box\n");
		goto error;
	}
	jp2_box_destroy(box);
	box = 0;

	/* Get more boxes... */
	found = 0;
	while ((box = jp2_box_get(in))) {
		if (jas_getdbglevel() >= 1) {
			fprintf(stderr, "box type %s\n", box->info->name);
		}
		switch (box->type) {
		case JP2_BOX_JP2C:
			found = 1;
			break;
		case JP2_BOX_IHDR:
			if (!dec->ihdr) {
				dec->ihdr = box;
				box = 0;
			}
			break;
		case JP2_BOX_BPCC:
			if (!dec->bpcc) {
				dec->bpcc = box;
				box = 0;
			}
			break;
		case JP2_BOX_CDEF:
			if (!dec->cdef) {
				dec->cdef = box;
				box = 0;
			}
			break;
		case JP2_BOX_PCLR:
			if (!dec->pclr) {
				dec->pclr = box;
				box = 0;
			}
			break;
		case JP2_BOX_CMAP:
			if (!dec->cmap) {
				dec->cmap = box;
				box = 0;
			}
			break;
		case JP2_BOX_COLR:
			if (!dec->colr) {
				dec->colr = box;
				box = 0;
			}
			break;
		}
		if (box) {
			jp2_box_destroy(box);
			box = 0;
		}
		if (found) {
			break;
		}
	}

	if (!found) {
		jas_eprintf("error: no code stream found\n");
		goto error;
	}

	if (!(dec->image = jpc_decode(in, optstr))) {
		jas_eprintf("error: cannot decode code stream\n");
		goto error;
	}

	/* An IHDR box must be present. */
	if (!dec->ihdr) {
		jas_eprintf("error: missing IHDR box\n");
		goto error;
	}

	/* Does the number of components indicated in the IHDR box match
	  the value specified in the code stream? */
	if (dec->ihdr->data.ihdr.numcmpts != JAS_CAST(uint, jas_image_numcmpts(dec->image))) {
		jas_eprintf("warning: number of components mismatch\n");
	}

	/* At least one component must be present. */
	if (!jas_image_numcmpts(dec->image)) {
		jas_eprintf("error: no components\n");
		goto error;
	}

	/* Determine if all components have the same data type. */
	samedtype = true;
	dtype = jas_image_cmptdtype(dec->image, 0);
	for (i = 1; i < JAS_CAST(uint, jas_image_numcmpts(dec->image)); ++i) {
		if (jas_image_cmptdtype(dec->image, i) != dtype) {
			samedtype = false;
			break;
		}
	}

	/* Is the component data type indicated in the IHDR box consistent
	  with the data in the code stream? */
	if ((samedtype && dec->ihdr->data.ihdr.bpc != JP2_DTYPETOBPC(dtype)) ||
	  (!samedtype && dec->ihdr->data.ihdr.bpc != JP2_IHDR_BPCNULL)) {
		jas_eprintf("warning: component data type mismatch\n");
	}

	/* Is the compression type supported? */
	if (dec->ihdr->data.ihdr.comptype != JP2_IHDR_COMPTYPE) {
		jas_eprintf("error: unsupported compression type\n");
		goto error;
	}

	if (dec->bpcc) {
		/* Is the number of components indicated in the BPCC box
		  consistent with the code stream data? */
		if (dec->bpcc->data.bpcc.numcmpts != JAS_CAST(uint, jas_image_numcmpts(
		  dec->image))) {
			jas_eprintf("warning: number of components mismatch\n");
		}
		/* Is the component data type information indicated in the BPCC
		  box consistent with the code stream data? */
		if (!samedtype) {
			for (i = 0; i < JAS_CAST(uint, jas_image_numcmpts(dec->image)); ++i) {
				if (jas_image_cmptdtype(dec->image, i) != JP2_BPCTODTYPE(dec->bpcc->data.bpcc.bpcs[i])) {
					jas_eprintf("warning: component data type mismatch\n");
				}
			}
		} else {
			jas_eprintf("warning: superfluous BPCC box\n");
		}
	}

	/* A COLR box must be present. */
	if (!dec->colr) {
		jas_eprintf("error: no COLR box\n");
		goto error;
	}

	switch (dec->colr->data.colr.method) {
	case JP2_COLR_ENUM:
		jas_image_setclrspc(dec->image, jp2_getcs(&dec->colr->data.colr));
		break;
	case JP2_COLR_ICC:
		iccprof = jas_iccprof_createfrombuf(dec->colr->data.colr.iccp,
		  dec->colr->data.colr.iccplen);
		assert(iccprof);
		jas_iccprof_gethdr(iccprof, &icchdr);
		jas_eprintf("ICC Profile CS %08x\n", icchdr.colorspc);
		jas_image_setclrspc(dec->image, fromiccpcs(icchdr.colorspc));
		dec->image->cmprof_ = jas_cmprof_createfromiccprof(iccprof);
		assert(dec->image->cmprof_);
		jas_iccprof_destroy(iccprof);
		break;
	}

	/* If a CMAP box is present, a PCLR box must also be present. */
	if (dec->cmap && !dec->pclr) {
		jas_eprintf("warning: missing PCLR box or superfluous CMAP box\n");
		jp2_box_destroy(dec->cmap);
		dec->cmap = 0;
	}

	/* If a CMAP box is not present, a PCLR box must not be present. */
	if (!dec->cmap && dec->pclr) {
		jas_eprintf("warning: missing CMAP box or superfluous PCLR box\n");
		jp2_box_destroy(dec->pclr);
		dec->pclr = 0;
	}

	/* Determine the number of channels (which is essentially the number
	  of components after any palette mappings have been applied). */
	dec->numchans = dec->cmap ? dec->cmap->data.cmap.numchans : JAS_CAST(uint, jas_image_numcmpts(dec->image));

	/* Perform a basic sanity check on the CMAP box if present. */
	if (dec->cmap) {
		for (i = 0; i < dec->numchans; ++i) {
			/* Is the component number reasonable? */
			if (dec->cmap->data.cmap.ents[i].cmptno >= JAS_CAST(uint, jas_image_numcmpts(dec->image))) {
				jas_eprintf("error: invalid component number in CMAP box\n");
				goto error;
			}
			/* Is the LUT index reasonable? */
			if (dec->cmap->data.cmap.ents[i].pcol >= dec->pclr->data.pclr.numchans) {
				jas_eprintf("error: invalid CMAP LUT index\n");
				goto error;
			}
		}
	}

	/* Allocate space for the channel-number to component-number LUT. */
	if (!(dec->chantocmptlut = jas_malloc(dec->numchans * sizeof(uint_fast16_t)))) {
		jas_eprintf("error: no memory\n");
		goto error;
	}

	if (!dec->cmap) {
		for (i = 0; i < dec->numchans; ++i) {
			dec->chantocmptlut[i] = i;
		}
	} else {
		cmapd = &dec->cmap->data.cmap;
		pclrd = &dec->pclr->data.pclr;
		cdefd = &dec->cdef->data.cdef;
		for (channo = 0; channo < cmapd->numchans; ++channo) {
			cmapent = &cmapd->ents[channo];
			if (cmapent->map == JP2_CMAP_DIRECT) {
				dec->chantocmptlut[channo] = channo;
			} else if (cmapent->map == JP2_CMAP_PALETTE) {
				lutents = jas_malloc(pclrd->numlutents * sizeof(int_fast32_t));
				for (i = 0; i < pclrd->numlutents; ++i) {
					lutents[i] = pclrd->lutdata[cmapent->pcol + i * pclrd->numchans];
				}
				newcmptno = jas_image_numcmpts(dec->image);
				jas_image_depalettize(dec->image, cmapent->cmptno, pclrd->numlutents, lutents, JP2_BPCTODTYPE(pclrd->bpc[cmapent->pcol]), newcmptno);
				dec->chantocmptlut[channo] = newcmptno;
				jas_free(lutents);
#if 0
				if (dec->cdef) {
					cdefent = jp2_cdef_lookup(cdefd, channo);
					if (!cdefent) {
						abort();
					}
				jas_image_setcmpttype(dec->image, newcmptno, jp2_getct(jas_image_clrspc(dec->image), cdefent->type, cdefent->assoc));
				} else {
				jas_image_setcmpttype(dec->image, newcmptno, jp2_getct(jas_image_clrspc(dec->image), 0, channo + 1));
				}
#endif
			}
		}
	}

	/* Mark all components as being of unknown type. */

	for (i = 0; i < JAS_CAST(uint, jas_image_numcmpts(dec->image)); ++i) {
		jas_image_setcmpttype(dec->image, i, JAS_IMAGE_CT_UNKNOWN);
	}

	/* Determine the type of each component. */
	if (dec->cdef) {
		for (i = 0; i < dec->numchans; ++i) {
			jas_image_setcmpttype(dec->image,
			  dec->chantocmptlut[dec->cdef->data.cdef.ents[i].channo],
			  jp2_getct(jas_image_clrspc(dec->image),
			  dec->cdef->data.cdef.ents[i].type, dec->cdef->data.cdef.ents[i].assoc));
		}
	} else {
		for (i = 0; i < dec->numchans; ++i) {
			jas_image_setcmpttype(dec->image, dec->chantocmptlut[i],
			  jp2_getct(jas_image_clrspc(dec->image), 0, i + 1));
		}
	}

	/* Delete any components that are not of interest. */
	for (i = jas_image_numcmpts(dec->image); i > 0; --i) {
		if (jas_image_cmpttype(dec->image, i - 1) == JAS_IMAGE_CT_UNKNOWN) {
			jas_image_delcmpt(dec->image, i - 1);
		}
	}

	/* Ensure that some components survived. */
	if (!jas_image_numcmpts(dec->image)) {
		jas_eprintf("error: no components\n");
		goto error;
	}
#if 0
fprintf(stderr, "no of components is %d\n", jas_image_numcmpts(dec->image));
#endif

	/* Prevent the image from being destroyed later. */
	image = dec->image;
	dec->image = 0;

	jp2_dec_destroy(dec);

	return image;

error:
	if (box) {
		jp2_box_destroy(box);
	}
	if (dec) {
		jp2_dec_destroy(dec);
	}
	return 0;
}

int jp2_validate(jas_stream_t *in)
{
	char buf[JP2_VALIDATELEN];
	int i;
	int n;
#if 0
	jas_stream_t *tmpstream;
	jp2_box_t *box;
#endif

	assert(JAS_STREAM_MAXPUTBACK >= JP2_VALIDATELEN);

	/* Read the validation data (i.e., the data used for detecting
	  the format). */
	if ((n = jas_stream_read(in, buf, JP2_VALIDATELEN)) < 0) {
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
	if (n < JP2_VALIDATELEN) {
		return -1;
	}

	/* Is the box type correct? */
	if (((buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7]) !=
	  JP2_BOX_JP)
	{
		return -1;
	}

	return 0;
}

static jp2_dec_t *jp2_dec_create(void)
{
	jp2_dec_t *dec;

	if (!(dec = jas_malloc(sizeof(jp2_dec_t)))) {
		return 0;
	}
	dec->ihdr = 0;
	dec->bpcc = 0;
	dec->cdef = 0;
	dec->pclr = 0;
	dec->image = 0;
	dec->chantocmptlut = 0;
	dec->cmap = 0;
	dec->colr = 0;
	return dec;
}

static void jp2_dec_destroy(jp2_dec_t *dec)
{
	if (dec->ihdr) {
		jp2_box_destroy(dec->ihdr);
	}
	if (dec->bpcc) {
		jp2_box_destroy(dec->bpcc);
	}
	if (dec->cdef) {
		jp2_box_destroy(dec->cdef);
	}
	if (dec->pclr) {
		jp2_box_destroy(dec->pclr);
	}
	if (dec->image) {
		jas_image_destroy(dec->image);
	}
	if (dec->cmap) {
		jp2_box_destroy(dec->cmap);
	}
	if (dec->colr) {
		jp2_box_destroy(dec->colr);
	}
	if (dec->chantocmptlut) {
		jas_free(dec->chantocmptlut);
	}
	jas_free(dec);
}

static int jp2_getct(int colorspace, int type, int assoc)
{
	if (type == 1 && assoc == 0) {
		return JAS_IMAGE_CT_OPACITY;
	}
	if (type == 0 && assoc >= 1 && assoc <= 65534) {
		switch (colorspace) {
		case JAS_CLRSPC_FAM_RGB:
			switch (assoc) {
			case JP2_CDEF_RGB_R:
				return JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_R);
				break;
			case JP2_CDEF_RGB_G:
				return JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_G);
				break;
			case JP2_CDEF_RGB_B:
				return JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_B);
				break;
			}
			break;
		case JAS_CLRSPC_FAM_YCBCR:
			switch (assoc) {
			case JP2_CDEF_YCBCR_Y:
				return JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_Y);
				break;
			case JP2_CDEF_YCBCR_CB:
				return JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_CB);
				break;
			case JP2_CDEF_YCBCR_CR:
				return JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_CR);
				break;
			}
			break;
		case JAS_CLRSPC_FAM_GRAY:
			switch (assoc) {
			case JP2_CDEF_GRAY_Y:
				return JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_GRAY_Y);
				break;
			}
			break;
		default:
			return JAS_IMAGE_CT_COLOR(assoc - 1);
			break;
		}
	}
	return JAS_IMAGE_CT_UNKNOWN;
}

static int jp2_getcs(jp2_colr_t *colr)
{
	if (colr->method == JP2_COLR_ENUM) {
		switch (colr->csid) {
		case JP2_COLR_SRGB:
			return JAS_CLRSPC_SRGB;
			break;
		case JP2_COLR_SYCC:
			return JAS_CLRSPC_SYCBCR;
			break;
		case JP2_COLR_SGRAY:
			return JAS_CLRSPC_SGRAY;
			break;
		}
	}
	return JAS_CLRSPC_UNKNOWN;
}

static int fromiccpcs(int cs)
{
	switch (cs) {
	case ICC_CS_RGB:
		return JAS_CLRSPC_GENRGB;
		break;
	case ICC_CS_YCBCR:
		return JAS_CLRSPC_GENYCBCR;
		break;
	case ICC_CS_GRAY:
		return JAS_CLRSPC_GENGRAY;
		break;
	}
	return JAS_CLRSPC_UNKNOWN;
}

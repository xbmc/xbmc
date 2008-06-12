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
#include "jasper/jas_types.h"

#include "jasper/jas_tvp.h"
#include "jasper/jas_stream.h"
#include "jasper/jas_image.h"
#include "jasper/jas_string.h"
#include "jasper/jas_debug.h"

#include "jpg_jpeglib.h"
#include "jpg_cod.h"
#include "jpg_enc.h"

/******************************************************************************\
* Types.
\******************************************************************************/

typedef struct jpg_src_s {

	/* Output buffer. */
	JSAMPARRAY buffer;

	/* Height of output buffer. */
	JDIMENSION buffer_height;

	/* The current row. */
	JDIMENSION row;

	/* The image used to hold the decompressed sample data. */
	jas_image_t *image;

	/* The row buffer. */
	jas_matrix_t *data;

	/* The error indicator.  If this is nonzero, something has gone wrong
	  during decompression. */
	int error;

	jpg_enc_t *enc;

} jpg_src_t;

typedef struct {
	int qual;
} jpg_encopts_t;

typedef enum {
	OPT_QUAL
} jpg_optid_t;

jas_taginfo_t jpg_opttab[] = {
	{OPT_QUAL, "quality"},
	{-1, 0}
};

/******************************************************************************\
* Local prototypes.
\******************************************************************************/

static int jpg_copyfiletostream(jas_stream_t *out, FILE *in);
static void jpg_start_input(j_compress_ptr cinfo, struct jpg_src_s *sinfo);
static JDIMENSION jpg_get_pixel_rows(j_compress_ptr cinfo, struct jpg_src_s *sinfo);
static void jpg_finish_input(j_compress_ptr cinfo, struct jpg_src_s *sinfo);
static int tojpgcs(int colorspace);
static int jpg_parseencopts(char *optstr, jpg_encopts_t *encopts);

/******************************************************************************\
*
\******************************************************************************/

static int jpg_copyfiletostream(jas_stream_t *out, FILE *in)
{
	int c;
	while ((c = fgetc(in)) != EOF) {
		if (jas_stream_putc(out, c) == EOF) {
			return -1;
		}
	}
	return 0;
}

static void jpg_start_input(j_compress_ptr cinfo, struct jpg_src_s *sinfo)
{
	/* Avoid compiler warnings about unused parameters. */
	cinfo = 0;

	sinfo->row = 0;
}

static JDIMENSION jpg_get_pixel_rows(j_compress_ptr cinfo, struct jpg_src_s *sinfo)
{
	JSAMPLE *bufptr;
	int i;
	int cmptno;
	int width;
	int *cmpts;

	cmpts = sinfo->enc->cmpts;

	width = jas_image_width(sinfo->image);

	if (sinfo->error) {
		return 0;
	}
	for (cmptno = 0; cmptno < cinfo->input_components; ++cmptno) {
		if (jas_image_readcmpt(sinfo->image, cmpts[cmptno], 0, sinfo->row, width, 1, sinfo->data)) {
			;
		}
		bufptr = (sinfo->buffer[0]) + cmptno;
		for (i = 0; i < width; ++i) {
			*bufptr = jas_matrix_get(sinfo->data, 0, i);
			bufptr += cinfo->input_components;
		}
	}
	++sinfo->row;
	return 1;
}

static void jpg_finish_input(j_compress_ptr cinfo, struct jpg_src_s *sinfo)
{
	/* Avoid compiler warnings about unused parameters. */
	cinfo = 0;
	sinfo = 0;
}

/******************************************************************************\
* Code for save operation.
\******************************************************************************/

/* Save an image to a stream in the the JPG format. */

int jpg_encode(jas_image_t *image, jas_stream_t *out, char *optstr)
{
	JDIMENSION numscanlines;
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	jas_image_coord_t width;
	jas_image_coord_t height;
	jpg_src_t src_mgr_buf;
	jpg_src_t *src_mgr = &src_mgr_buf;
	FILE *output_file;
	int cmptno;
	jpg_enc_t encbuf;
	jpg_enc_t *enc = &encbuf;
	jpg_encopts_t encopts;

	output_file = 0;

	if (jpg_parseencopts(optstr, &encopts))
		goto error;

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
			goto error;
		}
		break;
	case JAS_CLRSPC_FAM_YCBCR:
		if (jas_image_clrspc(image) != JAS_CLRSPC_SYCBCR)
			jas_eprintf("warning: inaccurate color\n");
		enc->numcmpts = 3;
		if ((enc->cmpts[0] = jas_image_getcmptbytype(image,
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_Y))) < 0 ||
		  (enc->cmpts[1] = jas_image_getcmptbytype(image,
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_CB))) < 0 ||
		  (enc->cmpts[2] = jas_image_getcmptbytype(image,
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_CR))) < 0) {
			jas_eprintf("error: missing color component\n");
			goto error;
		}
		break;
	case JAS_CLRSPC_FAM_GRAY:
		if (jas_image_clrspc(image) != JAS_CLRSPC_SGRAY)
			jas_eprintf("warning: inaccurate color\n");
		enc->numcmpts = 1;
		if ((enc->cmpts[0] = jas_image_getcmptbytype(image,
		  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_GRAY_Y))) < 0) {
			jas_eprintf("error: missing color component\n");
			goto error;
		}
		break;
	default:
		jas_eprintf("error: JPG format does not support color space\n");
		goto error;
		break;
	}

	width = jas_image_width(image);
	height = jas_image_height(image);

	for (cmptno = 0; cmptno < enc->numcmpts; ++cmptno) {
		if (jas_image_cmptwidth(image, enc->cmpts[cmptno]) != width ||
		  jas_image_cmptheight(image, enc->cmpts[cmptno]) != height ||
		  jas_image_cmpttlx(image, enc->cmpts[cmptno]) != 0 ||
		  jas_image_cmpttly(image, enc->cmpts[cmptno]) != 0 ||
		  jas_image_cmpthstep(image, enc->cmpts[cmptno]) != 1 ||
		  jas_image_cmptvstep(image, enc->cmpts[cmptno]) != 1 ||
		  jas_image_cmptprec(image, enc->cmpts[cmptno]) != 8 ||
		  jas_image_cmptsgnd(image, enc->cmpts[cmptno]) != false) {
			jas_eprintf("error: The JPG encoder cannot handle an image with this geometry.\n");
			goto error;
		}
	}

	if (!(output_file = tmpfile())) {
		goto error;
	}

	/* Create a JPEG compression object. */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	/* Specify data destination for compression */
	jpeg_stdio_dest(&cinfo, output_file);

	cinfo.in_color_space = tojpgcs(jas_image_clrspc(image));
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = enc->numcmpts;
	jpeg_set_defaults(&cinfo);

	src_mgr->error = 0;
	src_mgr->image = image;
	src_mgr->data = jas_matrix_create(1, width);
	src_mgr->buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo,
	  JPOOL_IMAGE, (JDIMENSION) width * cinfo.input_components,
	  (JDIMENSION) 1);
	src_mgr->buffer_height = 1;
	src_mgr->enc = enc;

	/* Read the input file header to obtain file size & colorspace. */
	jpg_start_input(&cinfo, src_mgr);

	if (encopts.qual >= 0) {
		jpeg_set_quality(&cinfo, encopts.qual, TRUE);
	}

	/* Now that we know input colorspace, fix colorspace-dependent defaults */
	jpeg_default_colorspace(&cinfo);

	/* Start compressor */
	jpeg_start_compress(&cinfo, TRUE);

	/* Process data */
	while (cinfo.next_scanline < cinfo.image_height) {
		if ((numscanlines = jpg_get_pixel_rows(&cinfo, src_mgr)) <= 0) {
			break;
		}
		jpeg_write_scanlines(&cinfo, src_mgr->buffer, numscanlines);
	}

	/* Finish compression and release memory */
	jpg_finish_input(&cinfo, src_mgr);
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	rewind(output_file);
	jpg_copyfiletostream(out, output_file);
	fclose(output_file);
	output_file = 0;

	return 0;

error:
	if (output_file) {
		fclose(output_file);
	}
	return -1;
}

static int tojpgcs(int colorspace)
{
	switch (jas_clrspc_fam(colorspace)) {
	case JAS_CLRSPC_FAM_RGB:
		return JCS_RGB;
		break;
	case JAS_CLRSPC_FAM_YCBCR:
		return JCS_YCbCr;
		break;
	case JAS_CLRSPC_FAM_GRAY:
		return JCS_GRAYSCALE;
		break;
	default:
		abort();
		break;
	}
}

/* Parse the encoder options string. */
static int jpg_parseencopts(char *optstr, jpg_encopts_t *encopts)
{
	jas_tvparser_t *tvp;
	char *qual_str;
	int ret;

	tvp = 0;

	/* Initialize default values for encoder options. */
	encopts->qual = -1;

	/* Create the tag-value parser. */
	if (!(tvp = jas_tvparser_create(optstr ? optstr : ""))) {
		goto error;
	}

	/* Get tag-value pairs, and process as necessary. */
	while (!(ret = jas_tvparser_next(tvp))) {
		switch (jas_taginfo_nonull(jas_taginfos_lookup(jpg_opttab,
		  jas_tvparser_gettag(tvp)))->id) {
		case OPT_QUAL:
			qual_str = jas_tvparser_getval(tvp);
			if (sscanf(qual_str, "%d", &encopts->qual) != 1) {
				fprintf(stderr,
					"ignoring bad quality specifier %s\n",
					jas_tvparser_getval(tvp));
				encopts->qual = -1;
			}
			break;
		default:
			fprintf(stderr, "warning: ignoring invalid option %s\n",
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

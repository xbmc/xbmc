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
static J_COLOR_SPACE tojpgcs(int colorspace);
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
	assert(src_mgr->data);
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
	jas_matrix_destroy(src_mgr->data);

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

static J_COLOR_SPACE tojpgcs(int colorspace)
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
				jas_eprintf("ignoring bad quality specifier %s\n",
					jas_tvparser_getval(tvp));
				encopts->qual = -1;
			}
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

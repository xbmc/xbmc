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

#include <stdio.h>
#include <assert.h>
#include <ctype.h>

#include "jasper/jas_tvp.h"
#include "jasper/jas_stream.h"
#include "jasper/jas_image.h"
#include "jasper/jas_string.h"

#include "jpg_jpeglib.h"
#include "jpg_cod.h"

/******************************************************************************\
* Types.
\******************************************************************************/

/* JPEG decoder data sink type. */

typedef struct jpg_dest_s {

	/* Initialize output. */
	void (*start_output)(j_decompress_ptr cinfo, struct jpg_dest_s *dinfo);

	/* Output rows of decompressed data. */
	void (*put_pixel_rows)(j_decompress_ptr cinfo, struct jpg_dest_s *dinfo,
	  JDIMENSION rows_supplied);

	/* Cleanup output. */
	void (*finish_output)(j_decompress_ptr cinfo, struct jpg_dest_s *dinfo);

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

} jpg_dest_t;

/******************************************************************************\
* Local functions.
\******************************************************************************/

static void jpg_start_output(j_decompress_ptr cinfo, jpg_dest_t *dinfo);
static void jpg_put_pixel_rows(j_decompress_ptr cinfo, jpg_dest_t *dinfo,
  JDIMENSION rows_supplied);
static void jpg_finish_output(j_decompress_ptr cinfo, jpg_dest_t *dinfo);
static int jpg_copystreamtofile(FILE *out, jas_stream_t *in);
static jas_image_t *jpg_mkimage(j_decompress_ptr cinfo);

/******************************************************************************\
* Code for load operation.
\******************************************************************************/

/* Load an image from a stream in the JPG format. */

jas_image_t *jpg_decode(jas_stream_t *in, char *optstr)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE *input_file;
	jpg_dest_t dest_mgr_buf;
	jpg_dest_t *dest_mgr = &dest_mgr_buf;
	int num_scanlines;
	jas_image_t *image;

	/* Avoid compiler warnings about unused parameters. */
	optstr = 0;

	image = 0;
	input_file = 0;
	if (!(input_file = tmpfile())) {
		goto error;
	}
	if (jpg_copystreamtofile(input_file, in)) {
		goto error;
	}
	rewind(input_file);

	/* Allocate and initialize a JPEG decompression object. */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	/* Specify the data source for decompression. */
	jpeg_stdio_src(&cinfo, input_file);

	/* Read the file header to obtain the image information. */
	jpeg_read_header(&cinfo, TRUE);

	/* Start the decompressor. */
	jpeg_start_decompress(&cinfo);

	/* Create an image object to hold the decoded data. */
	if (!(image = jpg_mkimage(&cinfo))) {
		goto error;
	}

	/* Initialize the data sink object. */
	dest_mgr->image = image;
	dest_mgr->data = jas_matrix_create(1, cinfo.output_width);
	dest_mgr->start_output = jpg_start_output;
	dest_mgr->put_pixel_rows = jpg_put_pixel_rows;
	dest_mgr->finish_output = jpg_finish_output;
    dest_mgr->buffer = (*cinfo.mem->alloc_sarray)
      ((j_common_ptr) &cinfo, JPOOL_IMAGE,
       cinfo.output_width * cinfo.output_components, (JDIMENSION) 1);
	dest_mgr->buffer_height = 1;
	dest_mgr->error = 0;

	/* Process the compressed data. */
	(*dest_mgr->start_output)(&cinfo, dest_mgr);
	while (cinfo.output_scanline < cinfo.output_height) {
		num_scanlines = jpeg_read_scanlines(&cinfo, dest_mgr->buffer,
		  dest_mgr->buffer_height);
		(*dest_mgr->put_pixel_rows)(&cinfo, dest_mgr, num_scanlines);
	}
	(*dest_mgr->finish_output)(&cinfo, dest_mgr);

	/* Complete the decompression process. */
	jpeg_finish_decompress(&cinfo);

	/* Destroy the JPEG decompression object. */
	jpeg_destroy_decompress(&cinfo);

	jas_matrix_destroy(dest_mgr->data);

	fclose(input_file);

	if (dest_mgr->error) {
		goto error;
	}

	return image;

error:
	if (image) {
		jas_image_destroy(image);
	}
	if (input_file) {
		fclose(input_file);
	}
	return 0;
}

/******************************************************************************\
*
\******************************************************************************/

static jas_image_t *jpg_mkimage(j_decompress_ptr cinfo)
{
	jas_image_t *image;
	int cmptno;
	jas_image_cmptparm_t cmptparm;
	int numcmpts;

	image = 0;
	numcmpts = cinfo->output_components;
	if (!(image = jas_image_create0())) {
		goto error;
	}
	for (cmptno = 0; cmptno < numcmpts; ++cmptno) {
		cmptparm.tlx = 0;
		cmptparm.tly = 0;
		cmptparm.hstep = 1;
		cmptparm.vstep = 1;
		cmptparm.width = cinfo->image_width;
		cmptparm.height = cinfo->image_height;
		cmptparm.prec = 8;
		cmptparm.sgnd = false;
		if (jas_image_addcmpt(image, cmptno, &cmptparm)) {
			goto error;
		}
	}

	if (numcmpts == 3) {
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

	return image;

error:
	if (image) {
		jas_image_destroy(image);
	}
	return 0;
}

/******************************************************************************\
* Data source code.
\******************************************************************************/

static int jpg_copystreamtofile(FILE *out, jas_stream_t *in)
{
	int c;

	while ((c = jas_stream_getc(in)) != EOF) {
		if (fputc(c, out) == EOF) {
			return -1;
		}
	}
	if (jas_stream_error(in)) {
		return -1;
	}
	return 0;
}

/******************************************************************************\
* Data sink code.
\******************************************************************************/

static void jpg_start_output(j_decompress_ptr cinfo, jpg_dest_t *dinfo)
{
	/* Avoid compiler warnings about unused parameters. */
	cinfo = 0;

	dinfo->row = 0;
}

static void jpg_put_pixel_rows(j_decompress_ptr cinfo, jpg_dest_t *dinfo,
  JDIMENSION rows_supplied)
{
	JSAMPLE *bufptr;
	int cmptno;
	JDIMENSION x;
	uint_fast32_t width;

	if (dinfo->error) {
		return;
	}

	assert(cinfo->output_components == jas_image_numcmpts(dinfo->image));

	for (cmptno = 0; cmptno < cinfo->output_components; ++cmptno) {
		width = jas_image_cmptwidth(dinfo->image, cmptno);
		bufptr = (dinfo->buffer[0]) + cmptno;
		for (x = 0; x < width; ++x) {
			jas_matrix_set(dinfo->data, 0, x, GETJSAMPLE(*bufptr));
			bufptr += cinfo->output_components;
		}
		if (jas_image_writecmpt(dinfo->image, cmptno, 0, dinfo->row, width, 1,
		  dinfo->data)) {
			dinfo->error = 1;
		}
	}
	dinfo->row += rows_supplied;
}

static void jpg_finish_output(j_decompress_ptr cinfo, jpg_dest_t *dinfo)
{
	/* Avoid compiler warnings about unused parameters. */
	cinfo = 0;
	dinfo = 0;
}

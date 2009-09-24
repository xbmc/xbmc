/*
 * Copyright (c) 1999-2000 Image Power, Inc. and the University of
 *   British Columbia.
 * Copyright (c) 2001-2002 Michael David Adams.
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
 * Sequence/Matrix Library
 *
 * $Id$
 */

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "jasper/jas_seq.h"
#include "jasper/jas_malloc.h"
#include "jasper/jas_math.h"

/******************************************************************************\
* Constructors and destructors.
\******************************************************************************/

jas_matrix_t *jas_seq2d_create(int xstart, int ystart, int xend, int yend)
{
	jas_matrix_t *matrix;
	assert(xstart <= xend && ystart <= yend);
	if (!(matrix = jas_matrix_create(yend - ystart, xend - xstart))) {
		return 0;
	}
	matrix->xstart_ = xstart;
	matrix->ystart_ = ystart;
	matrix->xend_ = xend;
	matrix->yend_ = yend;
	return matrix;
}

jas_matrix_t *jas_matrix_create(int numrows, int numcols)
{
	jas_matrix_t *matrix;
	int i;

	if (!(matrix = jas_malloc(sizeof(jas_matrix_t)))) {
		return 0;
	}
	matrix->flags_ = 0;
	matrix->numrows_ = numrows;
	matrix->numcols_ = numcols;
	matrix->rows_ = 0;
	matrix->maxrows_ = numrows;
	matrix->data_ = 0;
	matrix->datasize_ = numrows * numcols;

	if (matrix->maxrows_ > 0) {
		if (!(matrix->rows_ = jas_malloc(matrix->maxrows_ *
		  sizeof(jas_seqent_t *)))) {
			jas_matrix_destroy(matrix);
			return 0;
		}
	}

	if (matrix->datasize_ > 0) {
		if (!(matrix->data_ = jas_malloc(matrix->datasize_ *
		  sizeof(jas_seqent_t)))) {
			jas_matrix_destroy(matrix);
			return 0;
		}
	}

	for (i = 0; i < numrows; ++i) {
		matrix->rows_[i] = &matrix->data_[i * matrix->numcols_];
	}

	for (i = 0; i < matrix->datasize_; ++i) {
		matrix->data_[i] = 0;
	}

	matrix->xstart_ = 0;
	matrix->ystart_ = 0;
	matrix->xend_ = matrix->numcols_;
	matrix->yend_ = matrix->numrows_;

	return matrix;
}

void jas_matrix_destroy(jas_matrix_t *matrix)
{
	if (matrix->data_) {
		assert(!(matrix->flags_ & JAS_MATRIX_REF));
		jas_free(matrix->data_);
		matrix->data_ = 0;
	}
	if (matrix->rows_) {
		jas_free(matrix->rows_);
		matrix->rows_ = 0;
	}
	jas_free(matrix);
}

jas_seq2d_t *jas_seq2d_copy(jas_seq2d_t *x)
{
	jas_matrix_t *y;
	int i;
	int j;
	y = jas_seq2d_create(jas_seq2d_xstart(x), jas_seq2d_ystart(x), jas_seq2d_xend(x),
	  jas_seq2d_yend(x));
	assert(y);
	for (i = 0; i < x->numrows_; ++i) {
		for (j = 0; j < x->numcols_; ++j) {
			*jas_matrix_getref(y, i, j) = jas_matrix_get(x, i, j);
		}
	}
	return y;
}

jas_matrix_t *jas_matrix_copy(jas_matrix_t *x)
{
	jas_matrix_t *y;
	int i;
	int j;
	y = jas_matrix_create(x->numrows_, x->numcols_);
	for (i = 0; i < x->numrows_; ++i) {
		for (j = 0; j < x->numcols_; ++j) {
			*jas_matrix_getref(y, i, j) = jas_matrix_get(x, i, j);
		}
	}
	return y;
}

/******************************************************************************\
* Bind operations.
\******************************************************************************/

void jas_seq2d_bindsub(jas_matrix_t *s, jas_matrix_t *s1, int xstart, int ystart,
  int xend, int yend)
{
	jas_matrix_bindsub(s, s1, ystart - s1->ystart_, xstart - s1->xstart_,
	  yend - s1->ystart_ - 1, xend - s1->xstart_ - 1);
}

void jas_matrix_bindsub(jas_matrix_t *mat0, jas_matrix_t *mat1, int r0, int c0,
  int r1, int c1)
{
	int i;

	if (mat0->data_) {
		if (!(mat0->flags_ & JAS_MATRIX_REF)) {
			jas_free(mat0->data_);
		}
		mat0->data_ = 0;
		mat0->datasize_ = 0;
	}
	if (mat0->rows_) {
		jas_free(mat0->rows_);
		mat0->rows_ = 0;
	}
	mat0->flags_ |= JAS_MATRIX_REF;
	mat0->numrows_ = r1 - r0 + 1;
	mat0->numcols_ = c1 - c0 + 1;
	mat0->maxrows_ = mat0->numrows_;
	mat0->rows_ = jas_malloc(mat0->maxrows_ * sizeof(jas_seqent_t *));
	for (i = 0; i < mat0->numrows_; ++i) {
		mat0->rows_[i] = mat1->rows_[r0 + i] + c0;
	}

	mat0->xstart_ = mat1->xstart_ + c0;
	mat0->ystart_ = mat1->ystart_ + r0;
	mat0->xend_ = mat0->xstart_ + mat0->numcols_;
	mat0->yend_ = mat0->ystart_ + mat0->numrows_;
}

/******************************************************************************\
* Arithmetic operations.
\******************************************************************************/

int jas_matrix_cmp(jas_matrix_t *mat0, jas_matrix_t *mat1)
{
	int i;
	int j;

	if (mat0->numrows_ != mat1->numrows_ || mat0->numcols_ !=
	  mat1->numcols_) {
		return 1;
	}
	for (i = 0; i < mat0->numrows_; i++) {
		for (j = 0; j < mat0->numcols_; j++) {
			if (jas_matrix_get(mat0, i, j) != jas_matrix_get(mat1, i, j)) {
				return 1;
			}
		}
	}
	return 0;
}

void jas_matrix_divpow2(jas_matrix_t *matrix, int n)
{
	int i;
	int j;
	jas_seqent_t *rowstart;
	int rowstep;
	jas_seqent_t *data;

	rowstep = jas_matrix_rowstep(matrix);
	for (i = matrix->numrows_, rowstart = matrix->rows_[0]; i > 0; --i,
	  rowstart += rowstep) {
		for (j = matrix->numcols_, data = rowstart; j > 0; --j,
		  ++data) {
			*data = (*data >= 0) ? ((*data) >> n) :
			  (-((-(*data)) >> n));
		}
	}
}

void jas_matrix_clip(jas_matrix_t *matrix, jas_seqent_t minval, jas_seqent_t maxval)
{
	int i;
	int j;
	jas_seqent_t v;
	jas_seqent_t *rowstart;
	jas_seqent_t *data;
	int rowstep;

	rowstep = jas_matrix_rowstep(matrix);
	for (i = matrix->numrows_, rowstart = matrix->rows_[0]; i > 0; --i,
	  rowstart += rowstep) {
		data = rowstart;
		for (j = matrix->numcols_, data = rowstart; j > 0; --j,
		  ++data) {
			v = *data;
			if (v < minval) {
				*data = minval;
			} else if (v > maxval) {
				*data = maxval;
			}
		}
	}
}

void jas_matrix_asr(jas_matrix_t *matrix, int n)
{
	int i;
	int j;
	jas_seqent_t *rowstart;
	int rowstep;
	jas_seqent_t *data;

	assert(n >= 0);
	rowstep = jas_matrix_rowstep(matrix);
	for (i = matrix->numrows_, rowstart = matrix->rows_[0]; i > 0; --i,
	  rowstart += rowstep) {
		for (j = matrix->numcols_, data = rowstart; j > 0; --j,
		  ++data) {
			*data >>= n;
		}
	}
}

void jas_matrix_asl(jas_matrix_t *matrix, int n)
{
	int i;
	int j;
	jas_seqent_t *rowstart;
	int rowstep;
	jas_seqent_t *data;

	rowstep = jas_matrix_rowstep(matrix);
	for (i = matrix->numrows_, rowstart = matrix->rows_[0]; i > 0; --i,
	  rowstart += rowstep) {
		for (j = matrix->numcols_, data = rowstart; j > 0; --j,
		  ++data) {
			*data <<= n;
		}
	}
}

/******************************************************************************\
* Code.
\******************************************************************************/

int jas_matrix_resize(jas_matrix_t *matrix, int numrows, int numcols)
{
	int size;
	int i;

	size = numrows * numcols;
	if (size > matrix->datasize_ || numrows > matrix->maxrows_) {
		return -1;
	}

	matrix->numrows_ = numrows;
	matrix->numcols_ = numcols;

	for (i = 0; i < numrows; ++i) {
		matrix->rows_[i] = &matrix->data_[numcols * i];
	}

	return 0;
}

int jas_matrix_output(jas_matrix_t *matrix, FILE *out)
{
	int i;
	int j;
	jas_seqent_t x;

	fprintf(out, "%d %d\n", jas_matrix_numrows(matrix), jas_matrix_numcols(matrix));
	for (i = 0; i < jas_matrix_numrows(matrix); ++i) {
		for (j = 0; j < jas_matrix_numcols(matrix); ++j) {
			x = jas_matrix_get(matrix, i, j);
			fprintf(out, "%ld", JAS_CAST(long, x));
			if (j < jas_matrix_numcols(matrix) - 1) {
				fprintf(out, " ");
			}
		}
		fprintf(out, "\n");
	}
	return 0;
}

void jas_matrix_setall(jas_matrix_t *matrix, jas_seqent_t val)
{
	int i;
	int j;
	jas_seqent_t *rowstart;
	int rowstep;
	jas_seqent_t *data;

	rowstep = jas_matrix_rowstep(matrix);
	for (i = matrix->numrows_, rowstart = matrix->rows_[0]; i > 0; --i,
	  rowstart += rowstep) {
		for (j = matrix->numcols_, data = rowstart; j > 0; --j,
		  ++data) {
			*data = val;
		}
	}
}

#if 0
jas_matrix_t *jas_matrix_input(FILE *in)
{
	jas_matrix_t *matrix;
	int i;
	int j;
	long x;
	int numrows;
	int numcols;

	if (fscanf(in, "%d %d", &numrows, &numcols) != 2)
		return 0;
	if (!(matrix = jas_matrix_create(numrows, numcols)))
		return 0;

	/* Get matrix data. */
	for (i = 0; i < jas_matrix_numrows(matrix); i++) {
		for (j = 0; j < jas_matrix_numcols(matrix); j++) {
			if (fscanf(in, "%ld", &x) != 1) {
				jas_matrix_destroy(matrix);
				return 0;
			}
			jas_matrix_set(matrix, i, j, JAS_CAST(jas_seqent_t, x));
		}
	}

	return matrix;
}
#endif

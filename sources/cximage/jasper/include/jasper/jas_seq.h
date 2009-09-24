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

#ifndef JAS_SEQ_H
#define JAS_SEQ_H

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <jasper/jas_config.h>

#include <jasper/jas_stream.h>
#include <jasper/jas_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
* Constants.
\******************************************************************************/

/* This matrix is a reference to another matrix. */
#define JAS_MATRIX_REF	0x0001

/******************************************************************************\
* Types.
\******************************************************************************/

/* An element in a sequence. */
typedef int_fast32_t jas_seqent_t;

/* An element in a matrix. */
typedef int_fast32_t jas_matent_t;

/* Matrix. */

typedef struct {

	/* Additional state information. */
	int flags_;

	/* The starting horizontal index. */
	int_fast32_t xstart_;

	/* The starting vertical index. */
	int_fast32_t ystart_;

	/* The ending horizontal index. */
	int_fast32_t xend_;

	/* The ending vertical index. */
	int_fast32_t yend_;

	/* The number of rows in the matrix. */
	int_fast32_t numrows_;

	/* The number of columns in the matrix. */
	int_fast32_t numcols_;

	/* Pointers to the start of each row. */
	jas_seqent_t **rows_;

	/* The allocated size of the rows array. */
	int_fast32_t maxrows_;

	/* The matrix data buffer. */
	jas_seqent_t *data_;

	/* The allocated size of the data array. */
	int_fast32_t datasize_;

} jas_matrix_t;

typedef jas_matrix_t jas_seq2d_t;
typedef jas_matrix_t jas_seq_t;

/******************************************************************************\
* Functions/macros for matrix class.
\******************************************************************************/

/* Get the number of rows. */
#define jas_matrix_numrows(matrix) \
	((matrix)->numrows_)

/* Get the number of columns. */
#define jas_matrix_numcols(matrix) \
	((matrix)->numcols_)

/* Get a matrix element. */
#define jas_matrix_get(matrix, i, j) \
	((matrix)->rows_[i][j])

/* Set a matrix element. */
#define jas_matrix_set(matrix, i, j, v) \
	((matrix)->rows_[i][j] = (v))

/* Get an element from a matrix that is known to be a row or column vector. */
#define jas_matrix_getv(matrix, i) \
	(((matrix)->numrows_ == 1) ? ((matrix)->rows_[0][i]) : \
	  ((matrix)->rows_[i][0]))

/* Set an element in a matrix that is known to be a row or column vector. */
#define jas_matrix_setv(matrix, i, v) \
	(((matrix)->numrows_ == 1) ? ((matrix)->rows_[0][i] = (v)) : \
	  ((matrix)->rows_[i][0] = (v)))

/* Get the address of an element in a matrix. */
#define	jas_matrix_getref(matrix, i, j) \
	(&(matrix)->rows_[i][j])

#define	jas_matrix_getvref(matrix, i) \
	(((matrix)->numrows_ > 1) ? jas_matrix_getref(matrix, i, 0) : jas_matrix_getref(matrix, 0, i))

#define jas_matrix_length(matrix) \
	(max((matrix)->numrows_, (matrix)->numcols_))

/* Create a matrix with the specified dimensions. */
jas_matrix_t *jas_matrix_create(int numrows, int numcols);

/* Destroy a matrix. */
void jas_matrix_destroy(jas_matrix_t *matrix);

/* Resize a matrix.  The previous contents of the matrix are lost. */
int jas_matrix_resize(jas_matrix_t *matrix, int numrows, int numcols);

int jas_matrix_output(jas_matrix_t *matrix, FILE *out);

/* Create a matrix that references part of another matrix. */
void jas_matrix_bindsub(jas_matrix_t *mat0, jas_matrix_t *mat1, int r0, int c0,
  int r1, int c1);

/* Create a matrix that is a reference to a row of another matrix. */
#define jas_matrix_bindrow(mat0, mat1, r) \
  (jas_matrix_bindsub((mat0), (mat1), (r), 0, (r), (mat1)->numcols_ - 1))

/* Create a matrix that is a reference to a column of another matrix. */
#define jas_matrix_bindcol(mat0, mat1, c) \
  (jas_matrix_bindsub((mat0), (mat1), 0, (c), (mat1)->numrows_ - 1, (c)))

/* Clip the values of matrix elements to the specified range. */
void jas_matrix_clip(jas_matrix_t *matrix, jas_seqent_t minval,
  jas_seqent_t maxval);

/* Arithmetic shift left of all elements in a matrix. */
void jas_matrix_asl(jas_matrix_t *matrix, int n);

/* Arithmetic shift right of all elements in a matrix. */
void jas_matrix_asr(jas_matrix_t *matrix, int n);

/* Almost-but-not-quite arithmetic shift right of all elements in a matrix. */
void jas_matrix_divpow2(jas_matrix_t *matrix, int n);

/* Set all elements of a matrix to the specified value. */
void jas_matrix_setall(jas_matrix_t *matrix, jas_seqent_t val);

/* The spacing between rows of a matrix. */
#define	jas_matrix_rowstep(matrix) \
	(((matrix)->numrows_ > 1) ? ((matrix)->rows_[1] - (matrix)->rows_[0]) : (0))

/* The spacing between columns of a matrix. */
#define	jas_matrix_step(matrix) \
	(((matrix)->numrows_ > 1) ? (jas_matrix_rowstep(matrix)) : (1))

/* Compare two matrices for equality. */
int jas_matrix_cmp(jas_matrix_t *mat0, jas_matrix_t *mat1);

jas_matrix_t *jas_matrix_copy(jas_matrix_t *x);

/******************************************************************************\
* Functions/macros for 2-D sequence class.
\******************************************************************************/

jas_seq2d_t *jas_seq2d_copy(jas_seq2d_t *x);

jas_matrix_t *jas_seq2d_create(int xstart, int ystart, int xend, int yend);

#define	jas_seq2d_destroy(s) \
	jas_matrix_destroy(s)

#define	jas_seq2d_xstart(s) \
	((s)->xstart_)
#define	jas_seq2d_ystart(s) \
	((s)->ystart_)
#define	jas_seq2d_xend(s) \
	((s)->xend_)
#define	jas_seq2d_yend(s) \
	((s)->yend_)
#define	jas_seq2d_getref(s, x, y) \
	(jas_matrix_getref(s, (y) - (s)->ystart_, (x) - (s)->xstart_))
#define	jas_seq2d_get(s, x, y) \
	(jas_matrix_get(s, (y) - (s)->ystart_, (x) - (s)->xstart_))
#define	jas_seq2d_rowstep(s) \
	jas_matrix_rowstep(s)
#define	jas_seq2d_width(s) \
	((s)->xend_ - (s)->xstart_)
#define	jas_seq2d_height(s) \
	((s)->yend_ - (s)->ystart_)
#define	jas_seq2d_setshift(s, x, y) \
	((s)->xstart_ = (x), (s)->ystart_ = (y), \
	  (s)->xend_ = (s)->xstart_ + (s)->numcols_, \
	  (s)->yend_ = (s)->ystart_ + (s)->numrows_)

void jas_seq2d_bindsub(jas_matrix_t *s, jas_matrix_t *s1, int xstart,
  int ystart, int xend, int yend);

/******************************************************************************\
* Functions/macros for 1-D sequence class.
\******************************************************************************/

#define	jas_seq_create(start, end) \
	(jas_seq2d_create(start, 0, end, 1))

#define	jas_seq_destroy(seq) \
	(jas_seq2d_destroy(seq))

#define jas_seq_set(seq, i, v) \
	((seq)->rows_[0][(i) - (seq)->xstart_] = (v))
#define	jas_seq_getref(seq, i) \
	(&(seq)->rows_[0][(i) - (seq)->xstart_])
#define	jas_seq_get(seq, i) \
	((seq)->rows_[0][(i) - (seq)->xstart_])
#define	jas_seq_start(seq) \
	((seq)->xstart_)
#define	jas_seq_end(seq) \
	((seq)->xend_)

#ifdef __cplusplus
}
#endif

#endif

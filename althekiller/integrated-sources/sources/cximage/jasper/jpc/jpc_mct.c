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
 * Multicomponent Transform Code
 *
 * $Id$
 */

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <assert.h>

#include "jasper/jas_seq.h"

#include "jpc_fix.h"
#include "jpc_mct.h"

/******************************************************************************\
* Code.
\******************************************************************************/

/* Compute the forward RCT. */

void jpc_rct(jas_matrix_t *c0, jas_matrix_t *c1, jas_matrix_t *c2)
{
	int numrows;
	int numcols;
	int i;
	int j;
	jpc_fix_t *c0p;
	jpc_fix_t *c1p;
	jpc_fix_t *c2p;

	numrows = jas_matrix_numrows(c0);
	numcols = jas_matrix_numcols(c0);

	/* All three matrices must have the same dimensions. */
	assert(jas_matrix_numrows(c1) == numrows && jas_matrix_numcols(c1) == numcols
	  && jas_matrix_numrows(c2) == numrows && jas_matrix_numcols(c2) == numcols);

	for (i = 0; i < numrows; i++) {
		c0p = jas_matrix_getref(c0, i, 0);
		c1p = jas_matrix_getref(c1, i, 0);
		c2p = jas_matrix_getref(c2, i, 0);
		for (j = numcols; j > 0; --j) {
			int r;
			int g;
			int b;
			int y;
			int u;
			int v;
			r = *c0p;
			g = *c1p;
			b = *c2p;
			y = (r + (g << 1) + b) >> 2;
			u = b - g;
			v = r - g;
			*c0p++ = y;
			*c1p++ = u;
			*c2p++ = v;
		}
	}
}

/* Compute the inverse RCT. */

void jpc_irct(jas_matrix_t *c0, jas_matrix_t *c1, jas_matrix_t *c2)
{
	int numrows;
	int numcols;
	int i;
	int j;
	jpc_fix_t *c0p;
	jpc_fix_t *c1p;
	jpc_fix_t *c2p;

	numrows = jas_matrix_numrows(c0);
	numcols = jas_matrix_numcols(c0);

	/* All three matrices must have the same dimensions. */
	assert(jas_matrix_numrows(c1) == numrows && jas_matrix_numcols(c1) == numcols
	  && jas_matrix_numrows(c2) == numrows && jas_matrix_numcols(c2) == numcols);

	for (i = 0; i < numrows; i++) {
		c0p = jas_matrix_getref(c0, i, 0);
		c1p = jas_matrix_getref(c1, i, 0);
		c2p = jas_matrix_getref(c2, i, 0);
		for (j = numcols; j > 0; --j) {
			int r;
			int g;
			int b;
			int y;
			int u;
			int v;
			y = *c0p;
			u = *c1p;
			v = *c2p;
			g = y - ((u + v) >> 2);
			r = v + g;
			b = u + g;
			*c0p++ = r;
			*c1p++ = g;
			*c2p++ = b;
		}
	}
}

void jpc_ict(jas_matrix_t *c0, jas_matrix_t *c1, jas_matrix_t *c2)
{
	int numrows;
	int numcols;
	int i;
	int j;
	jpc_fix_t r;
	jpc_fix_t g;
	jpc_fix_t b;
	jpc_fix_t y;
	jpc_fix_t u;
	jpc_fix_t v;
	jpc_fix_t *c0p;
	jpc_fix_t *c1p;
	jpc_fix_t *c2p;

	numrows = jas_matrix_numrows(c0);
	assert(jas_matrix_numrows(c1) == numrows && jas_matrix_numrows(c2) == numrows);
	numcols = jas_matrix_numcols(c0);
	assert(jas_matrix_numcols(c1) == numcols && jas_matrix_numcols(c2) == numcols);
	for (i = 0; i < numrows; ++i) {
		c0p = jas_matrix_getref(c0, i, 0);
		c1p = jas_matrix_getref(c1, i, 0);
		c2p = jas_matrix_getref(c2, i, 0);
		for (j = numcols; j > 0; --j) {
			r = *c0p;
			g = *c1p;
			b = *c2p;
			y = jpc_fix_add3(jpc_fix_mul(jpc_dbltofix(0.299), r), jpc_fix_mul(jpc_dbltofix(0.587), g),
			  jpc_fix_mul(jpc_dbltofix(0.114), b));
			u = jpc_fix_add3(jpc_fix_mul(jpc_dbltofix(-0.16875), r), jpc_fix_mul(jpc_dbltofix(-0.33126), g),
			  jpc_fix_mul(jpc_dbltofix(0.5), b));
			v = jpc_fix_add3(jpc_fix_mul(jpc_dbltofix(0.5), r), jpc_fix_mul(jpc_dbltofix(-0.41869), g),
			  jpc_fix_mul(jpc_dbltofix(-0.08131), b));
			*c0p++ = y;
			*c1p++ = u;
			*c2p++ = v;
		}
	}
}

void jpc_iict(jas_matrix_t *c0, jas_matrix_t *c1, jas_matrix_t *c2)
{
	int numrows;
	int numcols;
	int i;
	int j;
	jpc_fix_t r;
	jpc_fix_t g;
	jpc_fix_t b;
	jpc_fix_t y;
	jpc_fix_t u;
	jpc_fix_t v;
	jpc_fix_t *c0p;
	jpc_fix_t *c1p;
	jpc_fix_t *c2p;

	numrows = jas_matrix_numrows(c0);
	assert(jas_matrix_numrows(c1) == numrows && jas_matrix_numrows(c2) == numrows);
	numcols = jas_matrix_numcols(c0);
	assert(jas_matrix_numcols(c1) == numcols && jas_matrix_numcols(c2) == numcols);
	for (i = 0; i < numrows; ++i) {
		c0p = jas_matrix_getref(c0, i, 0);
		c1p = jas_matrix_getref(c1, i, 0);
		c2p = jas_matrix_getref(c2, i, 0);
		for (j = numcols; j > 0; --j) {
			y = *c0p;
			u = *c1p;
			v = *c2p;
			r = jpc_fix_add(y, jpc_fix_mul(jpc_dbltofix(1.402), v));
			g = jpc_fix_add3(y, jpc_fix_mul(jpc_dbltofix(-0.34413), u),
			  jpc_fix_mul(jpc_dbltofix(-0.71414), v));
			b = jpc_fix_add(y, jpc_fix_mul(jpc_dbltofix(1.772), u));
			*c0p++ = r;
			*c1p++ = g;
			*c2p++ = b;
		}
	}
}

jpc_fix_t jpc_mct_getsynweight(int mctid, int cmptno)
{
	jpc_fix_t synweight;

	synweight = JPC_FIX_ONE;
	switch (mctid) {
	case JPC_MCT_RCT:
		switch (cmptno) {
		case 0:
			synweight = jpc_dbltofix(sqrt(3.0));
			break;
		case 1:
			synweight = jpc_dbltofix(sqrt(0.6875));
			break;
		case 2:
			synweight = jpc_dbltofix(sqrt(0.6875));
			break;
		}
		break;
	case JPC_MCT_ICT:
		switch (cmptno) {
		case 0:
			synweight = jpc_dbltofix(sqrt(3.0000));
			break;
		case 1:
			synweight = jpc_dbltofix(sqrt(3.2584));
			break;
		case 2:
			synweight = jpc_dbltofix(sqrt(2.4755));
			break;
		}
		break;
#if 0
	default:
		synweight = JPC_FIX_ONE;
		break;
#endif
	}

	return synweight;
}

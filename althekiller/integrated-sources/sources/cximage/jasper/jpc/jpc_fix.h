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
 * Fixed-Point Number Class
 *
 * $Id$
 */

#ifndef JPC_FIX_H
#define JPC_FIX_H

/******************************************************************************\
* Includes.
\******************************************************************************/

#include "jasper/jas_types.h"
#include "jasper/jas_fix.h"

/******************************************************************************\
* Basic parameters of the fixed-point type.
\******************************************************************************/

/* The integral type used to represent a fixed-point number.  This
  type must be capable of representing values from -(2^31) to 2^31-1
  (inclusive). */
typedef int_fast32_t jpc_fix_t;

/* The integral type used to respresent higher-precision intermediate results.
  This type should be capable of representing values from -(2^63) to 2^63-1
  (inclusive). */
typedef int_fast64_t jpc_fix_big_t;

/* The number of bits used for the fractional part of a fixed-point number. */
#define JPC_FIX_FRACBITS	13

/******************************************************************************\
* Instantiations of the generic fixed-point number macros for the
* parameters given above.  (Too bad C does not support templates, eh?)
* The purpose of these macros is self-evident if one examines the
* corresponding macros in the jasper/jas_fix.h header file.
\******************************************************************************/

#define	JPC_FIX_ZERO	JAS_FIX_ZERO(jpc_fix_t, JPC_FIX_FRACBITS)
#define	JPC_FIX_ONE		JAS_FIX_ONE(jpc_fix_t, JPC_FIX_FRACBITS)
#define	JPC_FIX_HALF	JAS_FIX_HALF(jpc_fix_t, JPC_FIX_FRACBITS)

#define jpc_inttofix(x)	JAS_INTTOFIX(jpc_fix_t, JPC_FIX_FRACBITS, x)
#define jpc_fixtoint(x)	JAS_FIXTOINT(jpc_fix_t, JPC_FIX_FRACBITS, x)
#define jpc_fixtodbl(x)	JAS_FIXTODBL(jpc_fix_t, JPC_FIX_FRACBITS, x)
#define jpc_dbltofix(x)	JAS_DBLTOFIX(jpc_fix_t, JPC_FIX_FRACBITS, x)

#define	jpc_fix_add(x, y)	JAS_FIX_ADD(jpc_fix_t, JPC_FIX_FRACBITS, x, y)
#define	jpc_fix_sub(x, y)	JAS_FIX_SUB(jpc_fix_t, JPC_FIX_FRACBITS, x, y)
#define	jpc_fix_mul(x, y) \
	JAS_FIX_MUL(jpc_fix_t, JPC_FIX_FRACBITS, jpc_fix_big_t, x, y)
#define	jpc_fix_mulbyint(x, y) \
	JAS_FIX_MULBYINT(jpc_fix_t, JPC_FIX_FRACBITS, x, y)
#define	jpc_fix_div(x, y) \
	JAS_FIX_DIV(jpc_fix_t, JPC_FIX_FRACBITS, jpc_fix_big_t, x, y)
#define	jpc_fix_neg(x)		JAS_FIX_NEG(jpc_fix_t, JPC_FIX_FRACBITS, x)
#define	jpc_fix_asl(x, n)	JAS_FIX_ASL(jpc_fix_t, JPC_FIX_FRACBITS, x, n)
#define	jpc_fix_asr(x, n)	JAS_FIX_ASR(jpc_fix_t, JPC_FIX_FRACBITS, x, n)

#define jpc_fix_pluseq(x, y)	JAS_FIX_PLUSEQ(jpc_fix_t, JPC_FIX_FRACBITS, x, y)
#define jpc_fix_minuseq(x, y)	JAS_FIX_MINUSEQ(jpc_fix_t, JPC_FIX_FRACBITS, x, y)
#define	jpc_fix_muleq(x, y)	\
	JAS_FIX_MULEQ(jpc_fix_t, JPC_FIX_FRACBITS, jpc_fix_big_t, x, y)

#define	jpc_fix_abs(x)		JAS_FIX_ABS(jpc_fix_t, JPC_FIX_FRACBITS, x)
#define	jpc_fix_isint(x)	JAS_FIX_ISINT(jpc_fix_t, JPC_FIX_FRACBITS, x)
#define jpc_fix_sgn(x)		JAS_FIX_SGN(jpc_fix_t, JPC_FIX_FRACBITS, x)
#define	jpc_fix_round(x)	JAS_FIX_ROUND(jpc_fix_t, JPC_FIX_FRACBITS, x)
#define	jpc_fix_floor(x)	JAS_FIX_FLOOR(jpc_fix_t, JPC_FIX_FRACBITS, x)
#define jpc_fix_trunc(x)	JAS_FIX_TRUNC(jpc_fix_t, JPC_FIX_FRACBITS, x)

/******************************************************************************\
* Extra macros for convenience.
\******************************************************************************/

/* Compute the sum of three fixed-point numbers. */
#define jpc_fix_add3(x, y, z)	jpc_fix_add(jpc_fix_add(x, y), z)

#endif

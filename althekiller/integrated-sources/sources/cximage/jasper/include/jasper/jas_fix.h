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

#ifndef JAS_FIX_H
#define JAS_FIX_H

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <jasper/jas_config.h>
#include <jasper/jas_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
* Constants.
\******************************************************************************/

/* The representation of the value zero. */
#define	JAS_FIX_ZERO(fix_t, fracbits) \
	JAS_CAST(fix_t, 0)

/* The representation of the value one. */
#define	JAS_FIX_ONE(fix_t, fracbits) \
	(JAS_CAST(fix_t, 1) << (fracbits))

/* The representation of the value one half. */
#define	JAS_FIX_HALF(fix_t, fracbits) \
	(JAS_CAST(fix_t, 1) << ((fracbits) - 1))

/******************************************************************************\
* Conversion operations.
\******************************************************************************/

/* Convert an int to a fixed-point number. */
#define JAS_INTTOFIX(fix_t, fracbits, x) \
	JAS_CAST(fix_t, (x) << (fracbits))

/* Convert a fixed-point number to an int. */
#define JAS_FIXTOINT(fix_t, fracbits, x) \
	JAS_CAST(int, (x) >> (fracbits))

/* Convert a fixed-point number to a double. */
#define JAS_FIXTODBL(fix_t, fracbits, x) \
	(JAS_CAST(double, x) / (JAS_CAST(fix_t, 1) << (fracbits)))

/* Convert a double to a fixed-point number. */
#define JAS_DBLTOFIX(fix_t, fracbits, x) \
	JAS_CAST(fix_t, ((x) * JAS_CAST(double, JAS_CAST(fix_t, 1) << (fracbits))))

/******************************************************************************\
* Basic arithmetic operations.
* All other arithmetic operations are synthesized from these basic operations.
* There are three macros for each type of arithmetic operation.
* One macro always performs overflow/underflow checking, one never performs
* overflow/underflow checking, and one is generic with its behavior
* depending on compile-time flags.
* Only the generic macros should be invoked directly by application code.
\******************************************************************************/

/* Calculate the sum of two fixed-point numbers. */
#if !defined(DEBUG_OVERFLOW)
#define JAS_FIX_ADD			JAS_FIX_ADD_FAST
#else
#define JAS_FIX_ADD			JAS_FIX_ADD_OFLOW
#endif

/* Calculate the sum of two fixed-point numbers without overflow checking. */
#define	JAS_FIX_ADD_FAST(fix_t, fracbits, x, y)	((x) + (y))

/* Calculate the sum of two fixed-point numbers with overflow checking. */
#define	JAS_FIX_ADD_OFLOW(fix_t, fracbits, x, y) \
	((x) >= 0) ? \
	  (((y) >= 0) ? ((x) + (y) >= 0 || JAS_FIX_OFLOW(), (x) + (y)) : \
	  ((x) + (y))) : \
	  (((y) >= 0) ? ((x) + (y)) : ((x) + (y) < 0 || JAS_FIX_OFLOW(), \
	  (x) + (y)))

/* Calculate the product of two fixed-point numbers. */
#if !defined(DEBUG_OVERFLOW)
#define JAS_FIX_MUL			JAS_FIX_MUL_FAST
#else
#define JAS_FIX_MUL			JAS_FIX_MUL_OFLOW
#endif

/* Calculate the product of two fixed-point numbers without overflow
  checking. */
#define	JAS_FIX_MUL_FAST(fix_t, fracbits, bigfix_t, x, y) \
	JAS_CAST(fix_t, (JAS_CAST(bigfix_t, x) * JAS_CAST(bigfix_t, y)) >> \
	  (fracbits))

/* Calculate the product of two fixed-point numbers with overflow
  checking. */
#define JAS_FIX_MUL_OFLOW(fix_t, fracbits, bigfix_t, x, y) \
	((JAS_CAST(bigfix_t, x) * JAS_CAST(bigfix_t, y) >> (fracbits)) == \
	  JAS_CAST(fix_t, (JAS_CAST(bigfix_t, x) * JAS_CAST(bigfix_t, y) >> \
	  (fracbits))) ? \
	  JAS_CAST(fix_t, (JAS_CAST(bigfix_t, x) * JAS_CAST(bigfix_t, y) >> \
	  (fracbits))) : JAS_FIX_OFLOW())

/* Calculate the product of a fixed-point number and an int. */
#if !defined(DEBUG_OVERFLOW)
#define	JAS_FIX_MULBYINT	JAS_FIX_MULBYINT_FAST
#else
#define	JAS_FIX_MULBYINT	JAS_FIX_MULBYINT_OFLOW
#endif

/* Calculate the product of a fixed-point number and an int without overflow
  checking. */
#define	JAS_FIX_MULBYINT_FAST(fix_t, fracbits, x, y) \
	JAS_CAST(fix_t, ((x) * (y)))

/* Calculate the product of a fixed-point number and an int with overflow
  checking. */
#define	JAS_FIX_MULBYINT_OFLOW(fix_t, fracbits, x, y) \
	JAS_FIX_MULBYINT_FAST(fix_t, fracbits, x, y)

/* Calculate the quotient of two fixed-point numbers. */
#if !defined(DEBUG_OVERFLOW)
#define JAS_FIX_DIV			JAS_FIX_DIV_FAST
#else
#define JAS_FIX_DIV			JAS_FIX_DIV_UFLOW
#endif

/* Calculate the quotient of two fixed-point numbers without underflow
  checking. */
#define	JAS_FIX_DIV_FAST(fix_t, fracbits, bigfix_t, x, y) \
	JAS_CAST(fix_t, (JAS_CAST(bigfix_t, x) << (fracbits)) / (y))

/* Calculate the quotient of two fixed-point numbers with underflow
  checking. */
#define JAS_FIX_DIV_UFLOW(fix_t, fracbits, bigfix_t, x, y) \
	JAS_FIX_DIV_FAST(fix_t, fracbits, bigfix_t, x, y)

/* Negate a fixed-point number. */
#if !defined(DEBUG_OVERFLOW)
#define	JAS_FIX_NEG			JAS_FIX_NEG_FAST
#else
#define	JAS_FIX_NEG			JAS_FIX_NEG_OFLOW
#endif

/* Negate a fixed-point number without overflow checking. */
#define	JAS_FIX_NEG_FAST(fix_t, fracbits, x) \
	(-(x))

/* Negate a fixed-point number with overflow checking. */
/* Yes, overflow is actually possible for two's complement representations,
  although highly unlikely to occur. */
#define	JAS_FIX_NEG_OFLOW(fix_t, fracbits, x) \
	(((x) < 0) ? (-(x) > 0 || JAS_FIX_OFLOW(), -(x)) : (-(x)))

/* Perform an arithmetic shift left of a fixed-point number. */
#if !defined(DEBUG_OVERFLOW)
#define	JAS_FIX_ASL			JAS_FIX_ASL_FAST
#else
#define	JAS_FIX_ASL			JAS_FIX_ASL_OFLOW
#endif

/* Perform an arithmetic shift left of a fixed-point number without overflow
  checking. */
#define	JAS_FIX_ASL_FAST(fix_t, fracbits, x, n) \
	((x) << (n))

/* Perform an arithmetic shift left of a fixed-point number with overflow
  checking. */
#define	JAS_FIX_ASL_OFLOW(fix_t, fracbits, x, n) \
	((((x) << (n)) >> (n)) == (x) || JAS_FIX_OFLOW(), (x) << (n))

/* Perform an arithmetic shift right of a fixed-point number. */
#if !defined(DEBUG_OVERFLOW)
#define	JAS_FIX_ASR			JAS_FIX_ASR_FAST
#else
#define	JAS_FIX_ASR			JAS_FIX_ASR_UFLOW
#endif

/* Perform an arithmetic shift right of a fixed-point number without underflow
  checking. */
#define	JAS_FIX_ASR_FAST(fix_t, fracbits, x, n) \
	((x) >> (n))

/* Perform an arithmetic shift right of a fixed-point number with underflow
  checking. */
#define	JAS_FIX_ASR_UFLOW(fix_t, fracbits, x, n) \
	JAS_FIX_ASR_FAST(fix_t, fracbits, x, n)

/******************************************************************************\
* Other basic arithmetic operations.
\******************************************************************************/

/* Calculate the difference between two fixed-point numbers. */
#define JAS_FIX_SUB(fix_t, fracbits, x, y) \
	JAS_FIX_ADD(fix_t, fracbits, x, JAS_FIX_NEG(fix_t, fracbits, y))

/* Add one fixed-point number to another. */
#define JAS_FIX_PLUSEQ(fix_t, fracbits, x, y) \
	((x) = JAS_FIX_ADD(fix_t, fracbits, x, y))

/* Subtract one fixed-point number from another. */
#define JAS_FIX_MINUSEQ(fix_t, fracbits, x, y) \
	((x) = JAS_FIX_SUB(fix_t, fracbits, x, y))

/* Multiply one fixed-point number by another. */
#define	JAS_FIX_MULEQ(fix_t, fracbits, bigfix_t, x, y) \
	((x) = JAS_FIX_MUL(fix_t, fracbits, bigfix_t, x, y))

/******************************************************************************\
* Miscellaneous operations.
\******************************************************************************/

/* Calculate the absolute value of a fixed-point number. */
#define	JAS_FIX_ABS(fix_t, fracbits, x) \
	(((x) >= 0) ? (x) : (JAS_FIX_NEG(fix_t, fracbits, x)))

/* Is a fixed-point number an integer? */
#define	JAS_FIX_ISINT(fix_t, fracbits, x) \
	(JAS_FIX_FLOOR(fix_t, fracbits, x) == (x))

/* Get the sign of a fixed-point number. */
#define JAS_FIX_SGN(fix_t, fracbits, x) \
	((x) >= 0 ? 1 : (-1))

/******************************************************************************\
* Relational operations.
\******************************************************************************/

/* Compare two fixed-point numbers. */
#define JAS_FIX_CMP(fix_t, fracbits, x, y) \
	((x) > (y) ? 1 : (((x) == (y)) ? 0 : (-1)))

/* Less than. */
#define	JAS_FIX_LT(fix_t, fracbits, x, y) \
	((x) < (y))

/* Less than or equal. */
#define	JAS_FIX_LTE(fix_t, fracbits, x, y) \
	((x) <= (y))

/* Greater than. */
#define	JAS_FIX_GT(fix_t, fracbits, x, y) \
	((x) > (y))

/* Greater than or equal. */
#define	JAS_FIX_GTE(fix_t, fracbits, x, y) \
	((x) >= (y))

/******************************************************************************\
* Rounding functions.
\******************************************************************************/

/* Round a fixed-point number to the nearest integer. */
#define	JAS_FIX_ROUND(fix_t, fracbits, x) \
	(((x) < 0) ? JAS_FIX_FLOOR(fix_t, fracbits, JAS_FIX_ADD(fix_t, fracbits, \
	  (x), JAS_FIX_HALF(fix_t, fracbits))) : \
	  JAS_FIX_NEG(fix_t, fracbits, JAS_FIX_FLOOR(fix_t, fracbits, \
	  JAS_FIX_ADD(fix_t, fracbits, (-(x)), JAS_FIX_HALF(fix_t, fracbits)))))

/* Round a fixed-point number to the nearest integer in the direction of
  negative infinity (i.e., the floor function). */
#define	JAS_FIX_FLOOR(fix_t, fracbits, x) \
	((x) & (~((JAS_CAST(fix_t, 1) << (fracbits)) - 1)))

/* Round a fixed-point number to the nearest integer in the direction
  of zero. */
#define JAS_FIX_TRUNC(fix_t, fracbits, x) \
	(((x) >= 0) ? JAS_FIX_FLOOR(fix_t, fracbits, x) : \
	  JAS_FIX_CEIL(fix_t, fracbits, x))

/******************************************************************************\
* The below macros are for internal library use only.  Do not invoke them
* directly in application code.
\******************************************************************************/

/* Handle overflow. */
#define	JAS_FIX_OFLOW() \
	fprintf(stderr, "overflow error: file %s, line %d\n", __FILE__, __LINE__)

/* Handle underflow. */
#define	JAS_FIX_UFLOW() \
	fprintf(stderr, "underflow error: file %s, line %d\n", __FILE__, __LINE__)

#ifdef __cplusplus
}
#endif

#endif

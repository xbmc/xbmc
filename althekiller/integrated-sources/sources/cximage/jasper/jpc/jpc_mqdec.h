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
 * MQ Arithmetic Decoder
 *
 * $Id$
 */

#ifndef JPC_MQDEC_H
#define JPC_MQDEC_H

/******************************************************************************\
* Includes.
\******************************************************************************/

#include "jasper/jas_types.h"
#include "jasper/jas_stream.h"

#include "jpc_mqcod.h"

/******************************************************************************\
* Types.
\******************************************************************************/

/* MQ arithmetic decoder. */

typedef struct {

	/* The C register. */
	uint_fast32_t creg;

	/* The A register. */
	uint_fast32_t areg;

	/* The CT register. */
	uint_fast32_t ctreg;

	/* The current context. */
	jpc_mqstate_t **curctx;

	/* The per-context information. */
	jpc_mqstate_t **ctxs;

	/* The maximum number of contexts. */
	int maxctxs;

	/* The stream from which to read data. */
	jas_stream_t *in;

	/* The last character read. */
	uchar inbuffer;

	/* The EOF indicator. */
	int eof;

} jpc_mqdec_t;

/******************************************************************************\
* Functions/macros for construction and destruction.
\******************************************************************************/

/* Create a MQ decoder. */
jpc_mqdec_t *jpc_mqdec_create(int maxctxs, jas_stream_t *in);

/* Destroy a MQ decoder. */
void jpc_mqdec_destroy(jpc_mqdec_t *dec);

/******************************************************************************\
* Functions/macros for initialization.
\******************************************************************************/

/* Set the input stream associated with a MQ decoder. */
void jpc_mqdec_setinput(jpc_mqdec_t *dec, jas_stream_t *in);

/* Initialize a MQ decoder. */
void jpc_mqdec_init(jpc_mqdec_t *dec);

/******************************************************************************\
* Functions/macros for manipulating contexts.
\******************************************************************************/

/* Set the current context for a MQ decoder. */
#define	jpc_mqdec_setcurctx(dec, ctxno) \
	((mqdec)->curctx = &(mqdec)->ctxs[ctxno]);

/* Set the state information for a particular context of a MQ decoder. */
void jpc_mqdec_setctx(jpc_mqdec_t *dec, int ctxno, jpc_mqctx_t *ctx);

/* Set the state information for all contexts of a MQ decoder. */
void jpc_mqdec_setctxs(jpc_mqdec_t *dec, int numctxs, jpc_mqctx_t *ctxs);

/******************************************************************************\
* Functions/macros for decoding bits.
\******************************************************************************/

/* Decode a symbol. */
#if !defined(DEBUG)
#define	jpc_mqdec_getbit(dec) \
	jpc_mqdec_getbit_macro(dec)
#else
#define	jpc_mqdec_getbit(dec) \
	jpc_mqdec_getbit_func(dec)
#endif

/* Decode a symbol (assuming an unskewed probability distribution). */
#if !defined(DEBUG)
#define	jpc_mqdec_getbitnoskew(dec) \
	jpc_mqdec_getbit_macro(dec)
#else
#define	jpc_mqdec_getbitnoskew(dec) \
	jpc_mqdec_getbit_func(dec)
#endif

/******************************************************************************\
* Functions/macros for debugging.
\******************************************************************************/

/* Dump the MQ decoder state for debugging. */
void jpc_mqdec_dump(jpc_mqdec_t *dec, FILE *out);

/******************************************************************************\
* EVERYTHING BELOW THIS POINT IS IMPLEMENTATION SPECIFIC AND NOT PART OF THE
* APPLICATION INTERFACE.  DO NOT RELY ON ANY OF THE INTERNAL FUNCTIONS/MACROS
* GIVEN BELOW.
\******************************************************************************/

#define	jpc_mqdec_getbit_macro(dec) \
	((((dec)->areg -= (*(dec)->curctx)->qeval), \
	  (dec)->creg >> 16 >= (*(dec)->curctx)->qeval) ? \
	  ((((dec)->creg -= (*(dec)->curctx)->qeval << 16), \
	  (dec)->areg & 0x8000) ?  (*(dec)->curctx)->mps : \
	  jpc_mqdec_mpsexchrenormd(dec)) : \
	  jpc_mqdec_lpsexchrenormd(dec))

#define	jpc_mqdec_mpsexchange(areg, delta, curctx, bit) \
{ \
	if ((areg) < (delta)) { \
		register jpc_mqstate_t *state = *(curctx); \
		/* LPS decoded. */ \
		(bit) = state->mps ^ 1; \
		*(curctx) = state->nlps; \
	} else { \
		register jpc_mqstate_t *state = *(curctx); \
		/* MPS decoded. */ \
		(bit) = state->mps; \
		*(curctx) = state->nmps; \
	} \
}

#define	jpc_mqdec_lpsexchange(areg, delta, curctx, bit) \
{ \
	if ((areg) >= (delta)) { \
		register jpc_mqstate_t *state = *(curctx); \
		(areg) = (delta); \
		(bit) = state->mps ^ 1; \
		*(curctx) = state->nlps; \
	} else { \
		register jpc_mqstate_t *state = *(curctx); \
		(areg) = (delta); \
		(bit) = state->mps; \
		*(curctx) = state->nmps; \
	} \
}

#define	jpc_mqdec_renormd(areg, creg, ctreg, in, eof, inbuf) \
{ \
	do { \
		if (!(ctreg)) { \
			jpc_mqdec_bytein2(creg, ctreg, in, eof, inbuf); \
		} \
		(areg) <<= 1; \
		(creg) <<= 1; \
		--(ctreg); \
	} while (!((areg) & 0x8000)); \
}

#define	jpc_mqdec_bytein2(creg, ctreg, in, eof, inbuf) \
{ \
	int c; \
	unsigned char prevbuf; \
	if (!(eof)) { \
		if ((c = jas_stream_getc(in)) == EOF) { \
			(eof) = 1; \
			c = 0xff; \
		} \
		prevbuf = (inbuf); \
		(inbuf) = c; \
		if (prevbuf == 0xff) { \
			if (c > 0x8f) { \
				(creg) += 0xff00; \
				(ctreg) = 8; \
			} else { \
				(creg) += c << 9; \
				(ctreg) = 7; \
			} \
		} else { \
			(creg) += c << 8; \
			(ctreg) = 8; \
		} \
	} else { \
		(creg) += 0xff00; \
		(ctreg) = 8; \
	} \
}

int jpc_mqdec_getbit_func(jpc_mqdec_t *dec);
int jpc_mqdec_mpsexchrenormd(jpc_mqdec_t *dec);
int jpc_mqdec_lpsexchrenormd(jpc_mqdec_t *dec);

#endif

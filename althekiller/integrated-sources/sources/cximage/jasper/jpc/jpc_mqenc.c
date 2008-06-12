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
 * MQ Arithmetic Encoder
 *
 * $Id$
 */

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <assert.h>
#include <stdlib.h>

#include "jasper/jas_stream.h"
#include "jasper/jas_malloc.h"
#include "jasper/jas_math.h"
#include "jasper/jas_debug.h"

#include "jpc_mqenc.h"

/******************************************************************************\
* Macros
\******************************************************************************/

#if defined(DEBUG)
#define	JPC_MQENC_CALL(n, x) \
	((jas_getdbglevel() >= (n)) ? ((void)(x)) : ((void)0))
#else
#define	JPC_MQENC_CALL(n, x)
#endif

#define	jpc_mqenc_codemps9(areg, creg, ctreg, curctx, enc) \
{ \
	jpc_mqstate_t *state = *(curctx); \
	(areg) -= state->qeval; \
	if (!((areg) & 0x8000)) { \
		if ((areg) < state->qeval) { \
			(areg) = state->qeval; \
		} else { \
			(creg) += state->qeval; \
		} \
		*(curctx) = state->nmps; \
		jpc_mqenc_renorme((areg), (creg), (ctreg), (enc)); \
	} else { \
		(creg) += state->qeval; \
	} \
}

#define	jpc_mqenc_codelps2(areg, creg, ctreg, curctx, enc) \
{ \
	jpc_mqstate_t *state = *(curctx); \
	(areg) -= state->qeval; \
	if ((areg) < state->qeval) { \
		(creg) += state->qeval; \
	} else { \
		(areg) = state->qeval; \
	} \
	*(curctx) = state->nlps; \
	jpc_mqenc_renorme((areg), (creg), (ctreg), (enc)); \
}

#define	jpc_mqenc_renorme(areg, creg, ctreg, enc) \
{ \
	do { \
		(areg) <<= 1; \
		(creg) <<= 1; \
		if (!--(ctreg)) { \
			jpc_mqenc_byteout((areg), (creg), (ctreg), (enc)); \
		} \
	} while (!((areg) & 0x8000)); \
}

#define	jpc_mqenc_byteout(areg, creg, ctreg, enc) \
{ \
	if ((enc)->outbuf != 0xff) { \
		if ((creg) & 0x8000000) { \
			if (++((enc)->outbuf) == 0xff) { \
				(creg) &= 0x7ffffff; \
				jpc_mqenc_byteout2(enc); \
				enc->outbuf = ((creg) >> 20) & 0xff; \
				(creg) &= 0xfffff; \
				(ctreg) = 7; \
			} else { \
				jpc_mqenc_byteout2(enc); \
				enc->outbuf = ((creg) >> 19) & 0xff; \
				(creg) &= 0x7ffff; \
				(ctreg) = 8; \
			} \
		} else { \
			jpc_mqenc_byteout2(enc); \
			(enc)->outbuf = ((creg) >> 19) & 0xff; \
			(creg) &= 0x7ffff; \
			(ctreg) = 8; \
		} \
	} else { \
		jpc_mqenc_byteout2(enc); \
		(enc)->outbuf = ((creg) >> 20) & 0xff; \
		(creg) &= 0xfffff; \
		(ctreg) = 7; \
	} \
}

#define	jpc_mqenc_byteout2(enc) \
{ \
	if (enc->outbuf >= 0) { \
		if (jas_stream_putc(enc->out, (unsigned char)enc->outbuf) == EOF) { \
			enc->err |= 1; \
		} \
	} \
	enc->lastbyte = enc->outbuf; \
}

/******************************************************************************\
* Local function protoypes.
\******************************************************************************/

static void jpc_mqenc_setbits(jpc_mqenc_t *mqenc);

/******************************************************************************\
* Code for creation and destruction of encoder.
\******************************************************************************/

/* Create a MQ encoder. */

jpc_mqenc_t *jpc_mqenc_create(int maxctxs, jas_stream_t *out)
{
	jpc_mqenc_t *mqenc;

	/* Allocate memory for the MQ encoder. */
	if (!(mqenc = jas_malloc(sizeof(jpc_mqenc_t)))) {
		goto error;
	}
	mqenc->out = out;
	mqenc->maxctxs = maxctxs;

	/* Allocate memory for the per-context state information. */
	if (!(mqenc->ctxs = jas_malloc(mqenc->maxctxs * sizeof(jpc_mqstate_t *)))) {
		goto error;
	}

	/* Set the current context to the first one. */
	mqenc->curctx = mqenc->ctxs;

	jpc_mqenc_init(mqenc);

	/* Initialize the per-context state information to something sane. */
	jpc_mqenc_setctxs(mqenc, 0, 0);

	return mqenc;

error:
	if (mqenc) {
		jpc_mqenc_destroy(mqenc);
	}
	return 0;
}

/* Destroy a MQ encoder. */

void jpc_mqenc_destroy(jpc_mqenc_t *mqenc)
{
	if (mqenc->ctxs) {
		jas_free(mqenc->ctxs);
	}
	jas_free(mqenc);
}

/******************************************************************************\
* State initialization code.
\******************************************************************************/

/* Initialize the coding state of a MQ encoder. */

void jpc_mqenc_init(jpc_mqenc_t *mqenc)
{
	mqenc->areg = 0x8000;
	mqenc->outbuf = -1;
	mqenc->creg = 0;
	mqenc->ctreg = 12;
	mqenc->lastbyte = -1;
	mqenc->err = 0;
}

/* Initialize one or more contexts. */

void jpc_mqenc_setctxs(jpc_mqenc_t *mqenc, int numctxs, jpc_mqctx_t *ctxs)
{
	jpc_mqstate_t **ctx;
	int n;

	ctx = mqenc->ctxs;
	n = JAS_MIN(mqenc->maxctxs, numctxs);
	while (--n >= 0) {
		*ctx = &jpc_mqstates[2 * ctxs->ind + ctxs->mps];
		++ctx;
		++ctxs;
	}
	n = mqenc->maxctxs - numctxs;
	while (--n >= 0) {
		*ctx = &jpc_mqstates[0];
		++ctx;
	}

}

/* Get the coding state for a MQ encoder. */

void jpc_mqenc_getstate(jpc_mqenc_t *mqenc, jpc_mqencstate_t *state)
{
	state->areg = mqenc->areg;
	state->creg = mqenc->creg;
	state->ctreg = mqenc->ctreg;
	state->lastbyte = mqenc->lastbyte;
}

/******************************************************************************\
* Code for coding symbols.
\******************************************************************************/

/* Encode a bit. */

int jpc_mqenc_putbit_func(jpc_mqenc_t *mqenc, int bit)
{
	const jpc_mqstate_t *state;
	JAS_DBGLOG(100, ("jpc_mqenc_putbit(%p, %d)\n", mqenc, bit));
	JPC_MQENC_CALL(100, jpc_mqenc_dump(mqenc, stderr));

	state = *(mqenc->curctx);

	if (state->mps == bit) {
		/* Apply the CODEMPS algorithm as defined in the standard. */
		mqenc->areg -= state->qeval;
		if (!(mqenc->areg & 0x8000)) {
			jpc_mqenc_codemps2(mqenc);
		} else {
			mqenc->creg += state->qeval;
		}
	} else {
		/* Apply the CODELPS algorithm as defined in the standard. */
		jpc_mqenc_codelps2(mqenc->areg, mqenc->creg, mqenc->ctreg, mqenc->curctx, mqenc);
	}

	return jpc_mqenc_error(mqenc) ? (-1) : 0;
}

int jpc_mqenc_codemps2(jpc_mqenc_t *mqenc)
{
	/* Note: This function only performs part of the work associated with
	the CODEMPS algorithm from the standard.  Some of the work is also
	performed by the caller. */

	jpc_mqstate_t *state = *(mqenc->curctx);
	if (mqenc->areg < state->qeval) {
		mqenc->areg = state->qeval;
	} else {
		mqenc->creg += state->qeval;
	}
	*mqenc->curctx = state->nmps;
	jpc_mqenc_renorme(mqenc->areg, mqenc->creg, mqenc->ctreg, mqenc);
	return jpc_mqenc_error(mqenc) ? (-1) : 0;
}

int jpc_mqenc_codelps(jpc_mqenc_t *mqenc)
{
	jpc_mqenc_codelps2(mqenc->areg, mqenc->creg, mqenc->ctreg, mqenc->curctx, mqenc);
	return jpc_mqenc_error(mqenc) ? (-1) : 0;
}

/******************************************************************************\
* Miscellaneous code.
\******************************************************************************/

/* Terminate the code word. */

int jpc_mqenc_flush(jpc_mqenc_t *mqenc, int termmode)
{
	int_fast16_t k;

	switch (termmode) {
	case JPC_MQENC_PTERM:
		k = 11 - mqenc->ctreg + 1;
		while (k > 0) {
			mqenc->creg <<= mqenc->ctreg;
			mqenc->ctreg = 0;
			jpc_mqenc_byteout(mqenc->areg, mqenc->creg, mqenc->ctreg,
			  mqenc);
			k -= mqenc->ctreg;
		}
		if (mqenc->outbuf != 0xff) {
			jpc_mqenc_byteout(mqenc->areg, mqenc->creg, mqenc->ctreg, mqenc);
		}
		break;
	case JPC_MQENC_DEFTERM:
		jpc_mqenc_setbits(mqenc);
		mqenc->creg <<= mqenc->ctreg;
		jpc_mqenc_byteout(mqenc->areg, mqenc->creg, mqenc->ctreg, mqenc);
		mqenc->creg <<= mqenc->ctreg;
		jpc_mqenc_byteout(mqenc->areg, mqenc->creg, mqenc->ctreg, mqenc);
		if (mqenc->outbuf != 0xff) {
			jpc_mqenc_byteout(mqenc->areg, mqenc->creg, mqenc->ctreg, mqenc);
		}
		break;
	default:
		abort();
		break;
	}
	return 0;
}

static void jpc_mqenc_setbits(jpc_mqenc_t *mqenc)
{
	uint_fast32_t tmp = mqenc->creg + mqenc->areg;
	mqenc->creg |= 0xffff;
	if (mqenc->creg >= tmp) {
		mqenc->creg -= 0x8000;
	}
}

/* Dump a MQ encoder to a stream for debugging. */

int jpc_mqenc_dump(jpc_mqenc_t *mqenc, FILE *out)
{
	fprintf(out, "AREG = %08x, CREG = %08x, CTREG = %d\n",
	  mqenc->areg, mqenc->creg, mqenc->ctreg);
	fprintf(out, "IND = %02d, MPS = %d, QEVAL = %04x\n",
	  *mqenc->curctx - jpc_mqstates, (*mqenc->curctx)->mps,
	  (*mqenc->curctx)->qeval);
	return 0;
}

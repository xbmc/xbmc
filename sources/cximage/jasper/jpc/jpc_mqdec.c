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

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>

#include "jasper/jas_types.h"
#include "jasper/jas_malloc.h"
#include "jasper/jas_math.h"
#include "jasper/jas_debug.h"

#include "jpc_mqdec.h"

/******************************************************************************\
* Local macros.
\******************************************************************************/

#if defined(DEBUG)
#define	MQDEC_CALL(n, x) \
	((jas_getdbglevel() >= (n)) ? ((void)(x)) : ((void)0))
#else
#define	MQDEC_CALL(n, x)
#endif

/******************************************************************************\
* Local function prototypes.
\******************************************************************************/

static void jpc_mqdec_bytein(jpc_mqdec_t *mqdec);

/******************************************************************************\
* Code for creation and destruction of a MQ decoder.
\******************************************************************************/

/* Create a MQ decoder. */
jpc_mqdec_t *jpc_mqdec_create(int maxctxs, jas_stream_t *in)
{
	jpc_mqdec_t *mqdec;

	/* There must be at least one context. */
	assert(maxctxs > 0);

	/* Allocate memory for the MQ decoder. */
	if (!(mqdec = jas_malloc(sizeof(jpc_mqdec_t)))) {
		goto error;
	}
	mqdec->in = in;
	mqdec->maxctxs = maxctxs;
	/* Allocate memory for the per-context state information. */
	if (!(mqdec->ctxs = jas_malloc(mqdec->maxctxs * sizeof(jpc_mqstate_t *)))) {
		goto error;
	}
	/* Set the current context to the first context. */
	mqdec->curctx = mqdec->ctxs;

	/* If an input stream has been associated with the MQ decoder,
	  initialize the decoder state from the stream. */
	if (mqdec->in) {
		jpc_mqdec_init(mqdec);
	}
	/* Initialize the per-context state information. */
	jpc_mqdec_setctxs(mqdec, 0, 0);

	return mqdec;

error:
	/* Oops...  Something has gone wrong. */
	if (mqdec) {
		jpc_mqdec_destroy(mqdec);
	}
	return 0;
}

/* Destroy a MQ decoder. */
void jpc_mqdec_destroy(jpc_mqdec_t *mqdec)
{
	if (mqdec->ctxs) {
		jas_free(mqdec->ctxs);
	}
	jas_free(mqdec);
}

/******************************************************************************\
* Code for initialization of a MQ decoder.
\******************************************************************************/

/* Initialize the state of a MQ decoder. */

void jpc_mqdec_init(jpc_mqdec_t *mqdec)
{
	int c;

	mqdec->eof = 0;
	mqdec->creg = 0;
	/* Get the next byte from the input stream. */
	if ((c = jas_stream_getc(mqdec->in)) == EOF) {
		/* We have encountered an I/O error or EOF. */
		c = 0xff;
		mqdec->eof = 1;
	}
	mqdec->inbuffer = c;
	mqdec->creg += mqdec->inbuffer << 16;
	jpc_mqdec_bytein(mqdec);
	mqdec->creg <<= 7;
	mqdec->ctreg -= 7;
	mqdec->areg = 0x8000;
}

/* Set the input stream for a MQ decoder. */

void jpc_mqdec_setinput(jpc_mqdec_t *mqdec, jas_stream_t *in)
{
	mqdec->in = in;
}

/* Initialize one or more contexts. */

void jpc_mqdec_setctxs(jpc_mqdec_t *mqdec, int numctxs, jpc_mqctx_t *ctxs)
{
	jpc_mqstate_t **ctx;
	int n;

	ctx = mqdec->ctxs;
	n = JAS_MIN(mqdec->maxctxs, numctxs);
	while (--n >= 0) {
		*ctx = &jpc_mqstates[2 * ctxs->ind + ctxs->mps];
		++ctx;
		++ctxs;
	}
	n = mqdec->maxctxs - numctxs;
	while (--n >= 0) {
		*ctx = &jpc_mqstates[0];
		++ctx;
	}
}

/* Initialize a context. */

void jpc_mqdec_setctx(jpc_mqdec_t *mqdec, int ctxno, jpc_mqctx_t *ctx)
{
	jpc_mqstate_t **ctxi;
	ctxi = &mqdec->ctxs[ctxno];
	*ctxi = &jpc_mqstates[2 * ctx->ind + ctx->mps];
}

/******************************************************************************\
* Code for decoding a bit.
\******************************************************************************/

/* Decode a bit. */

int jpc_mqdec_getbit_func(register jpc_mqdec_t *mqdec)
{
	int bit;
	JAS_DBGLOG(100, ("jpc_mqdec_getbit_func(%p)\n", mqdec));
	MQDEC_CALL(100, jpc_mqdec_dump(mqdec, stderr));
	bit = jpc_mqdec_getbit_macro(mqdec);
	MQDEC_CALL(100, jpc_mqdec_dump(mqdec, stderr));
	JAS_DBGLOG(100, ("ctx = %d, decoded %d\n", mqdec->curctx -
	  mqdec->ctxs, bit));
	return bit;
}

/* Apply MPS_EXCHANGE algorithm (with RENORMD). */
int jpc_mqdec_mpsexchrenormd(register jpc_mqdec_t *mqdec)
{
	int ret;
	register jpc_mqstate_t *state = *mqdec->curctx;
	jpc_mqdec_mpsexchange(mqdec->areg, state->qeval, mqdec->curctx, ret);
	jpc_mqdec_renormd(mqdec->areg, mqdec->creg, mqdec->ctreg, mqdec->in,
	  mqdec->eof, mqdec->inbuffer);
	return ret;
}

/* Apply LPS_EXCHANGE algorithm (with RENORMD). */
int jpc_mqdec_lpsexchrenormd(register jpc_mqdec_t *mqdec)
{
	int ret;
	register jpc_mqstate_t *state = *mqdec->curctx;
	jpc_mqdec_lpsexchange(mqdec->areg, state->qeval, mqdec->curctx, ret);
	jpc_mqdec_renormd(mqdec->areg, mqdec->creg, mqdec->ctreg, mqdec->in,
	  mqdec->eof, mqdec->inbuffer);
	return ret;
}

/******************************************************************************\
* Support code.
\******************************************************************************/

/* Apply the BYTEIN algorithm. */
static void jpc_mqdec_bytein(jpc_mqdec_t *mqdec)
{
	int c;
	unsigned char prevbuf;

	if (!mqdec->eof) {
		if ((c = jas_stream_getc(mqdec->in)) == EOF) {
			mqdec->eof = 1;
			c = 0xff;
		}
		prevbuf = mqdec->inbuffer;
		mqdec->inbuffer = c;
		if (prevbuf == 0xff) {
			if (c > 0x8f) {
				mqdec->creg += 0xff00;
				mqdec->ctreg = 8;
			} else {
				mqdec->creg += c << 9;
				mqdec->ctreg = 7;
			}
		} else {
			mqdec->creg += c << 8;
			mqdec->ctreg = 8;
		}
	} else {
		mqdec->creg += 0xff00;
		mqdec->ctreg = 8;
	}
}

/******************************************************************************\
* Code for debugging.
\******************************************************************************/

/* Dump a MQ decoder to a stream for debugging. */

void jpc_mqdec_dump(jpc_mqdec_t *mqdec, FILE *out)
{
	fprintf(out, "MQDEC A = %08lx, C = %08lx, CT=%08lx, ",
	  (unsigned long) mqdec->areg, (unsigned long) mqdec->creg,
	  (unsigned long) mqdec->ctreg);
	fprintf(out, "CTX = %d, ", mqdec->curctx - mqdec->ctxs);
	fprintf(out, "IND %d, MPS %d, QEVAL %x\n", *mqdec->curctx -
	  jpc_mqstates, (*mqdec->curctx)->mps, (*mqdec->curctx)->qeval);
}

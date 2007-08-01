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
 * Quadrature Mirror-Image Filter Bank (QMFB) Library
 *
 * $Id$
 */

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <assert.h>

#include "jasper/jas_fix.h"
#include "jasper/jas_malloc.h"
#include "jasper/jas_math.h"

#include "jpc_qmfb.h"
#include "jpc_tsfb.h"
#include "jpc_math.h"

/******************************************************************************\
*
\******************************************************************************/

static jpc_qmfb1d_t *jpc_qmfb1d_create(void);

static int jpc_ft_getnumchans(jpc_qmfb1d_t *qmfb);
static int jpc_ft_getanalfilters(jpc_qmfb1d_t *qmfb, int len, jas_seq2d_t **filters);
static int jpc_ft_getsynfilters(jpc_qmfb1d_t *qmfb, int len, jas_seq2d_t **filters);
static void jpc_ft_analyze(jpc_qmfb1d_t *qmfb, int flags, jas_seq2d_t *x);
static void jpc_ft_synthesize(jpc_qmfb1d_t *qmfb, int flags, jas_seq2d_t *x);

static int jpc_ns_getnumchans(jpc_qmfb1d_t *qmfb);
static int jpc_ns_getanalfilters(jpc_qmfb1d_t *qmfb, int len, jas_seq2d_t **filters);
static int jpc_ns_getsynfilters(jpc_qmfb1d_t *qmfb, int len, jas_seq2d_t **filters);
static void jpc_ns_analyze(jpc_qmfb1d_t *qmfb, int flags, jas_seq2d_t *x);
static void jpc_ns_synthesize(jpc_qmfb1d_t *qmfb, int flags, jas_seq2d_t *x);

/******************************************************************************\
*
\******************************************************************************/

jpc_qmfb1dops_t jpc_ft_ops = {
	jpc_ft_getnumchans,
	jpc_ft_getanalfilters,
	jpc_ft_getsynfilters,
	jpc_ft_analyze,
	jpc_ft_synthesize
};

jpc_qmfb1dops_t jpc_ns_ops = {
	jpc_ns_getnumchans,
	jpc_ns_getanalfilters,
	jpc_ns_getsynfilters,
	jpc_ns_analyze,
	jpc_ns_synthesize
};

/******************************************************************************\
*
\******************************************************************************/

static void jpc_qmfb1d_setup(jpc_fix_t *startptr, int startind, int endind,
  int intrastep, jpc_fix_t **lstartptr, int *lstartind, int *lendind,
  jpc_fix_t **hstartptr, int *hstartind, int *hendind)
{
	*lstartind = JPC_CEILDIVPOW2(startind, 1);
	*lendind = JPC_CEILDIVPOW2(endind, 1);
	*hstartind = JPC_FLOORDIVPOW2(startind, 1);
	*hendind = JPC_FLOORDIVPOW2(endind, 1);
	*lstartptr = startptr;
	*hstartptr = &startptr[(*lendind - *lstartind) * intrastep];
}

static void jpc_qmfb1d_split(jpc_fix_t *startptr, int startind, int endind,
  register int step, jpc_fix_t *lstartptr, int lstartind, int lendind,
  jpc_fix_t *hstartptr, int hstartind, int hendind)
{
	int bufsize = JPC_CEILDIVPOW2(endind - startind, 2);
#if defined(WIN32)
#define QMFB_SPLITBUFSIZE 4096
	jpc_fix_t splitbuf[QMFB_SPLITBUFSIZE];
#else
	jpc_fix_t splitbuf[bufsize];
#endif
	jpc_fix_t *buf = splitbuf;
	int llen;
	int hlen;
	int twostep;
	jpc_fix_t *tmpptr;
	register jpc_fix_t *ptr;
	register jpc_fix_t *hptr;
	register jpc_fix_t *lptr;
	register int n;
	int state;

	twostep = step << 1;
	llen = lendind - lstartind;
	hlen = hendind - hstartind;

#if defined(WIN32)
	/* Get a buffer. */
	if (bufsize > QMFB_SPLITBUFSIZE) {
		if (!(buf = jas_malloc(bufsize * sizeof(jpc_fix_t)))) {
			/* We have no choice but to commit suicide in this case. */
			abort();
		}
	}
#endif

	if (hstartind < lstartind) {
		/* The first sample in the input signal is to appear
		  in the highpass subband signal. */
		/* Copy the appropriate samples into the lowpass subband
		  signal, saving any samples destined for the highpass subband
		  signal as they are overwritten. */
		tmpptr = buf;
		ptr = &startptr[step];
		lptr = lstartptr;
		n = llen;
		state = 1;
		while (n-- > 0) {
			if (state) {
				*tmpptr = *lptr;
				++tmpptr;
			}
			*lptr = *ptr;
			ptr += twostep;
			lptr += step;
			state ^= 1;
		}
		/* Copy the appropriate samples into the highpass subband
		  signal. */
		/* Handle the nonoverwritten samples. */
		hptr = &hstartptr[(hlen - 1) * step];
		ptr = &startptr[(((llen + hlen - 1) >> 1) << 1) * step];
		n = hlen - (tmpptr - buf);
		while (n-- > 0) {
			*hptr = *ptr;
			hptr -= step;
			ptr -= twostep;
		}
		/* Handle the overwritten samples. */
		n = tmpptr - buf;
		while (n-- > 0) {
			--tmpptr;
			*hptr = *tmpptr;
			hptr -= step;
		}
	} else {
		/* The first sample in the input signal is to appear
		  in the lowpass subband signal. */
		/* Copy the appropriate samples into the lowpass subband
		  signal, saving any samples for the highpass subband
		  signal as they are overwritten. */
		state = 0;
		ptr = startptr;
		lptr = lstartptr;
		tmpptr = buf;
		n = llen;
		while (n-- > 0) {
			if (state) {
				*tmpptr = *lptr;
				++tmpptr;
			}
			*lptr = *ptr;
			ptr += twostep;
			lptr += step;
			state ^= 1;
		}
		/* Copy the appropriate samples into the highpass subband
		  signal. */
		/* Handle the nonoverwritten samples. */
		ptr = &startptr[((((llen + hlen) >> 1) << 1) - 1) * step];
		hptr = &hstartptr[(hlen - 1) * step];
		n = hlen - (tmpptr - buf);
		while (n-- > 0) {
			*hptr = *ptr;
			ptr -= twostep;
			hptr -= step;
		}
		/* Handle the overwritten samples. */
		n = tmpptr - buf;
		while (n-- > 0) {
			--tmpptr;
			*hptr = *tmpptr;
			hptr -= step;
		}
	}

#if defined(WIN32)
	/* If the split buffer was allocated on the heap, free this memory. */
	if (buf != splitbuf) {
		jas_free(buf);
	}
#endif
}

static void jpc_qmfb1d_join(jpc_fix_t *startptr, int startind, int endind,
  register int step, jpc_fix_t *lstartptr, int lstartind, int lendind,
  jpc_fix_t *hstartptr, int hstartind, int hendind)
{
	int bufsize = JPC_CEILDIVPOW2(endind - startind, 2);
#if defined(WIN32)
#define	QMFB_JOINBUFSIZE	4096
	jpc_fix_t joinbuf[QMFB_JOINBUFSIZE];
#else
	jpc_fix_t joinbuf[bufsize];
#endif
	jpc_fix_t *buf = joinbuf;
	int llen;
	int hlen;
	int twostep;
	jpc_fix_t *tmpptr;
	register jpc_fix_t *ptr;
	register jpc_fix_t *hptr;
	register jpc_fix_t *lptr;
	register int n;
	int state;

#if defined(WIN32)
	/* Allocate memory for the join buffer from the heap. */
	if (bufsize > QMFB_JOINBUFSIZE) {
		if (!(buf = jas_malloc(bufsize * sizeof(jpc_fix_t)))) {
			/* We have no choice but to commit suicide. */
			abort();
		}
	}
#endif

	twostep = step << 1;
	llen = lendind - lstartind;
	hlen = hendind - hstartind;

	if (hstartind < lstartind) {
		/* The first sample in the highpass subband signal is to
		  appear first in the output signal. */
		/* Copy the appropriate samples into the first phase of the
		  output signal. */
		tmpptr = buf;
		hptr = hstartptr;
		ptr = startptr;
		n = (llen + 1) >> 1;
		while (n-- > 0) {
			*tmpptr = *ptr;
			*ptr = *hptr;
			++tmpptr;
			ptr += twostep;
			hptr += step;
		}
		n = hlen - ((llen + 1) >> 1);
		while (n-- > 0) {
			*ptr = *hptr;
			ptr += twostep;
			hptr += step;
		}
		/* Copy the appropriate samples into the second phase of
		  the output signal. */
		ptr -= (lendind > hendind) ? (step) : (step + twostep);
		state = !((llen - 1) & 1);
		lptr = &lstartptr[(llen - 1) * step];
		n = llen;
		while (n-- > 0) {
			if (state) {
				--tmpptr;
				*ptr = *tmpptr;
			} else {
				*ptr = *lptr;
			}
			lptr -= step;
			ptr -= twostep;
			state ^= 1;
		}
	} else {
		/* The first sample in the lowpass subband signal is to
		  appear first in the output signal. */
		/* Copy the appropriate samples into the first phase of the
		  output signal (corresponding to even indexed samples). */
		lptr = &lstartptr[(llen - 1) * step];
		ptr = &startptr[((llen - 1) << 1) * step];
		n = llen >> 1;
		tmpptr = buf;
		while (n-- > 0) {
			*tmpptr = *ptr;
			*ptr = *lptr;
			++tmpptr;
			ptr -= twostep;
			lptr -= step;
		}
		n = llen - (llen >> 1);
		while (n-- > 0) {
			*ptr = *lptr;
			ptr -= twostep;
			lptr -= step;
		}
		/* Copy the appropriate samples into the second phase of
		  the output signal (corresponding to odd indexed
		  samples). */
		ptr = &startptr[step];
		hptr = hstartptr;
		state = !(llen & 1);
		n = hlen;
		while (n-- > 0) {
			if (state) {
				--tmpptr;
				*ptr = *tmpptr;
			} else {
				*ptr = *hptr;
			}
			hptr += step;
			ptr += twostep;
			state ^= 1;
		}
	}

#if defined(WIN32)
	/* If the join buffer was allocated on the heap, free this memory. */
	if (buf != joinbuf) {
		jas_free(buf);
	}
#endif
}

/******************************************************************************\
* Code for 5/3 transform.
\******************************************************************************/

static int jpc_ft_getnumchans(jpc_qmfb1d_t *qmfb)
{
	/* Avoid compiler warnings about unused parameters. */
	qmfb = 0;

	return 2;
}

static int jpc_ft_getanalfilters(jpc_qmfb1d_t *qmfb, int len, jas_seq2d_t **filters)
{
	/* Avoid compiler warnings about unused parameters. */
	qmfb = 0;
	len = 0;
	filters = 0;
	abort();
	return -1;
}

static int jpc_ft_getsynfilters(jpc_qmfb1d_t *qmfb, int len, jas_seq2d_t **filters)
{
	jas_seq_t *lf;
	jas_seq_t *hf;

	/* Avoid compiler warnings about unused parameters. */
	qmfb = 0;

	lf = 0;
	hf = 0;

	if (len > 1 || (!len)) {
		if (!(lf = jas_seq_create(-1, 2))) {
			goto error;
		}
		jas_seq_set(lf, -1, jpc_dbltofix(0.5));
		jas_seq_set(lf, 0, jpc_dbltofix(1.0));
		jas_seq_set(lf, 1, jpc_dbltofix(0.5));
		if (!(hf = jas_seq_create(-1, 4))) {
			goto error;
		}
		jas_seq_set(hf, -1, jpc_dbltofix(-0.125));
		jas_seq_set(hf, 0, jpc_dbltofix(-0.25));
		jas_seq_set(hf, 1, jpc_dbltofix(0.75));
		jas_seq_set(hf, 2, jpc_dbltofix(-0.25));
		jas_seq_set(hf, 3, jpc_dbltofix(-0.125));
	} else if (len == 1) {
		if (!(lf = jas_seq_create(0, 1))) {
			goto error;
		}
		jas_seq_set(lf, 0, jpc_dbltofix(1.0));
		if (!(hf = jas_seq_create(0, 1))) {
			goto error;
		}
		jas_seq_set(hf, 0, jpc_dbltofix(2.0));
	} else {
		abort();
	}

	filters[0] = lf;
	filters[1] = hf;

	return 0;

error:
	if (lf) {
		jas_seq_destroy(lf);
	}
	if (hf) {
		jas_seq_destroy(hf);
	}
	return -1;
}

#define	NFT_LIFT0(lstartptr, lstartind, lendind, hstartptr, hstartind, hendind, step, pluseq) \
{ \
	register jpc_fix_t *lptr = (lstartptr); \
	register jpc_fix_t *hptr = (hstartptr); \
	register int n = (hendind) - (hstartind); \
	if ((hstartind) < (lstartind)) { \
		pluseq(*hptr, *lptr); \
		hptr += (step); \
		--n; \
	} \
	if ((hendind) >= (lendind)) { \
		--n; \
	} \
	while (n-- > 0) { \
		pluseq(*hptr, jpc_fix_asr(jpc_fix_add(*lptr, lptr[(step)]), 1)); \
		hptr += (step); \
		lptr += (step); \
	} \
	if ((hendind) >= (lendind)) { \
		pluseq(*hptr, *lptr); \
	} \
}

#define	NFT_LIFT1(lstartptr, lstartind, lendind, hstartptr, hstartind, hendind, step, pluseq) \
{ \
	register jpc_fix_t *lptr = (lstartptr); \
	register jpc_fix_t *hptr = (hstartptr); \
	register int n = (lendind) - (lstartind); \
	if ((hstartind) >= (lstartind)) { \
		pluseq(*lptr, *hptr); \
		lptr += (step); \
		--n; \
	} \
	if ((lendind) > (hendind)) { \
		--n; \
	} \
	while (n-- > 0) { \
		pluseq(*lptr, jpc_fix_asr(jpc_fix_add(*hptr, hptr[(step)]), 2)); \
		lptr += (step); \
		hptr += (step); \
	} \
	if ((lendind) > (hendind)) { \
		pluseq(*lptr, *hptr); \
	} \
}

#define	RFT_LIFT0(lstartptr, lstartind, lendind, hstartptr, hstartind, hendind, step, pmeqop) \
{ \
	register jpc_fix_t *lptr = (lstartptr); \
	register jpc_fix_t *hptr = (hstartptr); \
	register int n = (hendind) - (hstartind); \
	if ((hstartind) < (lstartind)) { \
		*hptr pmeqop *lptr; \
		hptr += (step); \
		--n; \
	} \
	if ((hendind) >= (lendind)) { \
		--n; \
	} \
	while (n-- > 0) { \
		*hptr pmeqop (*lptr + lptr[(step)]) >> 1; \
		hptr += (step); \
		lptr += (step); \
	} \
	if ((hendind) >= (lendind)) { \
		*hptr pmeqop *lptr; \
	} \
}

#define	RFT_LIFT1(lstartptr, lstartind, lendind, hstartptr, hstartind, hendind, step, pmeqop) \
{ \
	register jpc_fix_t *lptr = (lstartptr); \
	register jpc_fix_t *hptr = (hstartptr); \
	register int n = (lendind) - (lstartind); \
	if ((hstartind) >= (lstartind)) { \
		*lptr pmeqop ((*hptr << 1) + 2) >> 2; \
		lptr += (step); \
		--n; \
	} \
	if ((lendind) > (hendind)) { \
		--n; \
	} \
	while (n-- > 0) { \
		*lptr pmeqop ((*hptr + hptr[(step)]) + 2) >> 2; \
		lptr += (step); \
		hptr += (step); \
	} \
	if ((lendind) > (hendind)) { \
		*lptr pmeqop ((*hptr << 1) + 2) >> 2; \
	} \
}

static void jpc_ft_analyze(jpc_qmfb1d_t *qmfb, int flags, jas_seq2d_t *x)
{
	jpc_fix_t *startptr;
	int startind;
	int endind;
	jpc_fix_t *  lstartptr;
	int   lstartind;
	int   lendind;
	jpc_fix_t *  hstartptr;
	int   hstartind;
	int   hendind;
	int interstep;
	int intrastep;
	int numseq;

	/* Avoid compiler warnings about unused parameters. */
	qmfb = 0;

	if (flags & JPC_QMFB1D_VERT) {
		interstep = 1;
		intrastep = jas_seq2d_rowstep(x);
		numseq = jas_seq2d_width(x);
		startind = jas_seq2d_ystart(x);
		endind = jas_seq2d_yend(x);
	} else {
		interstep = jas_seq2d_rowstep(x);
		intrastep = 1;
		numseq = jas_seq2d_height(x);
		startind = jas_seq2d_xstart(x);
		endind = jas_seq2d_xend(x);
	}

	assert(startind < endind);

	startptr = jas_seq2d_getref(x, jas_seq2d_xstart(x), jas_seq2d_ystart(x));
	if (flags & JPC_QMFB1D_RITIMODE) {
		while (numseq-- > 0) {
			jpc_qmfb1d_setup(startptr, startind, endind, intrastep,
			  &lstartptr, &lstartind, &lendind, &hstartptr,
			  &hstartind, &hendind);
			if (endind - startind > 1) {
				jpc_qmfb1d_split(startptr, startind, endind,
				  intrastep, lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind);
				RFT_LIFT0(lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind, intrastep, -=);
				RFT_LIFT1(lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind, intrastep, +=);
			} else {
				if (lstartind == lendind) {
					*startptr <<= 1;
				}
			}
			startptr += interstep;
		}
	} else {
		while (numseq-- > 0) {
			jpc_qmfb1d_setup(startptr, startind, endind, intrastep,
			  &lstartptr, &lstartind, &lendind, &hstartptr,
			  &hstartind, &hendind);
			if (endind - startind > 1) {
				jpc_qmfb1d_split(startptr, startind, endind,
				  intrastep, lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind);
				NFT_LIFT0(lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind, intrastep,
				  jpc_fix_minuseq);
				NFT_LIFT1(lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind, intrastep,
				  jpc_fix_pluseq);
			} else {
				if (lstartind == lendind) {
					*startptr = jpc_fix_asl(*startptr, 1);
				}
			}
			startptr += interstep;
		}
	}
}

static void jpc_ft_synthesize(jpc_qmfb1d_t *qmfb, int flags, jas_seq2d_t *x)
{
	jpc_fix_t *startptr;
	int startind;
	int endind;
	jpc_fix_t *lstartptr;
	int lstartind;
	int lendind;
	jpc_fix_t *hstartptr;
	int hstartind;
	int hendind;
	int interstep;
	int intrastep;
	int numseq;

	/* Avoid compiler warnings about unused parameters. */
	qmfb = 0;

	if (flags & JPC_QMFB1D_VERT) {
		interstep = 1;
		intrastep = jas_seq2d_rowstep(x);
		numseq = jas_seq2d_width(x);
		startind = jas_seq2d_ystart(x);
		endind = jas_seq2d_yend(x);
	} else {
		interstep = jas_seq2d_rowstep(x);
		intrastep = 1;
		numseq = jas_seq2d_height(x);
		startind = jas_seq2d_xstart(x);
		endind = jas_seq2d_xend(x);
	}

	assert(startind < endind);

	startptr = jas_seq2d_getref(x, jas_seq2d_xstart(x), jas_seq2d_ystart(x));
	if (flags & JPC_QMFB1D_RITIMODE) {
		while (numseq-- > 0) {
			jpc_qmfb1d_setup(startptr, startind, endind, intrastep,
			  &lstartptr, &lstartind, &lendind, &hstartptr,
			  &hstartind, &hendind);
			if (endind - startind > 1) {
				RFT_LIFT1(lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind, intrastep, -=);
				RFT_LIFT0(lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind, intrastep, +=);
				jpc_qmfb1d_join(startptr, startind, endind,
				  intrastep, lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind);
			} else {
				if (lstartind == lendind) {
					*startptr >>= 1;
				}
			}
			startptr += interstep;
		}
	} else {
		while (numseq-- > 0) {
			jpc_qmfb1d_setup(startptr, startind, endind, intrastep,
			  &lstartptr, &lstartind, &lendind, &hstartptr,
			  &hstartind, &hendind);
			if (endind - startind > 1) {
				NFT_LIFT1(lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind, intrastep,
				  jpc_fix_minuseq);
				NFT_LIFT0(lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind, intrastep,
				  jpc_fix_pluseq);
				jpc_qmfb1d_join(startptr, startind, endind,
				  intrastep, lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind);
			} else {
				if (lstartind == lendind) {
					*startptr = jpc_fix_asr(*startptr, 1);
				}
			}
			startptr += interstep;
		}
	}
}

/******************************************************************************\
* Code for 9/7 transform.
\******************************************************************************/

static int jpc_ns_getnumchans(jpc_qmfb1d_t *qmfb)
{
	/* Avoid compiler warnings about unused parameters. */
	qmfb = 0;

	return 2;
}

static int jpc_ns_getanalfilters(jpc_qmfb1d_t *qmfb, int len, jas_seq2d_t **filters)
{
	/* Avoid compiler warnings about unused parameters. */
	qmfb = 0;
	len = 0;
	filters = 0;

	abort();
	return -1;
}

static int jpc_ns_getsynfilters(jpc_qmfb1d_t *qmfb, int len, jas_seq2d_t **filters)
{
	jas_seq_t *lf;
	jas_seq_t *hf;

	/* Avoid compiler warnings about unused parameters. */
	qmfb = 0;

	lf = 0;
	hf = 0;

	if (len > 1 || (!len)) {
		if (!(lf = jas_seq_create(-3, 4))) {
			goto error;
		}
		jas_seq_set(lf, -3, jpc_dbltofix(-0.09127176311424948));
		jas_seq_set(lf, -2, jpc_dbltofix(-0.05754352622849957));
		jas_seq_set(lf, -1, jpc_dbltofix(0.5912717631142470));
		jas_seq_set(lf, 0, jpc_dbltofix(1.115087052456994));
		jas_seq_set(lf, 1, jpc_dbltofix(0.5912717631142470));
		jas_seq_set(lf, 2, jpc_dbltofix(-0.05754352622849957));
		jas_seq_set(lf, 3, jpc_dbltofix(-0.09127176311424948));
		if (!(hf = jas_seq_create(-3, 6))) {
			goto error;
		}
		jas_seq_set(hf, -3, jpc_dbltofix(-0.02674875741080976 * 2.0));
		jas_seq_set(hf, -2, jpc_dbltofix(-0.01686411844287495 * 2.0));
		jas_seq_set(hf, -1, jpc_dbltofix(0.07822326652898785 * 2.0));
		jas_seq_set(hf, 0, jpc_dbltofix(0.2668641184428723 * 2.0));
		jas_seq_set(hf, 1, jpc_dbltofix(-0.6029490182363579 * 2.0));
		jas_seq_set(hf, 2, jpc_dbltofix(0.2668641184428723 * 2.0));
		jas_seq_set(hf, 3, jpc_dbltofix(0.07822326652898785 * 2.0));
		jas_seq_set(hf, 4, jpc_dbltofix(-0.01686411844287495 * 2.0));
		jas_seq_set(hf, 5, jpc_dbltofix(-0.02674875741080976 * 2.0));
	} else if (len == 1) {
		if (!(lf = jas_seq_create(0, 1))) {
			goto error;
		}
		jas_seq_set(lf, 0, jpc_dbltofix(1.0));
		if (!(hf = jas_seq_create(0, 1))) {
			goto error;
		}
		jas_seq_set(hf, 0, jpc_dbltofix(2.0));
	} else {
		abort();
	}

	filters[0] = lf;
	filters[1] = hf;

	return 0;

error:
	if (lf) {
		jas_seq_destroy(lf);
	}
	if (hf) {
		jas_seq_destroy(hf);
	}
	return -1;
}

#define	NNS_LIFT0(lstartptr, lstartind, lendind, hstartptr, hstartind, hendind, step, alpha) \
{ \
	register jpc_fix_t *lptr = (lstartptr); \
	register jpc_fix_t *hptr = (hstartptr); \
	register int n = (hendind) - (hstartind); \
	jpc_fix_t twoalpha = jpc_fix_mulbyint(alpha, 2); \
	if ((hstartind) < (lstartind)) { \
		jpc_fix_pluseq(*hptr, jpc_fix_mul(*lptr, (twoalpha))); \
		hptr += (step); \
		--n; \
	} \
	if ((hendind) >= (lendind)) { \
		--n; \
	} \
	while (n-- > 0) { \
		jpc_fix_pluseq(*hptr, jpc_fix_mul(jpc_fix_add(*lptr, lptr[(step)]), (alpha))); \
		hptr += (step); \
		lptr += (step); \
	} \
	if ((hendind) >= (lendind)) { \
		jpc_fix_pluseq(*hptr, jpc_fix_mul(*lptr, (twoalpha))); \
	} \
}

#define	NNS_LIFT1(lstartptr, lstartind, lendind, hstartptr, hstartind, hendind, step, alpha) \
{ \
	register jpc_fix_t *lptr = (lstartptr); \
	register jpc_fix_t *hptr = (hstartptr); \
	register int n = (lendind) - (lstartind); \
	int twoalpha = jpc_fix_mulbyint(alpha, 2); \
	if ((hstartind) >= (lstartind)) { \
		jpc_fix_pluseq(*lptr, jpc_fix_mul(*hptr, (twoalpha))); \
		lptr += (step); \
		--n; \
	} \
	if ((lendind) > (hendind)) { \
		--n; \
	} \
	while (n-- > 0) { \
		jpc_fix_pluseq(*lptr, jpc_fix_mul(jpc_fix_add(*hptr, hptr[(step)]), (alpha))); \
		lptr += (step); \
		hptr += (step); \
	} \
	if ((lendind) > (hendind)) { \
		jpc_fix_pluseq(*lptr, jpc_fix_mul(*hptr, (twoalpha))); \
	} \
}

#define	NNS_SCALE(startptr, startind, endind, step, alpha) \
{ \
	register jpc_fix_t *ptr = (startptr); \
	register int n = (endind) - (startind); \
	while (n-- > 0) { \
		jpc_fix_muleq(*ptr, alpha); \
		ptr += (step); \
	} \
}

static void jpc_ns_analyze(jpc_qmfb1d_t *qmfb, int flags, jas_seq2d_t *x)
{
	jpc_fix_t *startptr;
	int startind;
	int endind;
	jpc_fix_t *lstartptr;
	int lstartind;
	int lendind;
	jpc_fix_t *hstartptr;
	int hstartind;
	int hendind;
	int interstep;
	int intrastep;
	int numseq;

	/* Avoid compiler warnings about unused parameters. */
	qmfb = 0;

	if (flags & JPC_QMFB1D_VERT) {
		interstep = 1;
		intrastep = jas_seq2d_rowstep(x);
		numseq = jas_seq2d_width(x);
		startind = jas_seq2d_ystart(x);
		endind = jas_seq2d_yend(x);
	} else {
		interstep = jas_seq2d_rowstep(x);
		intrastep = 1;
		numseq = jas_seq2d_height(x);
		startind = jas_seq2d_xstart(x);
		endind = jas_seq2d_xend(x);
	}

	assert(startind < endind);

	startptr = jas_seq2d_getref(x, jas_seq2d_xstart(x), jas_seq2d_ystart(x));
	if (!(flags & JPC_QMFB1D_RITIMODE)) {
		while (numseq-- > 0) {
			jpc_qmfb1d_setup(startptr, startind, endind, intrastep,
			  &lstartptr, &lstartind, &lendind, &hstartptr,
			  &hstartind, &hendind);
			if (endind - startind > 1) {
				jpc_qmfb1d_split(startptr, startind, endind,
				  intrastep, lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind);
				NNS_LIFT0(lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind, intrastep,
				  jpc_dbltofix(-1.586134342));
				NNS_LIFT1(lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind, intrastep,
				  jpc_dbltofix(-0.052980118));
				NNS_LIFT0(lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind, intrastep,
				  jpc_dbltofix(0.882911075));
				NNS_LIFT1(lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind, intrastep,
				  jpc_dbltofix(0.443506852));
				NNS_SCALE(lstartptr, lstartind, lendind,
				  intrastep, jpc_dbltofix(1.0/1.23017410558578));
				NNS_SCALE(hstartptr, hstartind, hendind,
				  intrastep, jpc_dbltofix(1.0/1.62578613134411));
			} else {
#if 0
				if (lstartind == lendind) {
					*startptr = jpc_fix_asl(*startptr, 1);
				}
#endif
			}
			startptr += interstep;
		}
	} else {
		/* The reversible integer-to-integer mode is not supported
		  for this transform. */
		abort();
	}
}

static void jpc_ns_synthesize(jpc_qmfb1d_t *qmfb, int flags, jas_seq2d_t *x)
{
	jpc_fix_t *startptr;
	int startind;
	int endind;
	jpc_fix_t *lstartptr;
	int lstartind;
	int lendind;
	jpc_fix_t *hstartptr;
	int hstartind;
	int hendind;
	int interstep;
	int intrastep;
	int numseq;

	/* Avoid compiler warnings about unused parameters. */
	qmfb = 0;

	if (flags & JPC_QMFB1D_VERT) {
		interstep = 1;
		intrastep = jas_seq2d_rowstep(x);
		numseq = jas_seq2d_width(x);
		startind = jas_seq2d_ystart(x);
		endind = jas_seq2d_yend(x);
	} else {
		interstep = jas_seq2d_rowstep(x);
		intrastep = 1;
		numseq = jas_seq2d_height(x);
		startind = jas_seq2d_xstart(x);
		endind = jas_seq2d_xend(x);
	}

	assert(startind < endind);

	startptr = jas_seq2d_getref(x, jas_seq2d_xstart(x), jas_seq2d_ystart(x));
	if (!(flags & JPC_QMFB1D_RITIMODE)) {
		while (numseq-- > 0) {
			jpc_qmfb1d_setup(startptr, startind, endind, intrastep,
			  &lstartptr, &lstartind, &lendind, &hstartptr,
			  &hstartind, &hendind);
			if (endind - startind > 1) {
				NNS_SCALE(lstartptr, lstartind, lendind,
				  intrastep, jpc_dbltofix(1.23017410558578));
				NNS_SCALE(hstartptr, hstartind, hendind,
				  intrastep, jpc_dbltofix(1.62578613134411));
				NNS_LIFT1(lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind, intrastep,
				  jpc_dbltofix(-0.443506852));
				NNS_LIFT0(lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind, intrastep,
				  jpc_dbltofix(-0.882911075));
				NNS_LIFT1(lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind, intrastep,
				  jpc_dbltofix(0.052980118));
				NNS_LIFT0(lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind, intrastep,
				  jpc_dbltofix(1.586134342));
				jpc_qmfb1d_join(startptr, startind, endind,
				  intrastep, lstartptr, lstartind, lendind,
				  hstartptr, hstartind, hendind);
			} else {
#if 0
				if (lstartind == lendind) {
					*startptr = jpc_fix_asr(*startptr, 1);
				}
#endif
			}
			startptr += interstep;
		}
	} else {
		/* The reversible integer-to-integer mode is not supported
		  for this transform. */
		abort();
	}
}

/******************************************************************************\
*
\******************************************************************************/

jpc_qmfb1d_t *jpc_qmfb1d_make(int qmfbid)
{
	jpc_qmfb1d_t *qmfb;
	if (!(qmfb = jpc_qmfb1d_create())) {
		return 0;
	}
	switch (qmfbid) {
	case JPC_QMFB1D_FT:
		qmfb->ops = &jpc_ft_ops;
		break;
	case JPC_QMFB1D_NS:
		qmfb->ops = &jpc_ns_ops;
		break;
	default:
		jpc_qmfb1d_destroy(qmfb);
		return 0;
		break;
	}
	return qmfb;
}

static jpc_qmfb1d_t *jpc_qmfb1d_create()
{
	jpc_qmfb1d_t *qmfb;
	if (!(qmfb = jas_malloc(sizeof(jpc_qmfb1d_t)))) {
		return 0;
	}
	qmfb->ops = 0;
	return qmfb;
}

jpc_qmfb1d_t *jpc_qmfb1d_copy(jpc_qmfb1d_t *qmfb)
{
	jpc_qmfb1d_t *newqmfb;

	if (!(newqmfb = jpc_qmfb1d_create())) {
		return 0;
	}
	newqmfb->ops = qmfb->ops;
	return newqmfb;
}

void jpc_qmfb1d_destroy(jpc_qmfb1d_t *qmfb)
{
	jas_free(qmfb);
}

/******************************************************************************\
*
\******************************************************************************/

void jpc_qmfb1d_getbands(jpc_qmfb1d_t *qmfb, int flags, uint_fast32_t xstart,
  uint_fast32_t ystart, uint_fast32_t xend, uint_fast32_t yend, int maxbands,
  int *numbandsptr, jpc_qmfb1dband_t *bands)
{
	int start;
	int end;

	assert(maxbands >= 2);

	if (flags & JPC_QMFB1D_VERT) {
		start = ystart;
		end = yend;
	} else {
		start = xstart;
		end = xend;
	}
	assert(jpc_qmfb1d_getnumchans(qmfb) == 2);
	assert(start <= end);
	bands[0].start = JPC_CEILDIVPOW2(start, 1);
	bands[0].end = JPC_CEILDIVPOW2(end, 1);
	bands[0].locstart = start;
	bands[0].locend = start + bands[0].end - bands[0].start;
	bands[1].start = JPC_FLOORDIVPOW2(start, 1);
	bands[1].end = JPC_FLOORDIVPOW2(end, 1);
	bands[1].locstart = bands[0].locend;
	bands[1].locend = bands[1].locstart + bands[1].end - bands[1].start;
	assert(bands[1].locend == end);
	*numbandsptr = 2;
}

/******************************************************************************\
*
\******************************************************************************/

int jpc_qmfb1d_getnumchans(jpc_qmfb1d_t *qmfb)
{
	return (*qmfb->ops->getnumchans)(qmfb);
}

int jpc_qmfb1d_getanalfilters(jpc_qmfb1d_t *qmfb, int len, jas_seq2d_t **filters)
{
	return (*qmfb->ops->getanalfilters)(qmfb, len, filters);
}

int jpc_qmfb1d_getsynfilters(jpc_qmfb1d_t *qmfb, int len, jas_seq2d_t **filters)
{
	return (*qmfb->ops->getsynfilters)(qmfb, len, filters);
}

void jpc_qmfb1d_analyze(jpc_qmfb1d_t *qmfb, int flags, jas_seq2d_t *x)
{
	(*qmfb->ops->analyze)(qmfb, flags, x);
}

void jpc_qmfb1d_synthesize(jpc_qmfb1d_t *qmfb, int flags, jas_seq2d_t *x)
{
	(*qmfb->ops->synthesize)(qmfb, flags, x);
}

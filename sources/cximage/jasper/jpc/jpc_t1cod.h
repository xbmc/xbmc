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
 * $Id$
 */

#ifndef JPC_T1COD_H
#define JPC_T1COD_H

/******************************************************************************\
* Includes.
\******************************************************************************/

#include "jasper/jas_fix.h"
#include "jasper/jas_math.h"

#include "jpc_mqcod.h"
#include "jpc_tsfb.h"

/******************************************************************************\
* Constants.
\******************************************************************************/

/* The number of bits used to index into various lookup tables. */
#define JPC_NMSEDEC_BITS	7
#define JPC_NMSEDEC_FRACBITS	(JPC_NMSEDEC_BITS - 1)

/*
 * Segment types.
 */

/* Invalid. */
#define JPC_SEG_INVALID	0
/* MQ. */
#define JPC_SEG_MQ		1
/* Raw. */
#define JPC_SEG_RAW		2

/* The nominal word size. */
#define	JPC_PREC	32

/* Tier-1 coding pass types. */
#define	JPC_SIGPASS	0	/* significance */
#define	JPC_REFPASS	1	/* refinement */
#define	JPC_CLNPASS	2	/* cleanup */

/*
 * Per-sample state information for tier-1 coding.
 */

/* The northeast neighbour has been found to be significant. */
#define	JPC_NESIG	0x0001
/* The southeast neighbour has been found to be significant. */
#define	JPC_SESIG	0x0002
/* The southwest neighbour has been found to be significant. */
#define	JPC_SWSIG	0x0004
/* The northwest neighbour has been found to be significant. */
#define	JPC_NWSIG	0x0008
/* The north neighbour has been found to be significant. */
#define	JPC_NSIG	0x0010
/* The east neighbour has been found to be significant. */
#define	JPC_ESIG	0x0020
/* The south neighbour has been found to be significant. */
#define	JPC_SSIG	0x0040
/* The west neighbour has been found to be significant. */
#define	JPC_WSIG	0x0080
/* The significance mask for 8-connected neighbours. */
#define	JPC_OTHSIGMSK \
	(JPC_NSIG | JPC_NESIG | JPC_ESIG | JPC_SESIG | JPC_SSIG | JPC_SWSIG | JPC_WSIG | JPC_NWSIG)
/* The significance mask for 4-connected neighbours. */
#define	JPC_PRIMSIGMSK	(JPC_NSIG | JPC_ESIG | JPC_SSIG | JPC_WSIG)

/* The north neighbour is negative in value. */
#define	JPC_NSGN	0x0100
/* The east neighbour is negative in value. */
#define	JPC_ESGN	0x0200
/* The south neighbour is negative in value. */
#define	JPC_SSGN	0x0400
/* The west neighbour is negative in value. */
#define	JPC_WSGN	0x0800
/* The sign mask for 4-connected neighbours. */
#define	JPC_SGNMSK	(JPC_NSGN | JPC_ESGN | JPC_SSGN | JPC_WSGN)

/* This sample has been found to be significant. */
#define JPC_SIG		0x1000
/* The sample has been refined. */
#define	JPC_REFINE	0x2000
/* This sample has been processed during the significance pass. */
#define	JPC_VISIT	0x4000

/* The number of aggregation contexts. */
#define	JPC_NUMAGGCTXS	1
/* The number of zero coding contexts. */
#define	JPC_NUMZCCTXS	9
/* The number of magnitude contexts. */
#define	JPC_NUMMAGCTXS	3
/* The number of sign coding contexts. */
#define	JPC_NUMSCCTXS	5
/* The number of uniform contexts. */
#define	JPC_NUMUCTXS	1

/* The context ID for the first aggregation context. */
#define	JPC_AGGCTXNO	0
/* The context ID for the first zero coding context. */
#define	JPC_ZCCTXNO		(JPC_AGGCTXNO + JPC_NUMAGGCTXS)
/* The context ID for the first magnitude context. */
#define	JPC_MAGCTXNO	(JPC_ZCCTXNO + JPC_NUMZCCTXS)
/* The context ID for the first sign coding context. */
#define	JPC_SCCTXNO		(JPC_MAGCTXNO + JPC_NUMMAGCTXS)
/* The context ID for the first uniform context. */
#define	JPC_UCTXNO		(JPC_SCCTXNO + JPC_NUMSCCTXS)
/* The total number of contexts. */
#define	JPC_NUMCTXS		(JPC_UCTXNO + JPC_NUMUCTXS)

/******************************************************************************\
* External data.
\******************************************************************************/

/* These lookup tables are used by various macros/functions. */
/* Do not access these lookup tables directly. */
extern int jpc_zcctxnolut[];
extern int jpc_spblut[];
extern int jpc_scctxnolut[];
extern int jpc_magctxnolut[];
extern jpc_fix_t jpc_refnmsedec[];
extern jpc_fix_t jpc_signmsedec[];
extern jpc_fix_t jpc_refnmsedec0[];
extern jpc_fix_t jpc_signmsedec0[];

/* The initial settings for the MQ contexts. */
extern jpc_mqctx_t jpc_mqctxs[];

/******************************************************************************\
* Functions and macros.
\******************************************************************************/

/* Initialize the MQ contexts. */
void jpc_initctxs(jpc_mqctx_t *ctxs);

/* Get the zero coding context. */
int jpc_getzcctxno(int f, int orient);
#define	JPC_GETZCCTXNO(f, orient) \
	(jpc_zcctxnolut[((orient) << 8) | ((f) & JPC_OTHSIGMSK)])

/* Get the sign prediction bit. */
int jpc_getspb(int f);
#define	JPC_GETSPB(f) \
	(jpc_spblut[((f) & (JPC_PRIMSIGMSK | JPC_SGNMSK)) >> 4])

/* Get the sign coding context. */
int jpc_getscctxno(int f);
#define	JPC_GETSCCTXNO(f) \
	(jpc_scctxnolut[((f) & (JPC_PRIMSIGMSK | JPC_SGNMSK)) >> 4])

/* Get the magnitude context. */
int jpc_getmagctxno(int f);
#define	JPC_GETMAGCTXNO(f) \
	(jpc_magctxnolut[((f) & JPC_OTHSIGMSK) | ((((f) & JPC_REFINE) != 0) << 11)])

/* Get the normalized MSE reduction for significance passes. */
#define	JPC_GETSIGNMSEDEC(x, bitpos)	jpc_getsignmsedec_macro(x, bitpos)
jpc_fix_t jpc_getsignmsedec_func(jpc_fix_t x, int bitpos);
#define	jpc_getsignmsedec_macro(x, bitpos) \
	((bitpos > JPC_NMSEDEC_FRACBITS) ? jpc_signmsedec[JPC_ASR(x, bitpos - JPC_NMSEDEC_FRACBITS) & JAS_ONES(JPC_NMSEDEC_BITS)] : \
	  (jpc_signmsedec0[JPC_ASR(x, bitpos - JPC_NMSEDEC_FRACBITS) & JAS_ONES(JPC_NMSEDEC_BITS)]))

/* Get the normalized MSE reduction for refinement passes. */
#define	JPC_GETREFNMSEDEC(x, bitpos)	jpc_getrefnmsedec_macro(x, bitpos)
jpc_fix_t jpc_refsignmsedec_func(jpc_fix_t x, int bitpos);
#define	jpc_getrefnmsedec_macro(x, bitpos) \
	((bitpos > JPC_NMSEDEC_FRACBITS) ? jpc_refnmsedec[JPC_ASR(x, bitpos - JPC_NMSEDEC_FRACBITS) & JAS_ONES(JPC_NMSEDEC_BITS)] : \
	  (jpc_refnmsedec0[JPC_ASR(x, bitpos - JPC_NMSEDEC_FRACBITS) & JAS_ONES(JPC_NMSEDEC_BITS)]))

/* Arithmetic shift right (with ability to shift left also). */
#define	JPC_ASR(x, n) \
	(((n) >= 0) ? ((x) >> (n)) : ((x) << (-(n))))

/* Update the per-sample state information. */
#define	JPC_UPDATEFLAGS4(fp, rowstep, s, vcausalflag) \
{ \
	register jpc_fix_t *np = (fp) - (rowstep); \
	register jpc_fix_t *sp = (fp) + (rowstep); \
	if ((vcausalflag)) { \
		sp[-1] |= JPC_NESIG; \
		sp[1] |= JPC_NWSIG; \
		if (s) { \
			*sp |= JPC_NSIG | JPC_NSGN; \
			(fp)[-1] |= JPC_ESIG | JPC_ESGN; \
			(fp)[1] |= JPC_WSIG | JPC_WSGN; \
		} else { \
			*sp |= JPC_NSIG; \
			(fp)[-1] |= JPC_ESIG; \
			(fp)[1] |= JPC_WSIG; \
		} \
	} else { \
		np[-1] |= JPC_SESIG; \
		np[1] |= JPC_SWSIG; \
		sp[-1] |= JPC_NESIG; \
		sp[1] |= JPC_NWSIG; \
		if (s) { \
			*np |= JPC_SSIG | JPC_SSGN; \
			*sp |= JPC_NSIG | JPC_NSGN; \
			(fp)[-1] |= JPC_ESIG | JPC_ESGN; \
			(fp)[1] |= JPC_WSIG | JPC_WSGN; \
		} else { \
			*np |= JPC_SSIG; \
			*sp |= JPC_NSIG; \
			(fp)[-1] |= JPC_ESIG; \
			(fp)[1] |= JPC_WSIG; \
		} \
	} \
}

/* Initialize the lookup tables used by the codec. */
void jpc_initluts(void);

/* Get the nominal gain associated with a particular band. */
int JPC_NOMINALGAIN(int qmfbid, int numlvls, int lvlno, int orient);

/* Get the coding pass type. */
int JPC_PASSTYPE(int passno);

/* Get the segment type. */
int JPC_SEGTYPE(int passno, int firstpassno, int bypass);

/* Get the number of coding passess in the segment. */
int JPC_SEGPASSCNT(int passno, int firstpassno, int numpasses, int bypass,
  int termall);

/* Is the coding pass terminated? */
int JPC_ISTERMINATED(int passno, int firstpassno, int numpasses, int termall,
  int lazy);

#endif
